#include "CraftSession.h"
#include "AutoOutfitPolicy.h"
#include <iostream>
#include <stdexcept>
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(){try{
    Gallery::Craft::Session s;
    check(s.Begin(123,1000,5),"begin");
    check(!s.Begin(456,1001,5),"another activation cannot replace active furniture");
    check(!s.Try(false,false,5,true) && !s.attempted,"do not equip until player actually uses furniture");
    check(!s.Finished(2000,true,false,false),"enter");
    check(s.Try(true,false,5,true),"pre-menu window");
    check(!s.Try(true,false,5,true),"duplicate enter and activation only equip once");
    s.changed=true;s.exited=true;
    check(!s.Finished(3000,false,true,true),"menu still open prevents restoration");
    check(!s.Finished(3000,false,false,false),"exit animation prevents restoration");
    check(s.Finished(4000,false,true,false),"normal play permits restoration");
    check(s.Finished(4000,true,true,false),"explicit exit defeats retained furniture handle");
    s={};s.Begin(123,0,0);
    check(!s.Try(true,true,1,true) && s.attempted,"late attempt skipped");
    check(!s.Try(true,false,1,true),"no retry after menu closes");
    s={};s.Begin(123,0,0);
    check(!s.Try(true,false,1,true),"opened then closed menu still blocks");
    s={};s.Begin(123,1000,0);
    check(!s.Try(true,false,0,false) && !s.Try(true,false,0,true),"paused/combat/missing assignment skipped for session");
    s={};s.Begin(123,1000,0);
    check(!s.Finished(10999,false,true,false) && s.Finished(11000,false,true,false),"cancelled interaction expires");
    s={};s.Begin(123,1000,0);
    check(!s.Finished(999999,true,false,false),"long crafting remains held");
    s={};check(!s.Try(true,false,0,true),"reset/load disarms session");
    using namespace Gallery::AutoOutfit;
    Slots slots;slots[0]="normal";slots[23]="craft";
    check(!HasCombatStyles(slots),"craft does not enable battle styles");
    check(Choose(Place::Smithing,slots,true)=="craft","dedicated slot");
    check(SelectCondition(Place::Town,{},false,slots)==Place::Town,"walking near furniture does not select craft");
    slots[23].clear();check(Choose(Place::Smithing,slots,true).empty(),"empty craft slot never falls back");
    using Gallery::Craft::Kind;
    check(Gallery::Craft::SlotFor(Kind::Smithing)==static_cast<int>(Place::Smithing),"smithing index preserved");
    check(Gallery::Craft::SlotFor(Kind::Alchemy)==static_cast<int>(Place::Alchemy),"alchemy index matches policy");
    slots[23]="smith";slots[24]="alchemy";
    check(Choose(Place::Smithing,slots,true)=="smith" && Choose(Place::Alchemy,slots,true)=="alchemy","stations use independent assignments");
    slots[24].clear();check(Choose(Place::Alchemy,slots,true).empty(),"unassigned alchemy never inherits smithing or normal");
    s={};check(!s.Begin(123,0,0,Kind::None),"unsupported station cannot arm session");
    check(s.Begin(123,0,0,Kind::Alchemy),"alchemy entry");
    check(!s.Begin(456,1,0,Kind::Smithing) && s.kind==Kind::Alchemy,"other furniture cannot replace active alchemy kind");
    check(s.Try(true,false,0,true) && !s.Try(true,false,0,true),"alchemy also attempts exactly once");
    s.changed=true;s.exited=true;
    check(!s.Finished(100,true,true,true) && s.Finished(200,true,true,false),"alchemy restores only after menu and furniture exit");
    s={};check(s.Begin(456,300,1,Kind::Smithing) && s.Try(true,false,1,true),"next smithing visit rearms independently");
    s={};s.Begin(123,0,0,Kind::Alchemy);
    check(!s.Try(true,true,1,true) && !s.Try(true,false,1,true),"late alchemy never retries during same visit");
    check(Gallery::Craft::SlotFor(Kind::Enchanting)==static_cast<int>(Place::Enchanting),"enchanting index matches policy");
    slots[24]="alchemy";slots[25]="enchant";
    check(Choose(Place::Enchanting,slots,true)=="enchant" && Choose(Place::Alchemy,slots,true)=="alchemy" && Choose(Place::Smithing,slots,true)=="smith","three independent craft assignments");
    slots[25].clear();check(Choose(Place::Enchanting,slots,true).empty(),"empty enchanting never falls back");
    s={};check(s.Begin(789,0,0,Kind::Enchanting),"enchanting session arms");
    check(!s.Begin(123,1,0,Kind::Alchemy) && s.kind==Kind::Enchanting,"active enchanting kind retained");
    check(!s.Try(false,false,0,true) && s.Try(true,false,0,true) && !s.Try(true,false,0,true),"enchanting waits for furniture then equips once");
    s.exited=true;
    check(!s.Finished(100,true,true,true) && !s.Finished(101,false,false,false) && s.Finished(102,true,true,false),"enchanting restores only after normal exit");
    s={};s.Begin(789,0,0,Kind::Enchanting);
    check(!s.Try(true,true,1,true) && !s.Try(true,false,1,true),"late enchanting never retries");
    std::cout<<"PASS crafting entry, deadline, deduplication, cancellation and restoration guards\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
