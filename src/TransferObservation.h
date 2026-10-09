#pragma once
#include "ManagedLedger.h"
#include <vector>

namespace Gallery {
// Independent, bounded, session-only observations. Never used by deletion.
struct TransferObservation {
    std::map<ManagedIdentity,std::uint32_t> departed;
    static constexpr std::size_t limit=128;
    void Clear() {departed.clear();}
    std::size_t Capture(const ManagedLedger& ledger,std::uint32_t form,
                        std::uint32_t from,std::uint32_t to) {
        if(!from || from==to) return 0;
        std::size_t added=0;
        for(const auto& key:ledger.items) if(key.form==form && key.owner==from) {
            if(departed.contains(key)) {departed[key]=to; continue;}
            if(departed.size()<limit) {departed.emplace(key,to); ++added;}
        }
        return added;
    }
};
struct TransferCandidate {
    std::uint32_t owner{};
    std::uint16_t id{};
    int count{};
    bool protectedItem{},worn{};
};
inline const char* ClassifyTransfer(const ManagedIdentity& key,int total,
                                  const std::vector<TransferCandidate>& candidates) {
    if(total<=0) return "absent";
    const TransferCandidate* match=nullptr;
    unsigned matches=0;
    for(const auto& c:candidates) if(c.owner==key.owner && c.id==key.id) {match=&c; ++matches;}
    if(!matches) return "identity-not-found";
    if(matches!=1 || match->count!=1) return "ambiguous";
    if(match->protectedItem) return "matching-protected";
    if(match->worn) return "matching-worn";
    // A matching ID is evidence only: copied/reused IDs cannot prove provenance.
    return "matching-unworn-observation-only";
}
}
