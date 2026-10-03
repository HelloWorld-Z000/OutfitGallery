#pragma once
#include <cstdint>
#include <vector>
#include <string>
namespace Gallery {
// Scope belongs to the SKSE co-save, never to a global settings file.
struct PreferredItem {
    bool custom{};
    std::uint64_t scope{};
    std::uint16_t id{};
    std::string signature;
};
struct PreferredCandidate { std::uint16_t id{}; bool playerOwnedID{}, enchanted{}; int count{}; std::string signature; };
inline int FindPreferred(const PreferredItem& wanted, std::uint64_t scope,
                         const std::vector<PreferredCandidate>& candidates) {
    if(!wanted.custom || !wanted.id || !scope || wanted.scope!=scope) return -1;
    int match=-1;
    for(std::size_t n=0;n<candidates.size();++n) {
        const auto& c=candidates[n];
        if(c.id!=wanted.id || !c.playerOwnedID) continue;
        if(match!=-1 || !c.enchanted || c.count!=1) return -1;
        match=static_cast<int>(n);
    }
    if(match>=0 || wanted.signature.empty()) return match;
    // Identity was not found. Only one equivalent instance is acceptable;
    // stacks and duplicate equivalents must not silently choose an arbitrary copy.
    for(std::size_t n=0;n<candidates.size();++n) {
        const auto& c=candidates[n];
        if(!c.enchanted || c.signature!=wanted.signature) continue;
        if(match!=-1 || c.count!=1) return -1;
        match=static_cast<int>(n);
    }
    return match;
}
}
