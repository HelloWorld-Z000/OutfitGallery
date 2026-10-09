#pragma once
#include <cstdint>
namespace Gallery {
struct GeneratedIdentityEvidence {
    bool newPending{}, observedAfterCreation{};
    std::uint32_t actor{}, observedOwner{}, currentOwner{};
    std::uint16_t observedID{}, currentID{};
    int expectedCount{}, currentCount{}, instanceCount{};
    unsigned matchingIDs{};
    bool reserved{};
};
inline bool CanAdoptGeneratedIdentity(const GeneratedIdentityEvidence& e) {
    return e.newPending && e.observedAfterCreation && e.actor &&
        e.observedOwner==e.actor && e.currentOwner==e.actor &&
        e.observedID && e.currentID==e.observedID &&
        e.expectedCount==1 && e.currentCount==1 && e.instanceCount==1 &&
        e.matchingIDs==1 && !e.reserved;
}
}
