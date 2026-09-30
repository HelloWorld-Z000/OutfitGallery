#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <utility>

namespace Gallery {
struct ManagedIdentity {
    std::uint32_t form{}, owner{};
    std::uint16_t id{};
    auto operator<=>(const ManagedIdentity&) const = default;
};

// Engine-independent bookkeeping; callers hold ledgerMutex. Owner is the actor
// reference ID, never its base NPC form. Transfer events relinquish claims even
// when the engine omits an instance ID. A transfer back cannot restore a claim.
struct ManagedLedger {
    std::set<ManagedIdentity> items;
    std::map<std::pair<std::uint32_t,std::uint32_t>,int> pending;

    void Clear() {items.clear(); pending.clear();}
    void Begin(std::uint32_t owner) {
        std::erase_if(pending,[&](const auto& p){return p.first.first==owner;});
    }
    std::map<std::uint32_t,int> TakePending(std::uint32_t owner) {
        std::map<std::uint32_t,int> result;
        for(const auto& [key,count]:pending) if(key.first==owner) result[key.second]=count;
        Begin(owner);
        return result;
    }
    std::set<ManagedIdentity> ForActor(std::uint32_t owner) const {
        std::set<ManagedIdentity> result;
        for(const auto& key:items) if(key.owner==owner) result.insert(key);
        return result;
    }
    std::size_t Transfer(std::uint32_t form,std::uint32_t from,std::uint32_t to) {
        if(!from || from==to) return 0;
        pending.erase({from,form});
        return std::erase_if(items,[&](const auto& key){return key.owner==from && key.form==form;});
    }
};
}
