#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <array>
#include <optional>
namespace Gallery {
struct InputSettings { unsigned keyboard{66}, gamepad{32}; float holdSeconds{0.8f}; std::array<unsigned,3> captureKeys{67,68,87}; std::array<unsigned,3> capturePads{32768,16384,64}; };
InputSettings ReadInputSettings(const std::filesystem::path&);
void WriteInputSettings(const InputSettings&,const std::filesystem::path&);
struct StudioSettings { float distance{217.130f}, height{67.619f}, orbit{-22.831f}, pitch{0.350f}, fov{60.060f}; int columns{3}; bool showNames{true}, showCounts{true}; int language{0}; bool allowFreeCamera{}; bool addMissing{}; float lateral{13.929f}, elevation{0.000f}; std::uint32_t headSlots{0x1803}; std::string startupTab{}; };
using CameraBank=std::array<std::optional<StudioSettings>,7>;
CameraBank ReadCameraBank(const std::filesystem::path&);
void WriteCameraBank(const CameraBank&,const std::filesystem::path&);
struct Category { std::string id, name; };
struct PresetLabel { std::string name; std::vector<std::string> categories; bool deleted{}; };
struct Library { std::vector<Category> categories; std::map<std::string,PresetLabel> labels; std::vector<std::string> order; };
inline std::string ResolveStartupTab(const std::string& preferred,const Library& library) {
    if(preferred.empty() || preferred=="#head" || preferred=="#accessories" || preferred=="#trash") return preferred;
    bool hasFavorites=false;
    for(const auto& c:library.categories) {
        if(c.id==preferred) return preferred;
        if(c.id=="favorites") hasFavorites=true;
    }
    return hasFavorites?"favorites":"";
}
bool MovePhoto(std::vector<std::string>& order,const std::string& source,const std::string& target,bool after);
void DeleteTrashedFiles(const std::string& photo,const Library&,const std::filesystem::path& root);
Library ReadLibrary(const std::filesystem::path&);
void WriteLibrary(const Library&,const std::filesystem::path&);
StudioSettings ReadStudioSettings(const std::filesystem::path&);
void WriteStudioSettings(const StudioSettings&, const std::filesystem::path&);
struct Item { std::string plugin; std::uint32_t localID{}; std::string name; std::string kind; std::string hand; };
struct Preset { std::string name; std::string photo; std::vector<Item> items; std::uint32_t slotMask{}; bool accessories{}; };
// Old presets may contain weapons/ammo. Keep their files readable, but never
// apply those entries or include them in post-apply verification.
inline Preset ClothingPreset(const Preset& source) {
    auto result=source;
    result.items.clear();
    for(const auto& item:source.items) if(item.kind=="armor") result.items.push_back(item);
    return result;
}
inline bool SlotIntersects(std::uint32_t item, std::uint32_t scope) {return (item & scope)!=0;}
inline bool SlotFits(std::uint32_t item, std::uint32_t scope) {return item && (item & ~scope)==0;}
inline bool CollectionFits(std::uint32_t scope,const std::string& category,bool accessories=false) {
    if(category=="#trash") return true;
    if(category=="#accessories") return accessories;
    if(accessories) return false;
    if(category=="#head") return SlotFits(scope,0x1803);
    if(category=="#legacy") return scope && !SlotFits(scope,0x1803);
    return scope==0;
}
bool SameEquipment(const Preset&,const Preset&);
void WritePreset(const Preset&, const std::filesystem::path&);
Preset ReadPreset(const std::filesystem::path&);
Preset SnapshotEquipment(std::uint32_t slotMask=0); // game thread only; zero = full outfit
Preset SnapshotAccessories(); // equipped armor only, union of occupied slots
std::string ApplyEquipment(const Preset&, bool addMissing); // game thread only
}



