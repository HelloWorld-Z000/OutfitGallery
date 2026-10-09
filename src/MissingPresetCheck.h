#pragma once
#include "Presets.h"
#include <algorithm>
#include <utility>
namespace Gallery {
using MissingPresetResults=std::map<std::string,std::vector<Item>>;
// Checks the registered armor records, not inventory or optional photo files.
// Legacy weapon/ammo entries are not applied and therefore do not flag a set.
template<class Resolver>
MissingPresetResults CheckMissingPresetData(const std::vector<Preset>& presets,Resolver resolve) {
    MissingPresetResults result;
    std::map<std::pair<std::string,std::uint32_t>,bool> resolved;
    for(const auto& p:presets) {
        auto& missing=result[p.photo];
        for(const auto& item:p.items) {
            if(item.kind!="armor") continue;
            auto key=std::make_pair(item.plugin,item.localID);
            auto it=resolved.find(key);
            if(it==resolved.end()) it=resolved.emplace(std::move(key),resolve(item)).first;
            if(!it->second) missing.push_back(item);
        }
    }
    return result;
}
inline bool PresetHasMissingData(const MissingPresetResults& results,const std::string& photo) {
    const auto it=results.find(photo); return it!=results.end() && !it->second.empty();
}
struct MissingPresetCounts {unsigned active{}, trash{};};
template<class IsTrash>
MissingPresetCounts CountMissingPresets(const std::vector<Preset>& presets,const MissingPresetResults& results,IsTrash isTrash) {
    MissingPresetCounts counts;
    for(const auto& p:presets) if(PresetHasMissingData(results,p.photo)) {
        if(isTrash(p.photo)) ++counts.trash; else ++counts.active;
    }
    return counts;
}
inline void MissingPresetsFirst(std::vector<Preset>& cards,const MissingPresetResults& results) {
    std::stable_partition(cards.begin(),cards.end(),[&](const auto& p){return PresetHasMissingData(results,p.photo);});
}
}
