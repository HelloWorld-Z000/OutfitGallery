#include "AutoOutfitPolicy.h"
#include <stdexcept>
#include <iostream>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* message){if(!v) throw std::runtime_error(message);}
int main(){try{
    check(CombatStyle(Hand::Empty,Hand::Empty)==Place::Unarmed,"both hands empty");
    check(CombatStyle(Hand::Empty,Hand::Other)==Place::Combat,"shield/torch is not bare fists");
    check(CombatStyle(Hand::Empty,Hand::Magic)==Place::Combat,"left-only spell uses generic fallback");
    check(CombatStyle(Hand::Melee,Hand::Magic)==Place::Melee,"right sword beats left spell");
    check(CombatStyle(Hand::Magic,Hand::Melee)==Place::Magic,"right spell beats left sword");
    check(CombatStyle(Hand::Melee,Hand::Melee)==Place::Melee,"dual melee");
    check(CombatStyle(Hand::Magic,Hand::Magic)==Place::Magic,"dual magic");
    check(CombatStyle(Hand::Archery,Hand::Empty)==Place::Archery && CombatStyle(Hand::Empty,Hand::Archery)==Place::Archery,"ranged either hand");
    Slots slots;slots[4]="battle.png";
    check(!HasCombatStyles(slots),"legacy assignments do not opt into style switching");
    slots[18]="melee.png";slots[19]="magic.png";slots[21]="fists.png";
    check(HasCombatStyles(slots),"style assignment enables classification");
    check(Choose(Place::Melee,slots,true)=="melee.png" && Choose(Place::Archery,slots,true)=="battle.png","dedicated style then generic combat fallback");
    slots[4].clear();slots[0]="town-clothes.png";
    check(Choose(Place::Archery,slots,true).empty(),"unassigned combat never falls into town/default");
    StyleDebounce d;
    check(!d.Observe(Place::Melee,1000) && d.Observe(Place::Melee,2000),"armed stability interval");
    check(!d.Observe(Place::Unarmed,2100) && !d.Observe(Place::Unarmed,3100),"transient empty hands cannot equip fists");
    check(!d.Observe(Place::Archery,3200) && d.Observe(Place::Archery,4200),"bow interrupts pending fists");
    check(!d.Observe(Place::Unarmed,5000) && !d.Observe(Place::Unarmed,6999) && d.Observe(Place::Unarmed,7000),"real empty hands settle after two seconds");
    d.Block();check(!d.Observe(Place::Unarmed,20000),"menu/pause/load time cannot count as stable observations");
    Policy p;CombatSession battle;
    const int melee=static_cast<int>(Place::Melee),magic=static_cast<int>(Place::Magic);
    battle.Observe(true,p);
    check(battle.Allowed(true,true,melee) && !battle.Allowed(false,true,melee),"combat opt-in remains required");
    check(p.Observe(melee,1000,0) && p.Attempt("melee.png",1000),"initial melee change");
    p.Applied("melee.png");battle.Consume(melee);
    check(!battle.Allowed(true,true,melee),"same category never reapplies even if weapon changed");
    check(battle.Allowed(true,true,magic) && !p.Observe(magic,4000,0),"style change retains global cooldown");
    check(p.Observe(magic,6000,0) && p.Attempt("magic.png",6000),"switch during the same battle");
    p.Applied("magic.png");battle.Consume(magic);
    check(battle.Allowed(true,true,melee) && p.Observe(melee,11000,0),"return to prior style works");
    battle.Consume(melee);p.Attempt("invalid.png",11000);
    check(!battle.Allowed(true,true,melee),"invalid style not retried every tick");
    p={};check(!battle.Allowed(true,true,melee),"config reset does not retry the consumed style");
    battle.Manual(p);p={};
    for(int i:{4,18,19,20,21})check(!battle.Allowed(true,true,i),"manual selection survives style/config changes for entire battle");
    battle.Observe(false,p);battle.Observe(true,p);
    check(battle.Allowed(true,true,magic),"next battle clears manual override");
    p.Observe(magic,20000,0);p.Attempt("magic.png",20000);p.Applied("magic.png");battle.Consume(magic);
    battle.Observe(false,p);
    check(p.stable==-1 && p.lastAttempt==20000 && p.lastPhoto=="magic.png","combat style exit resets rule but keeps cooldown/cache");
    check(!p.Observe(0,21000,0) && p.Observe(0,25000,0) && p.Attempt("town.png",25000),"location returns after combat");
    // An outfit may equip a shield and turn Unarmed into generic Combat.
    // Consuming that resulting classification prevents a feedback wardrobe loop.
    battle.Observe(true,p);battle.Consume(static_cast<int>(Place::Combat));
    check(!battle.Allowed(true,true,4) && battle.Allowed(true,true,melee),"own shield change suppressed; later player weapon change remains possible");
    std::cout<<"PASS combat style classification, debounce, fallback, manual lock, cooldown and restoration\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
