#include "AutoOutfitPolicy.h"
#include <iostream>
#include <stdexcept>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 Slots s;s[0]="normal";s[4]="battle";s[7]="snow";s[10]="water";s[22]="sneak";
 Environment e{true,true,false,true,false,true};
 check(!HasCombatStyles(s),"sneak assignment must not enable combat styles");
 check(SelectCondition(Place::Town,e,false,s)==Place::Sneaking,"sneak beats snow and town");
 check(SelectCondition(Place::Home,e,false,s)==Place::Sneaking,"sneak beats dedicated location");
 check(SelectCondition(Place::Home,e,true,s)==Place::Combat,"battle beats sneak immediately");
 e.swimming=true;check(SelectCondition(Place::Town,e,false,s)==Place::Swimming,"water beats sneak");e.swimming=false;
 s[22].clear();check(SelectCondition(Place::Town,e,false,s)==Place::Snow,"empty sneak keeps weather");s[22]="sneak";
 check(Choose(Place::Sneaking,s,true)=="sneak","select dedicated photo");
 check(ConditionDelay(Place::Sneaking,0)==2000 && ConditionDelay(Place::Sneaking,2000)==2000 && ConditionDelay(Place::Sneaking,5000)==2000,"normal delay does not override sneak default");
 check(ConditionDelay(Place::Combat,0)==0,"sneak delay never delays battle entry");
 check(ConditionDelay(Place::Sneaking,10000,0)==0 && ConditionDelay(Place::Sneaking,0,5000)==5000,"independent 0 and 5 second settings");
 check(ConditionDelay(Place::Town,3000,5000)==3000 && ConditionDelay(Place::Combat,0,5000)==0,"sneak setting does not leak into other conditions");
 Policy instant;check(instant.Observe(static_cast<int>(Place::Sneaking),1000,ConditionDelay(Place::Sneaking,10000,0)),"zero allows first eligible check");
 Policy slow;check(!slow.Observe(static_cast<int>(Place::Sneaking),1000,5000) && !slow.Observe(static_cast<int>(Place::Sneaking),5999,5000) && slow.Observe(static_cast<int>(Place::Sneaking),6000,5000),"five second boundary");
 const int sneak=static_cast<int>(Place::Sneaking),combat=static_cast<int>(Place::Combat);
 Policy p;p.Observe(0,1000,0);p.Attempt("normal",1000);p.Applied("normal");
 check(!p.Observe(sneak,10000,ConditionDelay(Place::Sneaking,0)) && !p.Observe(sneak,11999,ConditionDelay(Place::Sneaking,0)),"brief sneak does not change outfit");
 check(!p.Observe(0,12500,0),"brief sneak exit leaves already-handled normal outfit alone");
 check(!p.Observe(sneak,14000,ConditionDelay(Place::Sneaking,0)) && p.Observe(sneak,16000,ConditionDelay(Place::Sneaking,0)),"new sneak entry restarts full wait");
 p.Attempt("sneak",17000);p.Applied("sneak");
 check(!p.Observe(sneak,25000,ConditionDelay(Place::Sneaking,0)),"no repeat while remaining crouched");
 CombatSession c;c.Observe(true,p);
 check(p.Observe(combat,26000,0),"combat interrupts sneak");p.Attempt("battle",26000);p.Applied("battle");c.Consume();
 c.Observe(false,p);
 check(!p.Observe(sneak,32000,ConditionDelay(Place::Sneaking,0)) && !p.Observe(sneak,33999,ConditionDelay(Place::Sneaking,0)) && p.Observe(sneak,34000,ConditionDelay(Place::Sneaking,0)),"combat exit starts a fresh sneak wait");
 p.Attempt("sneak",35000);p.Applied("sneak");
 check(!p.Observe(0,36000,0) && p.Observe(0,40000,0),"stand up restores current location after cooldown");
 p={};check(!p.Observe(sneak,1000,ConditionDelay(Place::Sneaking,0)),"start pending");p.Block();
 check(!p.Observe(sneak,10000,ConditionDelay(Place::Sneaking,0)),"pause/menu time excluded");
 p.Manual(sneak);check(!p.Observe(sneak,20000,ConditionDelay(Place::Sneaking,0)) && !p.Observe(sneak,24000,ConditionDelay(Place::Sneaking,0)),"manual outfit while sneaking retained");
 check(p.Observe(0,25000,0),"leaving sneak releases manual override");
 std::cout<<"PASS sneak priority, debounce, combat interruption, restoration and manual priority\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
