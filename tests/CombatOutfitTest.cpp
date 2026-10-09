#include "AutoOutfitPolicy.h"
#include <iostream>
#include <stdexcept>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* message) {if(!v) throw std::runtime_error(message);}
int main() {
 try {
    const int combat=static_cast<int>(Place::Combat);
    const Slots slots{"default.png","town.png","home.png","inn.png","battle.png"};
    check(Choose(Place::Combat,slots,true)=="battle.png","combat overrides location");
    auto empty=slots; empty[4].clear();
    check(Choose(Place::Combat,empty,true).empty(),"no location fallback while fighting");
    Policy p; CombatSession c;
    check(!c.active && !c.attempted,"new session defaults");
    check(!c.Allowed(true,true),"combat toggle does not affect peace");
    p.Observe(1,1000,0);p.Attempt("town.png",1000);p.Applied("town.png");
    p.Manual(1);
    check(c.Observe(true,p),"combat edge");
    check(!c.Allowed(false,true) && !c.Allowed(true,false),"explicit opt-in and assignment required");
    p.Block();check(p.manual==1,"disabled combat retains location manual override");
    check(c.Allowed(true,true),"enabled battle");
    check(!p.Observe(combat,2000,0),"global cooldown applies to combat");
    check(p.Observe(combat,6000,0),"battle overrides manual location outfit");
    c.Consume();check(p.Attempt("battle.png",6000),"battle attempt");p.Applied("battle.png");
    check(!c.Allowed(true,true),"one attempt per combat");
    p.Block();check(!c.Observe(true,p) && !c.Allowed(true,true),"temporary blocker never rearms battle");
    p={};check(!c.Allowed(true,true),"configuration reset cannot rearm same combat");
    // Restore a representative consumed-combat state after testing the configuration reset.
    p.Observe(combat,6000,0);p.Attempt("battle.png",6000);p.Applied("battle.png");
    check(c.Observe(false,p),"combat exit observed even while a menu blocks changes");
    check(p.lastAttempt==6000,"exit preserves cooldown");
    check(!p.Observe(1,7000,0) && p.Observe(1,11000,0),"location restoration waits for cooldown");
    check(p.Attempt("town.png",11000),"restore town");p.Applied("town.png");
    check(!p.Observe(1,21000,0),"restored location not repeatedly applied");
    check(c.Observe(true,p) && c.Allowed(true,true),"new battle can apply");
    check(p.Observe(combat,22000,0),"new battle ready"); c.Consume();
    check(p.Attempt("invalid.png",22000),"invalid set consumes attempt");
    p.Block();check(!c.Allowed(true,true),"failed battle is not retried");
    // A battle that ends in a blocked state still separates the next battle.
    c.Observe(false,p);p.Block();c.Observe(true,p);
    check(c.Allowed(true,true) && p.Observe(combat,28000,0),"new battle after blocked exit is independent");
    // Short combat that never reaches its delay must not equip after it ends.
    p={};c={};p.Observe(2,1000,0);p.Attempt("home.png",1000);p.Applied("home.png");
    c.Observe(true,p);check(!p.Observe(combat,7000,3000),"combat debounce starts");
    c.Observe(false,p);check(!p.Observe(2,8000,0),"short battle leaves handled home unchanged");
    // Shared outfit is consumed without a redundant equip, and next condition is correct.
    p={};c={};p.Observe(0,1000,0);p.Attempt("same.png",1000);p.Applied("same.png");
    c.Observe(true,p);check(p.Observe(combat,7000,0),"shared set condition ready");c.Consume();
    check(!p.Attempt("same.png",7000) && !c.Allowed(true,true),"shared set consumes battle without applying");
    c.Observe(false,p);check(p.Observe(3,12000,0) && p.Attempt("inn.png",12000),"post-combat uses current inn, not old default");
    // Failure after inventory mutation invalidates old cache so restoration is attempted.
    p={};c={};p.Observe(1,1000,0);p.Attempt("town.png",1000);p.Applied("town.png");
    c.Observe(true,p);p.Observe(combat,7000,0);c.Consume();p.Attempt("battle.png",7000);p.lastPhoto.clear();
    c.Observe(false,p);check(p.Observe(1,12000,0) && p.Attempt("town.png",12000),"partial combat failure cannot suppress restore");
    p={};c={};c.Manual(p);
    check(c.active && c.attempted && p.manual==combat,"manual entry consumes a battle even before first auto poll");
    check(!c.Observe(true,p) && !c.Allowed(true,true),"manual combat outfit is not replaced after Gallery closes");
    p.Block();check(!p.Observe(combat,10000,0),"manual survives temporary blockers");
    p={};check(!c.Allowed(true,true),"option changes cannot override manually handled battle");
    c.Observe(false,p);check(p.Observe(1,20000,0),"location resumes after manual combat outfit");
    c.Observe(true,p);check(c.Allowed(true,true),"next battle can use automatic outfit again");
    std::cout<<"PASS: combat opt-in, single attempt, blockers, failure, cooldown, restoration and short battles\n";
    return 0;
 }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
