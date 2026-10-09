#include "GuildLocationRules.h"
#include <stdexcept>
#include <iostream>
#include <set>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
struct Loc {Loc* parent{};Place place=Place::Normal;};
int main(){try{
 Slots slots{};for(unsigned i=0;i<slots.size();++i)slots[i]=std::to_string(i);
 const Environment dry{true,true,true,true,false}, wet{true,true,true,true,true};
 check(SelectCondition(Place::Normal,dry,false,slots)==Place::Snow,"water-edge walking preserves weather");
 check(SelectCondition(Place::Normal,wet,false,slots)==Place::Swimming,"swim beats all weather");
 check(SelectCondition(Place::Home,{false,false,false,false,true},false,slots,Place::Thieves)==Place::Swimming,"swim beats home and guild");
 check(SelectCondition(Place::Normal,wet,true,slots)==Place::Combat,"battle beats swimming");
 check(SwimmingChangeAllowed(true,Place::Swimming,slots),"assigned swim can apply in water");
 check(SwimmingChangeAllowed(true,Place::Combat,slots),"opted-in swimmer can use enabled battle outfit");
 check(!SwimmingChangeAllowed(true,Place::Snow,slots),"water gate not opened for unrelated rules");
 slots[10].clear();
 check(SelectCondition(Place::Normal,wet,false,slots)==Place::Snow,"unset swim keeps old rule choice");
 check(!SwimmingChangeAllowed(true,Place::Combat,slots),"without swim assignment old in-water block remains");
 check(SwimmingChangeAllowed(false,Place::Snow,slots),"dry land unaffected");slots[10]="swim";
 for(auto guild:{Place::Dawnguard,Place::College,Place::Brotherhood,Place::Companions,Place::Bards,Place::Thieves,Place::BluePalace}) {
  check(SelectCondition(Place::Home,{false,true,true,true},false,slots,guild)==guild,"assigned guild beats ordinary indoors");
  check(SelectCondition(Place::Town,dry,false,slots,guild)==Place::Snow,"guild never applies outdoors");
  check(SelectCondition(Place::Town,{},true,slots,guild)==Place::Combat,"combat beats every guild");
  check(ChooseSlot(guild,slots,true)==static_cast<int>(guild),"correct dedicated assignment");
  auto old=slots[static_cast<unsigned>(guild)];slots[static_cast<unsigned>(guild)].clear();
  check(SelectCondition(Place::Inn,{},false,slots,guild)==Place::Inn,"unset guild retains inn classification");
  check(SelectCondition(Place::Dungeon,{},false,slots,guild)==Place::Dungeon,"unset guild retains dungeon classification");
  slots[static_cast<unsigned>(guild)]=old;
 }
 Loc guild{nullptr,Place::College}, child{&guild,Place::Normal}, unrelated{};
 const auto parent=[](Loc* l){return l->parent;};const auto match=[](Loc* l){return l->place;};
 check(GuildForLocation(true,&child,parent,match)==Place::College,"indoor descendant recognized");
 check(GuildForLocation(false,&child,parent,match)==Place::Normal,"outdoor approach rejected");
 check(GuildForLocation(true,static_cast<Loc*>(nullptr),parent,match)==Place::Normal,"missing location safe");
 check(GuildForLocation(true,&unrelated,parent,match)==Place::Normal,"unrelated place safe");
 unrelated.parent=&unrelated;check(GuildForLocation(true,&unrelated,parent,match)==Place::Normal,"cyclic ancestry bounded");
 child.place=Place::Bards;check(GuildForLocation(true,&child,parent,match)==Place::Bards,"nearest guild wins");
 std::set<std::pair<std::string,std::uint32_t>> ids;
 for(const auto& r:GuildLocations){check(r.localID && IsGuild(r.place),"valid resolved rule");check(ids.emplace(r.plugin,r.localID).second,"no duplicate form keys");
  check(!(r.plugin=="Skyrim.esm" && (r.localID==0x076F3A || r.localID==0x02BCEB)),"college exterior/root and Midden not claimed");
  check(!(r.plugin=="Dawnguard.esm" && r.localID==0x004C1F),"Dayspring Canyon not claimed");}
 check(ids.count({"Skyrim.esm",0x020086})==1,"Blue Palace has dedicated mapping");
 const auto& palace=GuildLocations.back();
 check(palace.requiredCell==0x016A04 && GuildCellAllowed(palace,true),"palace main interior accepted");
 check(!GuildCellAllowed(palace,false),"shared location in Thalmor HQ or unresolved cell rejected");
 check(GuildCellAllowed(GuildLocations.front(),false),"other guild locations retain ancestry matching");
 check(ids.count({"Skyrim.esm",0x01914A})==0,"quest wing not added as formal palace");
 Policy p;
 check(p.Observe(10,1000,0)&&p.Attempt("swim",1000),"enter water once");p.Applied("swim");
 check(!p.Observe(10,8000,0),"continuous swimming no reapply");
 check(p.Observe(static_cast<int>(SelectCondition(Place::Town,dry,false,slots)),9000,0),"exit uses latest weather");
 check(p.Attempt("snow",9000),"restore snow instead of historical outfit");p.Applied("snow");
 check(!p.Observe(10,10000,0),"shore flicker respects cooldown");
 p.Manual(7);check(!p.Observe(7,15000,0),"manual preference retained");
 check(p.Observe(10,16000,0),"new swimming condition releases manual override");
 std::cout<<"PASS: swimming gates, priority, guild scope, ancestry and water transitions\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}}
