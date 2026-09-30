#include "Presets.h"
#include "ManagedItems.h"
#include <map>
#undef GetObject
namespace Gallery {
namespace {
std::string Kind(RE::TESBoundObject* o) {
    if (o->As<RE::TESObjectARMO>()) return "armor";
    if (o->As<RE::TESObjectWEAP>()) return "weapon";
    if (o->As<RE::TESAmmo>()) return "ammo";
    return {};
}
const RE::BGSEquipSlot* Slot(const std::string& hand) {
    if (hand.empty()) return nullptr;
    return RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
        hand == "left" ? RE::DEFAULT_OBJECT::kLeftHandEquip : RE::DEFAULT_OBJECT::kRightHandEquip);
}
Item Encode(RE::TESBoundObject* obj, const std::string& hand) {
    auto* file = obj->GetFile(0);
    if (!file || (obj->GetFormID() >> 24) == 0xFF) throw std::runtime_error("Cannot save a dynamically generated item");
    // Strip the load index, including the ESL light-plugin index.
    const auto id = obj->GetFormID() & (file->IsLight() ? 0xFFFu : 0xFFFFFFu);
    return {std::string(file->GetFilename()),id,obj->GetName() ? obj->GetName() : "",Kind(obj),hand};
}
}
Preset SnapshotEquipment(RE::Actor* actor, std::uint32_t slotMask) {
    if (!actor) throw std::runtime_error("Target unavailable");
    Preset p;
    p.slotMask=slotMask;
    for (const auto& [obj, data] : actor->GetInventory()) {
        const auto& [count, entry] = data;
        if (!obj || count <= 0 || !entry || !entry->IsWorn() || !obj->As<RE::TESObjectARMO>()) continue;
        if(slotMask) {
            const auto* armor=obj->As<RE::TESObjectARMO>();
            if(!armor || !SlotIntersects(armor->GetSlotMask().underlying(),slotMask)) continue;
            if(!SlotFits(armor->GetSlotMask().underlying(),slotMask)) throw std::runtime_error("Item uses unselected slots: "+std::string(obj->GetName()));
        }
        p.items.push_back(Encode(obj,""));
    }
    if (p.items.empty()) throw std::runtime_error("No equipped armor or clothing to save");
    return p;
}
Preset SnapshotAccessories(RE::Actor* actor) {
    if(!actor) throw std::runtime_error("Target unavailable");
    Preset p; p.accessories=true;
    for(const auto& [obj,data]:actor->GetInventory()) {
        if(!obj || data.first<=0 || !data.second || !data.second->IsWorn()) continue;
        const auto* armor=obj->As<RE::TESObjectARMO>();
        if(!armor) continue;
        const auto mask=armor->GetSlotMask().underlying();
        if(!mask) throw std::runtime_error("Item has no equipment slots: "+std::string(obj->GetName()));
        p.slotMask|=mask; p.items.push_back(Encode(obj,""));
    }
    if(p.items.empty()) throw std::runtime_error("No equipped accessories to save.");
    return p;
}
std::string ApplyEquipment(RE::Actor* actor, const Preset& source, bool addMissing) {
    const auto p=ClothingPreset(source);
    if(p.items.empty()) throw std::runtime_error("No equipped armor or clothing to save");
    BeginManagedAddition(actor);
    auto* manager = RE::ActorEquipManager::GetSingleton();
    auto* handler = RE::TESDataHandler::GetSingleton();
    if (!actor || !manager || !handler) throw std::runtime_error("Equipment manager unavailable");
    std::vector<std::pair<RE::TESBoundObject*,const RE::BGSEquipSlot*>> resolved;
    std::map<RE::TESBoundObject*,int> needed;
    std::uint32_t resolvedMask=0;
    // Resolve every item before changing inventory or removing any equipment.
    for (const auto& i : p.items) {
        auto* form = handler->LookupForm(i.localID,i.plugin);
        auto* obj = form ? form->As<RE::TESBoundObject>() : nullptr;
        if (!obj || Kind(obj) != i.kind) throw std::runtime_error("Missing or changed mod item: " + i.plugin + " / " + i.name);
        if(p.slotMask) {
            const auto* armor=obj->As<RE::TESObjectARMO>();
            if(!armor || !SlotFits(armor->GetSlotMask().underlying(),p.slotMask)) throw std::runtime_error("Item uses unselected slots: "+i.name);
            resolvedMask|=armor->GetSlotMask().underlying();
        }
        auto* slot = Slot(i.hand);
        if (!i.hand.empty() && !slot) throw std::runtime_error("Hand equip slot unavailable");
        resolved.emplace_back(obj,slot); ++needed[obj];
    }
    if(p.accessories && (!p.slotMask || resolvedMask!=p.slotMask)) throw std::runtime_error("Accessory slots changed. Register this set again.");
    auto inventory = actor->GetInventory();
    // The selected preset wins slot conflicts. An overlapping worn armor piece
    // is removed as a whole, even when it also occupies slots outside this scope.
    // Incoming items still must fit the saved scope (validated above).
    for (const auto& [obj, count] : needed) {
        const auto found = inventory.find(obj);
        const auto owned = found == inventory.end() ? 0 : std::max(0,found->second.first);
        if (owned < count && !addMissing) throw std::runtime_error("Not owned: " + std::string(obj->GetName()) + ". Enable Add missing base items for a new game.");
    }
    for (const auto& [obj, count] : needed) {
        const auto found = inventory.find(obj);
        const auto owned = found == inventory.end() ? 0 : std::max(0,found->second.first);
        if(owned<count) {
            AddManagedItems(actor,obj,count-owned,owned==0);
        }
    }
    // Use each worn instance when unequipping; never destroy inventory items.
    for (const auto& [obj,data] : inventory) {
        const auto& entry = data.second;
        if (!obj || !entry || !entry->extraLists || !obj->As<RE::TESObjectARMO>()) continue;
        if(p.slotMask) {
            const auto* armor=obj->As<RE::TESObjectARMO>();
            if(!armor || !SlotIntersects(armor->GetSlotMask().underlying(),p.slotMask)) continue;
        }
        for (auto* extra : *entry->extraLists) {
            if (!extra) continue;
            const bool right = extra->HasType<RE::ExtraWorn>();
            const bool left = extra->HasType<RE::ExtraWornLeft>();
            if (right) manager->UnequipObject(actor,obj,extra,1,obj->As<RE::TESObjectWEAP>() ? Slot("right") : nullptr,false,false,false,true);
            // The first unequip may destroy its extra list; do not reuse that
            // pointer if an instance happened to carry both worn markers.
            if (left) manager->UnequipObject(actor,obj,right ? nullptr : extra,1,obj->As<RE::TESObjectWEAP>() ? Slot("left") : nullptr,false,false,false,true);
        }
    }
    for (const auto& [obj,slot] : resolved) manager->EquipObject(actor,obj,nullptr,1,slot,false,false,false,true);
    TrackNewManagedItems(actor);
    // The blocking gallery can defer the normal actor model refresh until close.
    // Refresh once after the complete outfit, on the game task thread. CommonLib
    // also dispatches NiNodeUpdateEvent for consumers such as appearance plugins.
    actor->Update3DModel();
    return "Equip requests sent: " + p.name + ". Check the preview.";
}
}
