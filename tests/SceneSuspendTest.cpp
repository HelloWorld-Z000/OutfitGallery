#include "SceneSuspendPolicy.h"
#include "AutoOutfitPolicy.h"
#include <stdexcept>
#include <iostream>
using namespace Gallery::AutoOutfit;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 SceneSuspend s;
 check(!SceneBlocks(s.Observe(false,false,false,0,5000)),"absent optional dependency allows play");
 check(!SceneBlocks(s.Observe(true,false,false,0,5000)),"unavailable optional bridge preserves automatic outfits");
 check(s.Observe(true,false,false,500,5000)==SceneState::Unavailable,"unavailable remains visible instead of claiming idle protection");
 check(SceneBlocks(SceneState::Active) && SceneBlocks(SceneState::Recovering),"only participant and recovery gate equipment");
 check(!SceneBlocks(SceneState::Idle) && !SceneBlocks(SceneState::NotInstalled) && !SceneBlocks(SceneState::Unavailable),"ordinary play including unknown integration continues");
 check(s.Observe(true,true,false,1000,5000)==SceneState::Idle,"ready no scene");
 check(s.Observe(true,true,true,2000,5000)==SceneState::Active,"participant blocks");
 check(s.Observe(true,true,true,9000,5000)==SceneState::Active,"long scene blocks");
 check(s.Observe(true,true,false,10000,5000)==SceneState::Recovering,"exit starts hold");
 check(s.Observe(true,true,false,14999,5000)==SceneState::Recovering,"hold before boundary");
 check(s.Observe(true,true,false,15000,5000)==SceneState::Idle,"hold ends once without extending");
 s.Observe(true,true,true,16000,5000);s.Observe(true,true,false,17000,5000);
 check(s.Observe(true,true,true,18000,5000)==SceneState::Active,"new scene overrides hold");
 check(s.Observe(true,true,false,20000,5000)==SceneState::Recovering && s.Observe(true,true,false,24999,5000)==SceneState::Recovering,"new exit gets full hold");
 s={};check(s.Observe(true,true,true,30000,5000)==SceneState::Active,"load midscene recovers without event history");
 Policy p;p.Manual(1);
 s.Observe(true,true,false,31000,5000);p.Block();
 check(s.Observe(true,true,false,36000,5000)==SceneState::Idle && !p.Observe(1,36000,0),"same-condition manual override preserved");
 check(p.Observe(9,37000,0),"changed location becomes eligible after scene");
 p={};p.Observe(1,40000,0);p.Attempt("town",40000);p.Applied("town");p.Block();
 check(!p.Observe(1,50000,0),"scene exit alone does not reassert prior outfit");
 s={};s.Observe(true,true,true,60000,5000);
 check(!SceneBlocks(s.Observe(true,false,false,61000,5000)),"lost support must not leave a scene latch blocking auto outfits");
 check(s.Observe(true,true,false,62000,5000)==SceneState::Idle,"restored support outside scene has no stale recovery");
 check(SceneBlocks(s.Observe(true,true,true,63000,5000)),"later confirmed scene still suspends");
 check(SceneBlocks(s.Observe(true,true,false,64000,5000)),"confirmed end retains five second recovery");
 check(!SceneBlocks(s.Observe(true,true,false,69000,5000)),"confirmed recovery ends at boundary");
 s={};s.Observe(true,true,true,100000);
 check(s.Observe(true,true,false,101000)==SceneState::Recovering,"default hold starts");
 check(s.Observe(true,true,false,110999)==SceneState::Recovering,"default ten seconds not elapsed");
 check(s.Observe(true,true,false,111000)==SceneState::Idle,"default ten second boundary");
 s={};s.Observe(true,true,true,200000,30000);s.Observe(true,true,false,201000,30000);
 check(s.Observe(true,true,false,220000,5000)==SceneState::Recovering,"setting change does not shorten current hold");
 check(s.Observe(true,true,false,231000,30000)==SceneState::Idle,"custom thirty second boundary");
 std::cout<<"PASS: optional dependency, scene lifecycle, recovery, manual priority\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}}
