#include "Presets.h"
#include "DeletionPath.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <winioctl.h>

namespace fs=std::filesystem;
static void Require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
// Junctions require no symlink privilege and exercise real filesystem traversal.
static void Junction(const fs::path& link,const fs::path& destination) {
    fs::create_directory(link);
    const std::wstring target=L"\\??\\"+fs::absolute(destination).native();
    struct Buffer {
        DWORD tag; WORD length,reserved;
        WORD substituteOffset,substituteLength,printOffset,printLength;
        wchar_t path[4096];
    } data{};
    Require(target.size()+2<4096,"junction target too long");
    data.tag=IO_REPARSE_TAG_MOUNT_POINT;
    data.substituteLength=static_cast<WORD>(target.size()*sizeof(wchar_t));
    data.printOffset=data.substituteLength+sizeof(wchar_t);
    data.length=8+data.substituteLength+2*sizeof(wchar_t);
    std::copy(target.begin(),target.end(),data.path);
    HANDLE handle=CreateFileW(link.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    Require(handle!=INVALID_HANDLE_VALUE,"could not open junction fixture");
    DWORD returned{};
    const bool ok=DeviceIoControl(handle,FSCTL_SET_REPARSE_POINT,&data,data.length+8,
        nullptr,0,&returned,nullptr)!=0;
    CloseHandle(handle);
    Require(ok,"could not create junction fixture");
}
int main() {
    using namespace Gallery;
    const auto root=fs::temp_directory_path()/("OutfitGalleryTrashTest-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        const auto directory=root/"Data/SKSE/Plugins/OutfitGallery/Presets";
        Require(IsDirectDeletionChild(directory/"portrait-1.json",directory),"virtual child rejected");
        Require(!IsDirectDeletionChild(directory/"../Captures/portrait-1.json",directory),"traversal accepted");
        Require(!IsDirectDeletionChild(directory/"child/portrait-1.json",directory),"nested file accepted");
        Require(!IsDirectDeletionChild(directory/"portrait.json:stream",directory),"ADS accepted");
        Require(!IsDirectDeletionChild(directory/"portrait.json.",directory),"ambiguous Windows name accepted");
        Require(!IsDirectDeletionChild(root/"outside.json",directory),"outside path accepted");
        Require(!IsDirectDeletionChild(root/"PresetsOther/portrait.json",directory),"prefix sibling accepted");
        // Regression: resolved backing paths must not be deletion inputs. The
        // virtual file is authorized above; neither physical mod path is.
        Require(!IsDirectDeletionChild(root/"mods/Output/SKSE/Plugins/OutfitGallery/Presets/portrait.json",directory),"physical mod path accepted as virtual input");
        Require(!IsDirectDeletionChild(root/"overwrite/SKSE/Plugins/OutfitGallery/Presets/portrait.json",directory),"physical overwrite path accepted as virtual input");
        Library library; library.labels["trash.png"].deleted=true;
        Preset preset{"test","trash.png",{{"Test.esp",1,"armor","armor",""}}};
        const auto outside=root/"outside";
        fs::create_directories(outside);
        WritePreset(preset,outside/"record.json");
        {std::ofstream out(outside/"trash.png");out<<"outside must survive";}
        for(const auto* folder:{"Presets","Captures"}) {
            const auto gallery=root/(std::string("junction-")+folder);
            fs::create_directories(gallery);
            const auto link=gallery/folder;
            Junction(link,outside);
            std::vector<std::string> trace;
            bool rejected=false;
            try {DeleteTrashedFiles("trash.png",library,gallery,[&](const auto& line){trace.push_back(line);});}
            catch(const std::exception&){rejected=true;}
            Require(rejected,"junction deletion accepted");
            bool reason=false;
            for(const auto& line:trace) if(line.find("reparse_point")!=std::string::npos && line.find("rejected")!=std::string::npos) reason=true;
            Require(reason,"junction refusal missing diagnostic");
            Require(fs::exists(outside/"record.json") && fs::exists(outside/"trash.png"),"junction target modified");
            // Remove only the junction, never recurse through it.
            Require(RemoveDirectoryW(link.c_str())!=0,"junction cleanup failed");
        }
        // A missing target is a safe no-op; no backing-directory comparison needed.
        const auto missing=root/"missing";
        fs::create_directories(missing);
        DeleteTrashedFiles("trash.png",library,missing);
        fs::remove_all(root);
        std::cout<<"PASS: virtual direct-child policy, traversal/ADS/outside rejection, real Presets/Captures junction protection, absent targets\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
