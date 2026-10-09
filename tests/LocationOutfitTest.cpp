#include "AutoOutfitPolicy.h"
#include <stdexcept>
#include <iostream>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 Slots s{"default","town","home","inn","combat","night","rain","snow","dungeon","temple"};
 check(Classify(false,false,true,true,false)==Place::Dungeon,"dungeon over town");
 check(Classify(false,false,true,true,true)==Place::Temple,"temple over dungeon");
 check(Classify(false,true,true,true,true)==Place::Inn,"inn over temple");
 check(Classify(true,true,true,true,true)==Place::Home,"home over all");
 for(auto p:{Place::Dungeon,Place::Temple}) {
  check(SelectCondition(p,{true,true,true,true},false,s)==p,"dedicated outdoor place over weather");
  check(SelectCondition(p,{false,true,true,true},true,s)==Place::Combat,"combat over dedicated place");
  check(ChooseSlot(p,s,true)==static_cast<int>(p),"assigned place over town");
 }
 s[26]="shop";
 check(Classify(false,false,true,false,false,true,true)==Place::Shop,"shop over town");
 check(Classify(false,true,true,false,false,true,true)==Place::Inn,"inn over shop");
 check(Classify(true,false,true,false,false,true,true)==Place::Home,"home over shop");
 check(Classify(false,false,true,false,false,false,true)==Place::Town,"outdoor store excluded");
 check(ChooseSlot(Place::Shop,s,true)==26,"assigned shop");
 check(SelectCondition(Place::Shop,{},true,s)==Place::Combat,"combat over shop");
 s[22]="sneak"; check(SelectCondition(Place::Shop,{false,false,false,false,false,true},false,s)==Place::Sneaking,"sneak over shop");
 s[26].clear();check(ChooseSlot(Place::Shop,s,true)==1,"empty urban shop falls back town");
 check(ChooseSlot(Place::Shop,s,false)==0,"empty remote shop falls back default");
 s[8].clear();s[9].clear();
 check(ChooseSlot(Place::Dungeon,s,false)==0,"remote dungeon empty goes default");
 check(ChooseSlot(Place::Temple,s,true)==1,"urban temple empty goes town");
 s[1].clear();check(ChooseSlot(Place::Temple,s,true)==0,"empty town goes default");
 s[0].clear();check(ChooseSlot(Place::Dungeon,s,false)==-1,"all empty no change");
 s[8]="missing.png";check(ChooseSlot(Place::Dungeon,s,false)==8,"invalid assigned handled by validation not fallback");
 Policy policy;check(policy.Observe(8,1000,0)&&policy.Attempt("dungeon",1000),"enter dungeon");policy.Applied("dungeon");
 check(!policy.Observe(8,9000,0),"no reapply same category");policy.Manual(8);
 check(!policy.Observe(8,10000,0),"manual stays in dungeon");
 check(policy.Observe(9,11000,0)&&policy.Attempt("temple",11000),"new category releases manual");
 std::cout<<"PASS: dedicated places, precedence, fallbacks, transitions\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}}
