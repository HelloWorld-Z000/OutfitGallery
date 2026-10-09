#include "AutoOutfitPolicy.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* m) {if(!v) throw std::runtime_error(m);}
int main() {
 try {
    Slots slots{"default.png","town.png","home.png","inn.png","combat.png","night.png","rain.png","snow.png"};
    const auto select=[&](Place p,Environment e,bool combat=false){return SelectCondition(p,e,combat,slots);};
    check(!IsNight(19.99f) && IsNight(20.f) && IsNight(23.99f) && IsNight(0.f) && IsNight(5.99f) && !IsNight(6.f),"night boundaries and midnight");
    check(!IsNight(-1.f) && !IsNight(24.f) && !IsNight(std::numeric_limits<float>::quiet_NaN()),"invalid hours not night");
    check(select(Place::Town,{true,true,true,true},true)==Place::Combat,"combat over snow rain night town");
    check(select(Place::Town,{true,true,true,true})==Place::Snow,"overlapping precipitation chooses snow");
    check(select(Place::Town,{true,true,true,false})==Place::Rain,"rain over night town");
    check(select(Place::Normal,{true,true,false,false})==Place::Night,"night over outdoors default");
    check(select(Place::Town,{true,false,false,false})==Place::Town,"clear daytime town");
    check(select(Place::Town,{false,true,true,true})==Place::Town,"all environmental rules ignored indoors");
    check(select(Place::Normal,{false,true,true,true})==Place::Normal,"unclassified interiors use default");
    check(select(Place::Home,{true,true,true,true})==Place::Home,"exterior home retains dedicated rule");
    check(select(Place::Inn,{true,true,true,true})==Place::Inn,"exterior inn retains dedicated rule");
    check(select(Place::Home,{true,true,true,true},true)==Place::Combat,"combat still overrides home");
    slots[7].clear();check(select(Place::Town,{true,true,true,true})==Place::Rain,"empty snow falls to active rain");
    check(select(Place::Town,{true,true,false,true})==Place::Night,"snow does not invent inactive rain");
    slots[6].clear();check(select(Place::Town,{true,true,true,true})==Place::Night,"empty precipitation falls to night");
    slots[5].clear();check(select(Place::Town,{true,true,true,true})==Place::Town,"all new slots empty preserves legacy town");
    slots[1].clear();check(Choose(select(Place::Town,{true,true,true,true}),slots,true)=="default.png","empty town falls to default");
    slots[0].clear();check(ChooseSlot(select(Place::Town,{true,true,true,true}),slots,true)==-1,"no assignment is no change");
    slots[7]="missing-but-assigned.png";check(select(Place::Town,{true,true,true,true})==Place::Snow,"invalid assigned preset must not silently fallback");
    slots={"default.png","town.png","home.png","inn.png","combat.png","night.png","rain.png","snow.png"};
    Policy p;CombatSession c;
    check(p.Observe(6,1000,0) && p.Attempt("rain.png",1000),"rain entry");p.Applied("rain.png");
    check(!p.Observe(6,2000,0),"same rain condition no repeat");
    p.Manual(6);p.Block();check(!p.Observe(6,10000,0),"manual rain outfit survives gallery close");
    check(p.Observe(5,11000,0) && p.Attempt("night.png",11000),"rain ends at night releases manual rule");p.Applied("night.png");
    check(!p.Observe(1,12000,0) && p.Observe(1,16000,0),"entering interior obeys cooldown");
    check(p.Attempt("town.png",16000),"indoor town restored");p.Applied("town.png");
    c.Observe(true,p);check(p.Observe(4,22000,0),"battle entry");c.Consume();p.Attempt("combat.png",22000);p.Applied("combat.png");
    check(!c.Allowed(true,true),"weather changing during combat cannot reapply");
    c.Observe(false,p);check(p.Observe(7,27000,0) && p.Attempt("snow.png",27000),"after combat current snow replaces former town");p.Applied("snow.png");
    p={};check(!p.Observe(6,1000,3000) && !p.Observe(5,2000,3000) && !p.Observe(6,3000,3000),"weather flicker restarts delay");
    check(!p.Observe(6,5999,3000) && p.Observe(6,6000,3000),"stable weather completes delay");
    std::cout<<"PASS: environment priority, outdoor scope, night boundaries, fallbacks, manual and combat transitions\n";return 0;
 }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
