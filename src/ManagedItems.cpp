#include "ManagedItems.h"
#include "EnchantmentSignature.h"
#include "ManagedLedger.h"
#include "Presets.h"
#include "FollowerOutfits.h"
#include <mutex>
#include <set>
#include <map>
#include <random>

namespace Gallery {
namespace {
using Identity=ManagedIdentity;
std::mutex ledgerMutex;
ManagedLedger ledger;
std::map<std::uint16_t,std::uint64_t> preferredScopes;
std::set<std::uint16_t> reservedPreferredIDs;
constexpr std::uint32_t preferredRecord=0x454E4348;
constexpr std::uint32_t namespaceID=0x4F474D49; // OGMI
constexpr std::uint32_t itemRecord=0x4954454D; // ITEM

void Forget(const Identity& key) {std::scoped_lock lock(ledgerMutex); ledger.items.erase(key);}
void Revert(SKSE::SerializationInterface*) {
    {std::scoped_lock lock(ledgerMutex); ledger.Clear(); preferredScopes.clear(); reservedPreferredIDs.clear();}
    ClearFollowerOutfits();
}
void Save(SKSE::SerializationInterface* s) {
    SaveFollowerOutfits(s);
    if(!preferredScopes.empty()) {
        const auto count=static_cast<std::uint32_t>(preferredScopes.size());
        if(!s->OpenRecord(preferredRecord,2) || !s->WriteRecordData(count)) {SKSE::log::error("Enchanted item: identity scope save failed"); return;}
        for(const auto& [id,scope]:preferredScopes) if(!s->WriteRecordData(id) || !s->WriteRecordData(scope)) {SKSE::log::error("Enchanted item: reserved ID save failed"); return;}
    }
    std::set<Identity> copy;
    {std::scoped_lock lock(ledgerMutex); copy=ledger.items;}
    for(const auto& k:copy) {
        if(!s->OpenRecord(itemRecord,k.owner==0x14 ? 1 : 2) || !s->WriteRecordData(k.form) || !s->WriteRecordData(k.owner) || !s->WriteRecordData(k.id)) {
            SKSE::log::error("Managed item ledger could not be saved"); return;
        }
    }
}
void Load(SKSE::SerializationInterface* s) {
    ClearFollowerOutfits();
    preferredScopes.clear(); reservedPreferredIDs.clear();
    std::set<Identity> next;
    std::uint32_t type{},version{},length{};
    while(s->GetNextRecordInfo(type,version,length)) {
        if(type==preferredRecord) {
            std::uint32_t count{};
            if(version!=2 || length<4 || s->ReadRecordData(count)!=4 || count>65535 || length!=4+count*10) continue;
            std::map<std::uint16_t,std::uint64_t> scopes; bool valid=true;
            for(std::uint32_t n=0;n<count;++n) {std::uint16_t id{}; std::uint64_t scope{}; if(s->ReadRecordData(id)!=2 || s->ReadRecordData(scope)!=8 || !id || !scope || !scopes.emplace(id,scope).second) valid=false;}
            if(valid) {preferredScopes=std::move(scopes); for(const auto& [id,scope]:preferredScopes) reservedPreferredIDs.insert(id);}
            continue;
        }
        if(LoadFollowerOutfit(s,type,version,length)) continue;
        if(type!=itemRecord || (version!=1 && version!=2) || length!=10) continue;
        Identity old, current;
        if(s->ReadRecordData(old.form)!=4 || s->ReadRecordData(old.owner)!=4 || s->ReadRecordData(old.id)!=2) continue;
        if(!old.id || !s->ResolveFormID(old.form,current.form) || !s->ResolveFormID(old.owner,current.owner)) continue;
        current.id=old.id;
        if(current.owner && (version==2 || current.owner==0x14) && next.size()<10000) next.insert(current);
    }
    std::scoped_lock lock(ledgerMutex); ledger.Clear(); ledger.items=std::move(next);
}
class Transfers final : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* e,RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
        if(e) {
            // No engine calls or mutations under the event/ledger lock.
            std::scoped_lock lock(ledgerMutex);
            ledger.Transfer(e->baseObj,e->oldContainer,e->newContainer);
        }
        return RE::BSEventNotifyControl::kContinue;
    }
} transfers;
}
std::uint64_t PreferredScope(std::uint16_t id) {const auto it=preferredScopes.find(id); return it==preferredScopes.end()?0:it->second;}
PreferredItem RememberEnchanted(RE::Actor* actor,RE::ExtraDataList* extra) {
    if(!actor || actor->GetFormID()!=0x14 || !extra || !extra->HasType<RE::ExtraEnchantment>()) return {};
    PreferredItem result; result.custom=true;
    if(extra->GetCount()!=1) return result;
    auto* changes=actor->GetInventoryChanges(); if(!changes) return result;
    auto* uid=extra->GetByType<RE::ExtraUniqueID>();
    // Never replace identities owned by the engine or another mod/container.
    if(uid && (uid->baseID!=0x14 || !uid->uniqueID)) {
        SKSE::log::info("Enchanted item: registration fallback: existing identity owner={:08X} id={} is outside the supported player identity scope",uid->baseID,uid->uniqueID);
        return result;
    }
    if(!uid) {
        std::set<std::uint16_t> used=reservedPreferredIDs;
        for(const auto& [obj,data]:actor->GetInventory()) if(data.second && data.second->extraLists)
            for(auto* e:*data.second->extraLists) if(e) if(auto* id=e->GetByType<RE::ExtraUniqueID>(); id && id->baseID==0x14) used.insert(id->uniqueID);
        {std::scoped_lock lock(ledgerMutex); for(const auto& key:ledger.items) if(key.owner==0x14) used.insert(key.id);}
        std::uint16_t id{};
        for(unsigned n=0;n<65536;++n) {const auto next=changes->GetNextUniqueID(); if(next && !used.contains(next)) {id=next; break;}}
        if(!id) return result;
        uid=new RE::ExtraUniqueID(0x14,id); extra->Add(uid);
    }
    auto& scope=preferredScopes[uid->uniqueID];
    if(!scope) {std::random_device random; do {scope=(static_cast<std::uint64_t>(random())<<32)^random();} while(!scope);}
    reservedPreferredIDs.insert(uid->uniqueID);
    result.scope=scope; result.id=uid->uniqueID;
    result.signature=EnchantmentSignature(extra);
    SKSE::log::debug("Enchanted item: registered player enchanted instance id={}; save the game to persist identity scope",uid->uniqueID);
    return result;
}
void InitializeManagedItems() {
    auto* s=SKSE::GetSerializationInterface();
    s->SetUniqueID(namespaceID); s->SetSaveCallback(Save); s->SetLoadCallback(Load); s->SetRevertCallback(Revert);
}
void RegisterManagedItemEvents() {
    if(auto* source=RE::ScriptEventSourceHolder::GetSingleton()) source->AddEventSink(&transfers);
}
void BeginManagedAddition(RE::Actor* actor) {
    if(actor) {std::scoped_lock lock(ledgerMutex); ledger.Begin(actor->GetFormID());}
}
void AddManagedItems(RE::Actor* actor,RE::TESBoundObject* obj,int count,bool previouslyAbsent) {
    if(!actor || !obj || count<=0) return;
    // Record before AddObject: a synchronous outgoing transfer must be able to
    // cancel the claim. Incoming add events do not claim existing possessions.
    if(previouslyAbsent) {std::scoped_lock lock(ledgerMutex); ledger.pending[{actor->GetFormID(),obj->GetFormID()}]=count;}
    actor->AddObjectToContainer(obj,nullptr,count,nullptr);
}
void TrackNewManagedItems(RE::Actor* actor) {
    if(!actor) return;
    auto* changes=actor->GetInventoryChanges();
    std::map<RE::FormID,int> added;
    {std::scoped_lock lock(ledgerMutex); added=ledger.TakePending(actor->GetFormID());}
    if(!changes) return;
    auto inventory=actor->GetInventory();
    std::set<std::uint16_t> used;
    if(actor->GetFormID()==0x14) used=reservedPreferredIDs;
    for(const auto& [_,data]:inventory) if(data.second && data.second->extraLists) {
        for(auto* extra:*data.second->extraLists) if(extra) if(auto* id=extra->GetByType<RE::ExtraUniqueID>()) {
            if(id->baseID==actor->GetFormID()) used.insert(id->uniqueID);
        }
    }
    {std::scoped_lock lock(ledgerMutex); for(const auto& k:ledger.items) if(k.owner==actor->GetFormID()) used.insert(k.id);}
    for(const auto& [obj,data]:inventory) {
        if(!obj) continue;
        const auto found=added.find(obj->GetFormID());
        if(found==added.end() || data.first!=found->second || !data.second || !data.second->extraLists) continue;
        std::vector<RE::ExtraDataList*> instances;
        for(auto* extra:*data.second->extraLists) if(extra && extra->GetCount()==1 && (extra->HasType<RE::ExtraWorn>() || extra->HasType<RE::ExtraWornLeft>())) instances.push_back(extra);
        if(instances.size()!=static_cast<std::size_t>(found->second)) continue;
        for(auto* extra:instances) {
            // Use an engine-created worn list: its allocation/layout differs on
            // pre/post 1.6.629. Never allocate ExtraDataList using compile-time size.
            auto* uid=extra->GetByType<RE::ExtraUniqueID>();
            if(uid) continue; // unknown provenance: never replace another system's ID
            std::uint16_t id{};
            for(unsigned attempt=0;attempt<65536;++attempt) {
                const auto candidate=changes->GetNextUniqueID();
                if(candidate && !used.contains(candidate)) {id=candidate; break;}
            }
            if(!id) continue;
            extra->Add(new RE::ExtraUniqueID(actor->GetFormID(),id));
            used.insert(id);
            {std::scoped_lock lock(ledgerMutex); ledger.items.insert({obj->GetFormID(),actor->GetFormID(),id});}
            SKSE::log::info("Tracking gallery item {:08X}, instance {}, actor {:08X}",obj->GetFormID(),id,actor->GetFormID());
        }
    }
}
void ReclaimManagedItems(RE::Actor* actor) {
    if(!actor || !actor->Get3D() || actor->IsDead() || actor->IsDisabled()) return;
    if(actor!=RE::PlayerCharacter::GetSingleton() && (!actor->IsPlayerTeammate() || actor->IsInCombat())) return;
    std::set<Identity> copy;
    {std::scoped_lock lock(ledgerMutex); copy=ledger.ForActor(actor->GetFormID());}
    for(const auto& key:copy) {
        // Re-read after each removal: removing an item can invalidate extra lists.
        auto inventory=actor->GetInventory();
        RE::TESBoundObject* object{};
        RE::ExtraDataList* instance{};
        unsigned matches{}; bool protectedItem=false; int beforeCount{};
        for(const auto& [obj,data]:inventory) if(obj && obj->GetFormID()==key.form && data.second && data.second->extraLists) {
            protectedItem=data.second->IsQuestObject();
            beforeCount=data.first;
            for(auto* extra:*data.second->extraLists) if(extra) if(auto* uid=extra->GetByType<RE::ExtraUniqueID>()) {
                if(uid->baseID==key.owner && uid->uniqueID==key.id) {object=obj; instance=extra; ++matches;}
            }
        }
        if(matches!=1 || !instance || instance->GetCount()!=1 || beforeCount<=0 || !object || !object->As<RE::TESObjectARMO>()) {
            SKSE::log::info("Preserving ambiguous gallery item {:08X}, instance {}, matches={}, count={}",key.form,key.id,matches,instance?instance->GetCount():0);
            Forget(key); continue;
        }
        // User customization relinquishes our claim. Never remove tempered,
        // enchanted, renamed, favorited or quest equipment automatically.
        if(protectedItem || instance->HasType<RE::ExtraHealth>() || instance->HasType<RE::ExtraEnchantment>() || instance->HasType<RE::ExtraTextDisplayData>() || instance->HasType<RE::ExtraHotkey>() || instance->HasType<RE::ExtraPoison>()) {
            SKSE::log::info("Preserving customized gallery item {:08X}, instance {}: quest={}, health={}, enchant={}, name={}, favorite={}, poison={}",key.form,key.id,protectedItem,instance->HasType<RE::ExtraHealth>(),instance->HasType<RE::ExtraEnchantment>(),instance->HasType<RE::ExtraTextDisplayData>(),instance->HasType<RE::ExtraHotkey>(),instance->HasType<RE::ExtraPoison>());
            Forget(key); continue;
        }
        if(instance->HasType<RE::ExtraWorn>() || instance->HasType<RE::ExtraWornLeft>()) continue;
        bool claimed;
        {std::scoped_lock lock(ledgerMutex); claimed=ledger.items.erase(key)!=0;}
        if(!claimed) continue;
        // Explicit instance, count one. Never a base-form-only removal fallback.
        actor->RemoveItem(object,1,RE::ITEM_REMOVE_REASON::kRemove,instance,nullptr);
        const auto after=actor->GetInventory();
        const auto found=after.find(object);
        const auto afterCount=found==after.end()?0:found->second.first;
        bool identityRemains=false;
        if(found!=after.end() && found->second.second && found->second.second->extraLists) {
            for(auto* extra:*found->second.second->extraLists) if(extra) if(auto* uid=extra->GetByType<RE::ExtraUniqueID>()) {
                if(uid->baseID==key.owner && uid->uniqueID==key.id) identityRemains=true;
            }
        }
        if(afterCount==beforeCount-1 && !identityRemains) SKSE::log::info("Reclaimed gallery item {:08X}, instance {}, actor {:08X}, inventory {} -> {} (verified)",key.form,key.id,key.owner,beforeCount,afterCount);
        else SKSE::log::warn("Gallery removal not confirmed {:08X}, instance {}, inventory {} -> {}, identity remains={}; no base-form fallback",key.form,key.id,beforeCount,afterCount,identityRemains);
    }
}
}
