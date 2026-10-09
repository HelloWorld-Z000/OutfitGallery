#include "PresetEditing.h"
#include <fstream>
#include <iostream>
using namespace Gallery;
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
std::string bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}require(rejected,"expected rejection");}
int main(){try{
    Item coat{"clothes.esp",1,"coat","armor",""},gloves{"clothes.esp",2,"gloves","armor",""},ring{"clothes.esp",3,"ring","armor",""};
    Preset original{"Original","same.png",{coat,gloves}};
    Preset worn{"","",{ring,gloves}};worn.items[1].preferred={true,111,12,"new enchantment"};
    auto merged=MergePresetEquipment(original,{4,8},worn,{64,8},{0});
    require(merged.removed.empty() && merged.result.items.size()==3 && merged.result.photo==original.photo && merged.result.name==original.name,"add preserves original and identity");
    merged=MergePresetEquipment(original,{4,8},worn,{64,8},{1});
    require(merged.removed==std::vector<unsigned>{1} && merged.result.items.size()==2 && merged.result.items.back().preferred.signature=="new enchantment","same base re-enchantment updates metadata");
    merged=MergePresetEquipment(original,{12,8},worn,{64,8},{1});
    require(merged.removed.size()==2 && merged.result.items.size()==1,"multislot replacement removes whole conflicting items");
    rejects([&]{MergePresetEquipment(original,{4,8},worn,{8,8},{0,1});});
    rejects([&]{MergePresetEquipment(original,{4,8},worn,{64,8},{});});
    auto slotless=MergePresetEquipment(original,{4,8},worn,{0,8},{0});
    require(slotless.removed.empty() && slotless.result.items.size()==3,"slotless addition preserves clothes");
    auto withRing=slotless.result;
    auto otherRing=ring;otherRing.localID=4;otherRing.name="other ring";
    Preset rings{"","",{ring,otherRing}};rings.items[0].preferred={true,77,12,"updated"};
    slotless=MergePresetEquipment(withRing,{4,8,0},rings,{0,0},{0,1});
    require(slotless.removed==std::vector<unsigned>{2} && slotless.result.items.size()==4 && slotless.result.items[2].preferred.signature=="updated","update same slotless item and append different slotless item");
    auto clothesUpdate=MergePresetEquipment(withRing,{4,8,0},worn,{0,8},{1});
    require(clothesUpdate.removed==std::vector<unsigned>{1} && clothesUpdate.result.items[1].localID==ring.localID,"existing slotless item survives clothing replacement");
    rejects([&]{MergePresetEquipment(original,{4,8},rings,{0,0},{0,0});});
    auto duplicates=rings;duplicates.items[1]=duplicates.items[0];
    rejects([&]{MergePresetEquipment(original,{4,8},duplicates,{0,0},{0,1});});
    auto weapon=rings;weapon.items[0].kind="weapon";
    rejects([&]{MergePresetEquipment(original,{4,8},weapon,{0,0},{0});});
    rejects([&]{MergePresetEquipment(original,{4,8},worn,{64,8},{0,0});});
    rejects([&]{MergePresetEquipment(original,{4,8},worn,{64,8},{9});});
    auto partial=original;partial.slotMask=12;rejects([&]{MergePresetEquipment(partial,{4,8},worn,{64,8},{0});});
    const auto folder=std::filesystem::temp_directory_path()/"OutfitGallery-PresetEditingTest";
    std::filesystem::create_directories(folder);const auto path=folder/"custom-file-name.json";
    WritePreset(original,path);auto snapshot=FindPresetForEditing(folder,"same.png");
    require(snapshot.path==path,"resolve by photo, not assumed filename");
    auto result=MergePresetEquipment(original,{4,8},worn,{64,8},{1}).result;
    // A failed first baseline write must not overwrite the registered outfit.
    const auto before=bytes(path);
    {std::ofstream blocker(folder/"EditOriginals");blocker<<"not a directory";}
    rejects([&]{SaveEditedPreset(snapshot,result);});require(bytes(path)==before,"baseline failure prevents overwrite");
    std::filesystem::remove(folder/"EditOriginals");
    const auto saved=SaveEditedPreset(snapshot,result);
    require(saved.equipmentEdited && ReadPreset(path).equipmentEdited && ReadPreset(path).items.back().preferred.signature=="new enchantment","save metadata and persist edited flag");
    require(!std::filesystem::exists(folder/"EditBackups"),"editing creates no history directory");
    const auto after=bytes(path);rejects([&]{SaveEditedPreset(snapshot,original);});require(bytes(path)==after,"stale edit leaves newer file intact");
    snapshot=FindPresetForEditing(folder,"same.png");auto bad=result;bad.photo="different.png";
    rejects([&]{SaveEditedPreset(snapshot,bad);});require(bytes(path)==after,"identity change rejected");
    bad=result;bad.items.clear();rejects([&]{SaveEditedPreset(snapshot,bad);});require(bytes(path)==after,"invalid replacement preserves original");
    WritePreset(original,folder/"duplicate.json");rejects([&]{FindPresetForEditing(folder,"same.png");});
    std::filesystem::remove(folder/"duplicate.json");
    const auto origin=ReadPresetEditOrigin(path,"same.png");require(origin && !origin->legacy && origin->preset.items.size()==2,"first edit baseline saved");
    // Once an origin exists, editing must not inspect even an inaccessible legacy path.
    {std::ofstream blocker(folder/"EditBackups");blocker<<"not a directory";}
    snapshot=FindPresetForEditing(folder,"same.png");
    const auto restored=SaveEditedPreset(snapshot,origin->preset);
    require(!ReadPreset(path).equipmentEdited && !restored.equipmentEdited,"restore clears edited state");
    require(restored.origin==saved.origin,"restore reuses the single baseline");
    require(ReadPresetEditOrigin(path,"same.png")->bytes==origin->bytes,"restore never overwrites baseline");
    for(int n=0;n<12;++n) {
        snapshot=FindPresetForEditing(folder,"same.png");
        SaveEditedPreset(snapshot,n%2?original:result);
    }
    require(std::distance(std::filesystem::directory_iterator(folder/"EditOriginals"),std::filesystem::directory_iterator{})==1,"repeated edits retain one origin only");
    require(bytes(folder/"EditBackups")=="not a directory","legacy history is untouched");
    require(ReadPresetEditOrigin(path,"same.png")->bytes==origin->bytes,"repeated edits never replace origin");
    std::filesystem::remove(folder/"EditBackups");
    std::filesystem::remove(folder/"EditOriginals"/(path.filename().string()+".original"));std::filesystem::remove(folder/"EditOriginals");
    // Older edit-test1 files use the oldest available backup. Numeric time ordering matters.
    std::filesystem::create_directories(folder/"EditBackups");
    WritePreset(original,folder/"EditBackups"/"custom-file-name.json.9-1.bak");
    WritePreset(result,folder/"EditBackups"/"custom-file-name.json.10-1.bak");
    const auto legacy=ReadPresetEditOrigin(path,"same.png");require(legacy && legacy->legacy && legacy->preset.items.back().preferred.signature.empty(),"oldest legacy backup recovered numerically");
    {std::ofstream corrupt(folder/"EditBackups"/"custom-file-name.json.9-1.bak");corrupt<<"broken";}
    rejects([&]{ReadPresetEditOrigin(path,"same.png");});
    WritePreset(original,folder/"EditBackups"/"custom-file-name.json.9-1.bak");
    snapshot=FindPresetForEditing(folder,"same.png");
    const auto imported=SaveEditedPreset(snapshot,result);
    require(ReadPresetEditOrigin(path,"same.png")->legacy,"legacy origin provenance retained");
    require(std::distance(std::filesystem::directory_iterator(folder/"EditBackups"),std::filesystem::directory_iterator{})==2,"migration adds no history files");
    std::filesystem::remove(imported.origin);std::filesystem::remove(folder/"EditOriginals");
    std::filesystem::remove(folder/"EditBackups"/"custom-file-name.json.9-1.bak");std::filesystem::remove(folder/"EditBackups"/"custom-file-name.json.10-1.bak");std::filesystem::remove(folder/"EditBackups");
    std::filesystem::remove(path);std::filesystem::remove(folder);
    std::cout<<"PASS preset merge, metadata, backup, stale edit, ambiguous file and failed save protections\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
