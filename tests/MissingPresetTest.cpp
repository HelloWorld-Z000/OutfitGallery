#include "MissingPresetCheck.h"
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void Check(bool ok) {if(!ok) throw std::runtime_error("Missing preset check failed");}
int main() {try {
    Item present{"Installed.esp",1,"Present","armor"}, missing{"Gone.esp",2,"Missing","armor"}, wrong{"Changed.esp",3,"Wrong type","armor"}, legacy{"Gone.esp",4,"Ignored weapon","weapon"};
    Preset a,b,c,d; a.photo="a";a.items={present,legacy};b.photo="b";b.items={missing,present};c.photo="c";c.items={missing,wrong};d.photo="d";
    std::vector<Preset> saved{a,b,c,d}; unsigned calls=0;
    const auto results=CheckMissingPresetData(saved,[&](const Item& i){++calls;return i.plugin=="Installed.esp";});
    Check(calls==3 && results.size()==4 && results.at("a").empty() && results.at("b").size()==1 && results.at("c").size()==2 && results.at("d").empty());
    auto view=saved;MissingPresetsFirst(view,results);Check(view[0].photo=="b" && view[1].photo=="c" && view[2].photo=="a" && view[3].photo=="d");
    Check(saved[0].photo=="a" && saved[1].photo=="b");
    Check(!PresetHasMissingData(results,"new") && !PresetHasMissingData(results,"a"));
    auto counts=CountMissingPresets(saved,results,[](const std::string& p){return p=="b";});
    Check(counts.active==1 && counts.trash==1);
    counts=CountMissingPresets(saved,results,[](const std::string&){return false;});
    Check(counts.active==2 && counts.trash==0);
    auto remaining=saved; std::erase_if(remaining,[](const Preset& p){return p.photo=="b";});
    counts=CountMissingPresets(remaining,results,[](const std::string& p){return p=="b";});
    Check(counts.active==1 && counts.trash==0 && calls==3);
    bool failed=false;try {CheckMissingPresetData(saved,[](const Item&)->bool{throw std::runtime_error("unavailable");});}catch(...) {failed=true;}Check(failed);
    std::cout<<"PASS: unique lookups, missing/type mismatch, ignored legacy entries, stable temporary ordering and failed scan\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what(); return 1;}}
