#pragma once
#include <cstdint>
#include <set>

namespace Gallery {
// The engine returns a free inventory ID, not an advancing sequence. Its result
// may still be reserved by a saved preset or ledger entry outside the inventory.
inline std::uint16_t FindAvailableItemID(std::uint16_t candidate, const std::set<std::uint16_t>& used) {
    if (!candidate) return 0; // Respect an engine allocation failure.
    for (unsigned n=0; n<65535; ++n) {
        if (!used.contains(candidate)) return candidate;
        candidate=candidate==65535 ? 1 : static_cast<std::uint16_t>(candidate+1);
    }
    return 0;
}
}
