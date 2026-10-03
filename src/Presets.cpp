#include "Presets.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <set>
#include <cmath>
#include <algorithm>
#include <cctype>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
namespace Gallery {
using Json = nlohmann::json;
bool SameEquipment(const Preset& a,const Preset& b) {
    auto keys=[](const Preset& p) {
        std::multiset<std::string> result;
        for(const auto& i:p.items) {auto plugin=i.plugin; std::transform(plugin.begin(),plugin.end(),plugin.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));}); result.insert(plugin+":"+std::to_string(i.localID)+":"+i.kind+":"+i.hand);}
        return result;
    };
    return keys(a)==keys(b);
}
void CheckInput(const InputSettings& s) {
    std::set<unsigned> usedKeys{s.keyboard},usedPads{1,2,4,8,9,10,128,256,512,4096,8192};
    if(s.gamepad) usedPads.insert(s.gamepad);
    for(auto key:s.captureKeys) if(key && (key<2 || key>255 || !usedKeys.insert(key).second)) throw std::runtime_error("Capture key conflicts with another action.");
    for(auto key:s.capturePads) if(key && ((key!=16 && key!=32 && key!=64 && key!=16384 && key!=32768) || !usedPads.insert(key).second)) throw std::runtime_error("Capture button is reserved or already assigned.");
    const std::set<unsigned> buttons{0,1,2,4,8,9,10,16,32,64,128,256,512,4096,8192,16384,32768};
    if(s.keyboard<2 || s.keyboard>255 || !buttons.contains(s.gamepad) || !std::isfinite(s.holdSeconds) || s.holdSeconds<.4f || s.holdSeconds>3.f) throw std::runtime_error("Invalid hotkey settings");
}
InputSettings ReadInputSettings(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>4096) throw std::runtime_error("Hotkey file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1) throw std::runtime_error("Unsupported hotkey schema");
    InputSettings s{j.at("keyboard"),j.at("gamepad"),j.at("holdSeconds")}; s.captureKeys=j.value("captureKeys",s.captureKeys); s.capturePads=j.value("capturePads",s.capturePads); if(!j.contains("capturePads")) for(auto& key:s.capturePads) if(key==s.gamepad) key=0; if(!j.contains("captureKeys")) for(auto& key:s.captureKeys) if(key==s.keyboard) key=0; CheckInput(s); return s;
}
void WriteInputSettings(const InputSettings& s,const std::filesystem::path& path) {
    CheckInput(s);
    Json j={{"schema",1},{"keyboard",s.keyboard},{"gamepad",s.gamepad},{"holdSeconds",s.holdSeconds},{"captureKeys",s.captureKeys},{"capturePads",s.capturePads}};
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not save hotkeys");
}
void ValidateSettings(const StudioSettings& s) {
    if(s.startupTab.size()>240) throw std::runtime_error("Invalid startup tab");
    if(s.headSlots & ~0x1803u) throw std::runtime_error("Invalid head slots");
    if(s.language<0 || s.language>2) throw std::runtime_error("Invalid language");
    auto valid=[](float v,float low,float high){return std::isfinite(v) && v>=low && v<=high;};
    for(auto v:s.previewRect) if(!valid(v,0.f,1.f)) throw std::runtime_error("Invalid preview rectangle");
    if(s.previewRect[2]<.1f || s.previewRect[3]<.1f) throw std::runtime_error("Preview rectangle too small");
    if (!valid(s.distance,10,500) || !valid(s.height,-20,180) || !valid(s.orbit,-180,180) || !valid(s.pitch,-25,25) || !valid(s.lateral,-150,150) || !valid(s.elevation,-60,60) || !valid(s.fov,35,90) || s.columns<2 || s.columns>5)
        throw std::runtime_error("Invalid studio settings");
}
StudioSettings ReadStudioSettings(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path)>8192) throw std::runtime_error("Settings file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1) throw std::runtime_error("Unsupported settings schema");
    StudioSettings s{j.at("distance"),j.at("height"),j.at("orbit"),j.at("pitch"),j.at("fov"),j.value("columns",3)};
    s.showNames=j.value("showNames",true); s.showCounts=j.value("showCounts",true); s.language=j.value("language",0); s.allowFreeCamera=j.value("allowFreeCamera",false); s.addMissing=j.value("addMissing",true); s.preferEnchanted=j.value("preferEnchanted",true);
    s.headSlots=j.value("headSlots",0x1803u);
    s.startupTab=j.value("startupTab",std::string{});
    s.followerTargeting=j.value("followerTargeting",false);
    s.detachedPreview=j.value("detachedPreview",false); s.previewRect=j.value("previewRect",s.previewRect);
    s.lateral=j.value("lateral",0.f); s.elevation=j.value("elevation",0.f);
    ValidateSettings(s); return s;
}
void WriteStudioSettings(const StudioSettings& s,const std::filesystem::path& path) {
    ValidateSettings(s);
    Json j={{"schema",1},{"distance",s.distance},{"height",s.height},{"orbit",s.orbit},{"pitch",s.pitch},{"fov",s.fov},{"columns",s.columns},{"showNames",s.showNames},{"showCounts",s.showCounts},{"language",s.language},{"allowFreeCamera",s.allowFreeCamera},{"addMissing",s.addMissing},{"preferEnchanted",s.preferEnchanted},{"lateral",s.lateral},{"elevation",s.elevation},{"headSlots",s.headSlots},{"startupTab",s.startupTab},{"detachedPreview",s.detachedPreview},{"previewRect",s.previewRect},{"followerTargeting",s.followerTargeting}};
    std::filesystem::create_directories(path.parent_path());
    auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); out.exceptions(std::ios::badbit|std::ios::failbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not replace studio settings file");
}
CameraBank ReadCameraBank(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>16384) throw std::runtime_error("Camera preset file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1 || !j.at("slots").is_array() || j.at("slots").size()!=7) throw std::runtime_error("Invalid camera preset bank");
    CameraBank bank;
    for(std::size_t n=0;n<7;++n) if(!j["slots"][n].is_null()) {
        const auto& c=j["slots"][n]; StudioSettings s;
        s.distance=c.at("distance"); s.height=c.at("height"); s.orbit=c.at("orbit"); s.pitch=c.at("pitch"); s.fov=c.at("fov");
        s.lateral=c.value("lateral",0.f); s.elevation=c.value("elevation",0.f);
        ValidateSettings(s); bank[n]=s;
    }
    return bank;
}
void WriteCameraBank(const CameraBank& bank,const std::filesystem::path& path) {
    Json j={{"schema",1},{"slots",Json::array()}};
    for(const auto& slot:bank) {
        if(!slot) {j["slots"].push_back(nullptr); continue;}
        ValidateSettings(*slot); const auto& s=*slot;
        j["slots"].push_back({{"distance",s.distance},{"height",s.height},{"orbit",s.orbit},{"pitch",s.pitch},{"fov",s.fov},{"lateral",s.lateral},{"elevation",s.elevation}});
    }
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not save camera presets");
}
bool MovePhoto(std::vector<std::string>& order,const std::string& source,const std::string& target,bool after) {
    if(source==target || std::find(order.begin(),order.end(),source)==order.end() || std::find(order.begin(),order.end(),target)==order.end()) return false;
    std::erase(order,source);
    auto at=std::find(order.begin(),order.end(),target);
    order.insert(after?std::next(at):at,source); return true;
}
void CheckLibrary(const Library& l) {
    std::set<std::string> ordered;
    if(l.order.size()>10000) throw std::runtime_error("Too many ordered photos");
    for(const auto& photo:l.order) if(photo.empty() || photo.find_first_of("/\\:")!=std::string::npos || std::filesystem::path(photo).extension()!=".png" || !ordered.insert(photo).second) throw std::runtime_error("Invalid photo order");
    if(l.categories.size()>64 || l.labels.size()>10000) throw std::runtime_error("Library too large");
    std::set<std::string> ids;
    for(const auto& c:l.categories) {
        if(c.id.empty() || c.id.size()>80 || c.name.empty() || c.name.size()>240 || !ids.insert(c.id).second) throw std::runtime_error("Invalid category");
    }
    for(const auto& [photo,label]:l.labels) {
        if(photo.empty() || photo.find_first_of("/\\:")!=std::string::npos || std::filesystem::path(photo).extension()!=".png" || label.name.size()>240 || label.categories.size()>64) throw std::runtime_error("Invalid library entry");
        for(const auto& id:label.categories) if(!ids.contains(id)) throw std::runtime_error("Unknown category");
    }
}
Library ReadLibrary(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>4*1024*1024) throw std::runtime_error("Library file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1) throw std::runtime_error("Unsupported library schema");
    Library l;
    l.order=j.value("order",std::vector<std::string>{});
    for(const auto& c:j.at("categories")) l.categories.push_back({c.at("id"),c.at("name")});
    for(const auto& [key,value]:j.at("labels").items()) l.labels[key]={value.at("name"),value.at("categories").get<std::vector<std::string>>(),value.at("deleted")};
    for(const auto& [key,value]:j.at("labels").items()) if(value.contains("exchangeSlots")) {
        const auto& m=value.at("exchangeSlots");
        if(!m.is_number_integer() || m.get<std::int64_t>()<0 || m.get<std::uint64_t>()>UINT32_MAX)
            throw std::runtime_error("Invalid exchange slots.");
        l.labels[key].exchangeSlots=m.get<std::uint32_t>();
    }
    CheckLibrary(l); return l;
}
void DeleteTrashedFiles(const std::string& photo,const Library& library,const std::filesystem::path& root) {
    CheckLibrary(library);
    const auto label=library.labels.find(photo);
    if(label==library.labels.end() || !label->second.deleted) throw std::runtime_error("Only trashed presets can be deleted");
    const auto base=std::filesystem::weakly_canonical(root);
    std::vector<std::filesystem::path> files;
    auto safe=[&](const auto& path,const char* folder) {
        if(std::filesystem::is_symlink(std::filesystem::symlink_status(path)) || std::filesystem::weakly_canonical(path).parent_path()!=base/folder) throw std::runtime_error("Unsafe deletion path");
    };
    const auto presets=root/"Presets";
    if(std::filesystem::exists(presets)) for(const auto& entry:std::filesystem::directory_iterator(presets)) {
        if(entry.path().extension()!=".json") continue;
        safe(entry.path(),"Presets");
        if(ReadPreset(entry.path()).photo==photo) files.push_back(entry.path());
    }
    const auto image=root/"Captures"/photo;
    safe(image,"Captures");
    files.push_back(image);
    for(const auto& file:files) if(std::filesystem::exists(file) && !std::filesystem::is_regular_file(file)) throw std::runtime_error("Deletion target is not a regular file");
    for(const auto& file:files) std::filesystem::remove(file);
}
void WriteLibrary(const Library& l,const std::filesystem::path& path) {
    CheckLibrary(l);
    Json j={{"schema",1},{"categories",Json::array()},{"labels",Json::object()}};
    j["order"]=l.order;
    for(const auto& c:l.categories) j["categories"].push_back({{"id",c.id},{"name",c.name}});
    for(const auto& [key,value]:l.labels) j["labels"][key]={{"name",value.name},{"categories",value.categories},{"deleted",value.deleted}};
    for(const auto& [key,value]:l.labels) if(value.exchangeSlots) j["labels"][key]["exchangeSlots"]=*value.exchangeSlots;
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not save library");
}
void Validate(const Preset& p) {
    if(p.accessories && !p.slotMask) throw std::runtime_error("Accessories require occupied slots");
    if (p.name.empty() || p.name.size() > 240 || p.items.empty() || p.items.size() > 64)
        throw std::runtime_error("Invalid preset name or item count");
    if (p.photo.empty() || p.photo.find_first_of("/\\:") != std::string::npos || std::filesystem::path(p.photo).extension() != ".png")
        throw std::runtime_error("Invalid preset photo path");
    std::set<std::string> seen;
    for (const auto& i : p.items) {
        if(p.slotMask && i.kind!="armor") throw std::runtime_error("Partial presets support armor slots only");
        if (i.plugin.empty() || i.plugin.size() > 260 || i.plugin.find_first_of("/\\:") != std::string::npos || !i.localID || i.localID > 0xFFFFFF)
            throw std::runtime_error("Invalid plugin or local FormID");
        if ((i.kind != "armor" && i.kind != "weapon" && i.kind != "ammo") || (i.hand != "" && i.hand != "left" && i.hand != "right") || (i.kind != "weapon" && !i.hand.empty()))
            throw std::runtime_error("Unsupported item kind or hand");
        if (!seen.insert(i.plugin + ":" + std::to_string(i.localID) + ":" + i.hand).second)
            throw std::runtime_error("Duplicate preset item");
    }
}
void WritePreset(const Preset& p, const std::filesystem::path& path) {
    Validate(p);
    Json j = {{"schema",p.accessories?3:(p.slotMask?2:1)},{"name",p.name},{"photo",p.photo},{"items",Json::array()}};
    if(p.slotMask) j["slotMask"]=p.slotMask;
    for (const auto& i : p.items) {
        Json item={{"plugin",i.plugin},{"localID",i.localID},{"name",i.name},{"kind",i.kind},{"hand",i.hand}};
        if(i.preferred.custom) item["preferredEnchanted"]={{"scope",i.preferred.scope},{"id",i.preferred.id}};
        if(i.preferred.custom && !i.preferred.signature.empty()) {
            item["preferredEnchanted"]["signatureVersion"]=1;
            item["preferredEnchanted"]["signature"]=i.preferred.signature;
        }
        j["items"].push_back(std::move(item));
    }
    std::filesystem::create_directories(path.parent_path());
    auto tmp = path; tmp += ".tmp";
    { std::ofstream out(tmp, std::ios::binary | std::ios::trunc); out.exceptions(std::ios::failbit | std::ios::badbit); out << j.dump(2); out.close(); }
    std::filesystem::rename(tmp, path);
}
Preset ReadPreset(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 256*1024) throw std::runtime_error("Preset too large");
    std::ifstream in(path, std::ios::binary); Json j; in >> j;
    const int schema=j.at("schema").get<int>();
    if (schema!=1 && schema!=2 && schema!=3) throw std::runtime_error("Unsupported preset schema");
    Preset p{j.at("name").get<std::string>(),j.at("photo").get<std::string>(),{}};
    p.accessories=schema==3;
    if(schema>=2) {
        const auto mask=j.at("slotMask").get<std::int64_t>();
        if(mask<=0 || mask>UINT32_MAX) throw std::runtime_error("Invalid partial preset slots");
        p.slotMask=static_cast<std::uint32_t>(mask);
    }
    for (const auto& i : j.at("items")) {
        const auto id = i.at("localID").get<std::int64_t>();
        if (id <= 0 || id > 0xFFFFFF) throw std::runtime_error("Invalid FormID");
        p.items.push_back({i.at("plugin"),static_cast<std::uint32_t>(id),i.at("name"),i.at("kind"),i.at("hand")});
        if(i.contains("preferredEnchanted")) {
            auto& pref=p.items.back().preferred; pref.custom=true;
            try {
                const auto& m=i.at("preferredEnchanted");
                if(!m.at("scope").is_number_unsigned() || !m.at("id").is_number_unsigned()) continue;
                const auto uid=m.at("id").get<std::uint64_t>();
                if(uid>65535) continue;
                pref.scope=m.at("scope").get<std::uint64_t>(); pref.id=static_cast<std::uint16_t>(uid);
                if(m.contains("signatureVersion") && m["signatureVersion"]==1 && m.contains("signature") && m["signature"].is_string()) {
                    const auto& signature=m["signature"].get_ref<const std::string&>();
                    if(signature.size()<=16384) pref.signature=signature;
                }
            } catch(const Json::exception&) {pref.scope=0; pref.id=0;}
        }
    }
    Validate(p); return p;
}
}



