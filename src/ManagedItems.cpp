#include "ManagedItems.h"
#include <mutex>
#include <set>
#include <map>

namespace Gallery {
namespace {
struct Identity {
    RE::FormID form{}, owner{};
    std::uint16_t id{};
    auto operator<=>(const Identity&) const = default;
};
std::mutex ledgerMutex;
std::set<Identity> ledger;
std::map<RE::FormID,int> newItems;
constexpr std::uint32_t namespaceID=0x4F474D49; // OGMI
constexpr std::uint32_t itemRecord=0x4954454D; // ITEM

void Forget(const Identity& key) {std::scoped_lock lock(ledgerMutex); ledger.erase(key);}
void Revert(SKSE::SerializationInterface*) {std::scoped_lock lock(ledgerMutex); ledger.clear(); newItems.clear();}
void Save(SKSE::SerializationInterface* s) {
    std::set<Identity> copy;
    {std::scoped_lock lock(ledgerMutex); copy=ledger;}
    for(const auto& k:copy) {
        if(!s->OpenRecord(itemRecord,1) || !s->WriteRecordData(k.form) || !s->WriteRecordData(k.owner) || !s->WriteRecordData(k.id)) {
            SKSE::log::error("Managed item ledger could not be saved"); return;
        }
    }
}
void Load(SKSE::SerializationInterface* s) {
    std::set<Identity> next;
    std::uint32_t type{},version{},length{};
    while(s->GetNextRecordInfo(type,version,length)) {
        if(type!=itemRecord || version!=1 || length!=10) continue;
        Identity old, current;
        if(s->ReadRecordData(old.form)!=4 || s->ReadRecordData(old.owner)!=4 || s->ReadRecordData(old.id)!=2) continue;
        if(!old.id || !s->ResolveFormID(old.form,current.form) || !s->ResolveFormID(old.owner,current.owner)) continue;
        current.id=old.id;
        if(current.owner==0x14 && next.size()<10000) next.insert(current);
    }
    std::scoped_lock lock(ledgerMutex); ledger=std::move(next);
}
class Transfers final : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* e,RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
        if(e && (e->oldContainer==0x14 || e->newContainer==0x14)) {
            // Selling, dropping or storing an item relinquishes ownership. Some
            // events omit the instance ID: retire all claims for that base form.
            // No engine calls or inventory mutation while inside the event lock.
            std::scoped_lock lock(ledgerMutex);
            newItems.erase(e->baseObj);
            if(e->oldContainer==0x14 && e->newContainer!=0x14) {
                const auto erased=std::erase_if(ledger,[&](const auto& k){return k.form==e->baseObj;});
                if(erased) SKSE::log::info("Released {} gallery claims for transferred item {:08X}",erased,e->baseObj);
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
} transfers;
}
void InitializeManagedItems() {
    auto* s=SKSE::GetSerializationInterface();
    s->SetUniqueID(namespaceID); s->SetSaveCallback(Save); s->SetLoadCallback(Load); s->SetRevertCallback(Revert);
}
void RegisterManagedItemEvents() {
    if(auto* source=RE::ScriptEventSourceHolder::GetSingleton()) source->AddEventSink(&transfers);
}
void BeginManagedAddition() {std::scoped_lock lock(ledgerMutex); newItems.clear();}
void AddManagedItems(RE::PlayerCharacter* player,RE::TESBoundObject* obj,int count,bool previouslyAbsent) {
    player->AddObjectToContainer(obj,nullptr,count,nullptr);
    if(previouslyAbsent) {std::scoped_lock lock(ledgerMutex); newItems[obj->GetFormID()]=count;}
}
void TrackNewManagedItems() {
    auto* player=RE::PlayerCharacter::GetSingleton();
    auto* changes=player?player->GetInventoryChanges():nullptr;
    std::map<RE::FormID,int> added;
    {std::scoped_lock lock(ledgerMutex); added=std::move(newItems); newItems.clear();}
    if(!changes) return;
    auto inventory=player->GetInventory();
    std::set<std::uint16_t> used;
    for(const auto& [_,data]:inventory) if(data.second && data.second->extraLists) {
        for(auto* extra:*data.second->extraLists) if(extra) if(auto* id=extra->GetByType<RE::ExtraUniqueID>()) {
            if(id->baseID==player->GetFormID()) used.insert(id->uniqueID);
        }
    }
    {std::scoped_lock lock(ledgerMutex); for(const auto& k:ledger) used.insert(k.id);}
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
            extra->Add(new RE::ExtraUniqueID(player->GetFormID(),id));
            used.insert(id);
            {std::scoped_lock lock(ledgerMutex); ledger.insert({obj->GetFormID(),player->GetFormID(),id});}
            SKSE::log::info("Tracking gallery item {:08X}, instance {}",obj->GetFormID(),id);
        }
    }
}
void ReclaimManagedItems() {
    auto* player=RE::PlayerCharacter::GetSingleton();
    if(!player) return;
    std::set<Identity> copy;
    {std::scoped_lock lock(ledgerMutex); copy=ledger;}
    for(const auto& key:copy) {
        // Re-read after each removal: removing an item can invalidate extra lists.
        auto inventory=player->GetInventory();
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
        if(matches!=1 || !instance || instance->GetCount()!=1) {
            SKSE::log::info("Preserving ambiguous gallery item {:08X}, instance {}, matches={}, count={}",key.form,key.id,matches,instance?instance->GetCount():0);
            Forget(key); continue;
        }
        // Player customization relinquishes our claim. Never remove tempered,
        // enchanted, renamed, favorited or quest equipment automatically.
        if(protectedItem || instance->HasType<RE::ExtraHealth>() || instance->HasType<RE::ExtraEnchantment>() || instance->HasType<RE::ExtraTextDisplayData>() || instance->HasType<RE::ExtraHotkey>() || instance->HasType<RE::ExtraPoison>()) {
            SKSE::log::info("Preserving customized gallery item {:08X}, instance {}: quest={}, health={}, enchant={}, name={}, favorite={}, poison={}",key.form,key.id,protectedItem,instance->HasType<RE::ExtraHealth>(),instance->HasType<RE::ExtraEnchantment>(),instance->HasType<RE::ExtraTextDisplayData>(),instance->HasType<RE::ExtraHotkey>(),instance->HasType<RE::ExtraPoison>());
            Forget(key); continue;
        }
        if(instance->HasType<RE::ExtraWorn>() || instance->HasType<RE::ExtraWornLeft>()) continue;
        bool claimed;
        {std::scoped_lock lock(ledgerMutex); claimed=ledger.erase(key)!=0;}
        if(!claimed) continue;
        // Explicit instance, count one. Never a base-form-only removal fallback.
        player->RemoveItem(object,1,RE::ITEM_REMOVE_REASON::kRemove,instance,nullptr);
        const auto after=player->GetInventory();
        const auto found=after.find(object);
        const auto afterCount=found==after.end()?0:found->second.first;
        bool identityRemains=false;
        if(found!=after.end() && found->second.second && found->second.second->extraLists) {
            for(auto* extra:*found->second.second->extraLists) if(extra) if(auto* uid=extra->GetByType<RE::ExtraUniqueID>()) {
                if(uid->baseID==key.owner && uid->uniqueID==key.id) identityRemains=true;
            }
        }
        if(afterCount==beforeCount-1 && !identityRemains) SKSE::log::info("Reclaimed gallery item {:08X}, instance {}, inventory {} -> {} (verified)",key.form,key.id,beforeCount,afterCount);
        else SKSE::log::warn("Gallery removal not confirmed {:08X}, instance {}, inventory {} -> {}, identity remains={}; no base-form fallback",key.form,key.id,beforeCount,afterCount,identityRemains);
    }
}
}
