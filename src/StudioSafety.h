#pragma once
namespace Gallery {
// Combat entry is an opt-in for the player only. Followers retain both gates.
inline bool StudioCombatAllowed(bool playerCombat,bool targetCombat,bool playerTarget,bool allow) {
    return (!playerCombat && !targetCombat) || (playerTarget && allow);
}
}
