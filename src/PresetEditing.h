#pragma once
#include "Presets.h"
#include <algorithm>
#include <cctype>
namespace Gallery {
inline bool SameBaseItem(const Item& a,const Item& b) {
    if(a.localID!=b.localID || a.kind!=b.kind || a.plugin.size()!=b.plugin.size()) return false;
    return std::equal(a.plugin.begin(),a.plugin.end(),b.plugin.begin(),[](unsigned char x,unsigned char y){return std::tolower(x)==std::tolower(y);});
}
struct PresetEditMerge { Preset result; std::vector<unsigned> removed,added; };
inline PresetEditMerge MergePresetEquipment(const Preset& original,const std::vector<std::uint32_t>& oldMasks,
 const Preset& worn,const std::vector<std::uint32_t>& wornMasks,const std::vector<unsigned>& selected) {
    if(original.slotMask || original.accessories) throw std::runtime_error("Editing requires a full outfit preset.");
    if(oldMasks.size()!=original.items.size() || wornMasks.size()!=worn.items.size()) throw std::runtime_error("Invalid equipment snapshot.");
    if(selected.empty()) throw std::runtime_error("Select equipment to add or replace.");
    std::uint32_t replaced=0;
    PresetEditMerge merge;merge.result=original;merge.result.items.clear();
    for(auto n:selected) {
        // A resolved armor record may legitimately occupy no biped slots.
        // Missing/changed forms are rejected by ResolvePresetSlots before merging.
        if(n>=worn.items.size() || worn.items[n].kind!="armor") throw std::runtime_error("Equipment slots could not be determined.");
        if(std::any_of(merge.added.begin(),merge.added.end(),[&](auto i){return SameBaseItem(worn.items[i],worn.items[n]);}) || (replaced&wornMasks[n])) throw std::runtime_error("Selected equipment has overlapping slots.");
        replaced|=wornMasks[n];merge.added.push_back(n);
    }
    for(unsigned n=0;n<original.items.size();++n) {
        const bool same=std::any_of(selected.begin(),selected.end(),[&](auto i){return SameBaseItem(original.items[n],worn.items[i]);});
        if((oldMasks[n]&replaced) || same) merge.removed.push_back(n);
        else merge.result.items.push_back(original.items[n]);
    }
    for(auto n:selected)merge.result.items.push_back(worn.items[n]);
    return merge;
}
struct PresetEditFile { std::filesystem::path path; std::string bytes; Preset original; };
PresetEditFile FindPresetForEditing(const std::filesystem::path& folder,const std::string& photo);
struct PresetEditSaved { std::filesystem::path origin; bool equipmentEdited{}; };
PresetEditSaved SaveEditedPreset(const PresetEditFile&,const Preset&);
struct PresetEditOrigin { Preset preset; std::string bytes; bool legacy{}; };
std::optional<PresetEditOrigin> ReadPresetEditOrigin(const std::filesystem::path&,const std::string& photo);
void RememberSelectedEnchanted(RE::Actor*,Preset&,bool remember);
}
