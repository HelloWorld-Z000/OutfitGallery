#include "StudioSafety.h"
#include <iostream>
int main() {
 using Gallery::StudioCombatAllowed;
 if(!StudioCombatAllowed(false,false,true,false)) return 1; // ordinary player entry remains available
 if(StudioCombatAllowed(true,true,true,false)) return 2; // opt-out blocks player combat
 if(!StudioCombatAllowed(true,true,true,true)) return 3; // opt-in permits player combat
 if(!StudioCombatAllowed(false,false,false,true)) return 4; // peaceful follower still available
 if(StudioCombatAllowed(true,false,false,true)) return 5; // player combat blocks peaceful follower
 if(StudioCombatAllowed(false,true,false,true)) return 6; // follower combat blocks peaceful player
 if(StudioCombatAllowed(true,true,false,true)) return 7; // both fighting never bypasses follower gate
 std::cout<<"PASS: opt-in player combat entry and strict follower combat gates\n";
}
