#pragma once
#include <cstdint>
#include <vector>
#include <optional>
#include <span>
#include <utility>

namespace Gallery {
// Each persisted record is actor, count, then base armor IDs. Resolve every ID
// before accepting any state; a removed plugin must not produce a partial outfit.
template<class Resolve>
std::optional<std::pair<std::uint32_t,std::vector<std::uint32_t>>> DecodeFollowerOutfit(
    std::span<const std::uint32_t> data,Resolve resolve) {
    if(data.size()<3 || !data[1] || data[1]>256 || data.size()!=data[1]+2) return {};
    std::uint32_t actor{};
    if(!resolve(data[0],actor) || !actor || actor==0x14) return {};
    std::vector<std::uint32_t> armor;
    for(std::size_t i=2;i<data.size();++i) {
        std::uint32_t id{};
        if(!resolve(data[i],id) || !id) return {};
        armor.push_back(id);
    }
    return std::pair{actor,std::move(armor)};
}
// A bounded retry policy avoids fighting another outfit manager indefinitely.
struct FollowerOutfitState {
    std::vector<std::uint32_t> armor;
    std::uint64_t nextCheck{}, stableSince{};
    unsigned corrections{};
    bool Observe(bool matches,std::uint64_t now) {
        nextCheck=now+5000;
        if(matches) {
            if(!stableSince) stableSince=now;
            if(now-stableSince>=30000) corrections=0;
            return false;
        }
        stableSince=0;
        return ++corrections<=3;
    }
};
}
