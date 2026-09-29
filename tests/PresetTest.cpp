#include "Presets.h"
#include "HoldButton.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <stdexcept>
int main() {
    using namespace Gallery;
    auto folder = std::filesystem::temp_directory_path() / ("OutfitGalleryTest-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        Preset p{"白いドレス","portrait-1.png",{{"Dress.esp",0xABC,"ドレス","armor",""},{"Sword.esl",0x801,"Sword","weapon","left"},{"Sword.esl",0x801,"Sword","weapon","right"}}};
        WritePreset(p,folder/"valid.json");
        auto q=ReadPreset(folder/"valid.json");
        if(q.name!=p.name || q.items.size()!=3 || q.items[0].localID!=0xABC || q.items[1].hand!="left" || q.items[2].plugin!="Sword.esl") throw std::runtime_error("round trip failed");
        auto rejects=[&](Preset bad) { try { WritePreset(bad,folder/"bad.json"); } catch(const std::exception&) { return; } throw std::runtime_error("accepted invalid preset"); };
        if(q.slotMask) throw std::runtime_error("Legacy outfit became partial");
        if(q.accessories) throw std::runtime_error("Legacy outfit became accessory set");
        Preset accessories{"Ring and necklace","accessories.png",{{"Jewelry.esp",0x123,"Ring","armor",""},{"Jewelry.esp",0x124,"Necklace","armor",""}},0x60,true};
        WritePreset(accessories,folder/"accessories.json");
        const auto accessoryCopy=ReadPreset(folder/"accessories.json");
        if(!accessoryCopy.accessories || accessoryCopy.slotMask!=0x60 || !SameEquipment(accessories,accessoryCopy)) throw std::runtime_error("Accessory set roundtrip failed");
        for(const auto& category:std::vector<std::string>{"","favorites","#head","#legacy"}) if(CollectionFits(2,category,true)) throw std::runtime_error("Accessory photo leaked to other collections");
        if(!CollectionFits(2,"#accessories",true) || !CollectionFits(2,"#trash",true) || CollectionFits(2,"#accessories",false)) throw std::runtime_error("Accessory isolation failed");
        auto invalidAccessories=accessories; invalidAccessories.slotMask=0; rejects(invalidAccessories);
        invalidAccessories=accessories; invalidAccessories.items[0].kind="weapon"; rejects(invalidAccessories);
        // A ring overlaps the target; body armor sharing its slot must block
        // the operation, whereas ordinary body armor stays outside the scope.
        if(!SlotFits(0x40,0x60) || !SlotIntersects(0x44,0x60) || SlotFits(0x44,0x60) || SlotIntersects(4,0x60)) throw std::runtime_error("Accessory conflict protection failed");
        if(CollectionFits(0x1803,"") || CollectionFits(2,"favorites") || CollectionFits(0,"#head") || CollectionFits(4,"#head") || !CollectionFits(2,"#head") || !CollectionFits(4,"#legacy") || !CollectionFits(2,"#trash")) throw std::runtime_error("Independent head collection leaked");
        Preset part{"Head","head.png",{{"Hair.esp",0x123,"Hair","armor",""}},0x1803};
        WritePreset(part,folder/"partial.json");
        if(ReadPreset(folder/"partial.json").slotMask!=0x1803) throw std::runtime_error("Partial scope lost");
        part.slotMask=0x80000000u; WritePreset(part,folder/"slot61.json");
        if(ReadPreset(folder/"slot61.json").slotMask!=0x80000000u) throw std::runtime_error("High slot bit lost");
        auto invalidPart=p; invalidPart.slotMask=0x1803; rejects(invalidPart);
        if(!SlotIntersects(0x1802,0x1803) || !SlotFits(0x1802,0x1803) || SlotIntersects(4,0x1803) || SlotFits(6,0x1803) || SlotFits(0,0x1803)) throw std::runtime_error("Partial scope affects outside/body slots");
        {std::ofstream out(folder/"empty-scope.json"); out<<R"({"schema":2,"name":"bad","photo":"bad.png","slotMask":0,"items":[]})";}
        bool invalidScope=false; try{ReadPreset(folder/"empty-scope.json");}catch(const std::exception&){invalidScope=true;}
        if(!invalidScope) throw std::runtime_error("Empty partial scope accepted as full outfit");
        auto bad=p; bad.photo="../outside.png"; rejects(bad);
        bad=p; bad.items[0].localID=0xFE123ABC; rejects(bad);
        bad=p; bad.items.push_back(bad.items[0]); rejects(bad);
        bad=p; bad.items[0].hand="left"; rejects(bad);
        {std::ofstream out(folder/"broken.json"); out << "{broken";}
        bool caught=false; try { ReadPreset(folder/"broken.json"); } catch(const std::exception&) { caught=true; }
        if(!caught) throw std::runtime_error("accepted broken JSON");
        StudioSettings settings{219.916f,64.189f,-6.545f,.350f,60.846f,4};
        settings.startupTab="custom-7";
        WriteStudioSettings(settings,folder/"opening-settings.json");
        if(ReadStudioSettings(folder/"opening-settings.json").startupTab!="custom-7") throw std::runtime_error("Opening tab not persisted");
        Library openingLibrary; openingLibrary.categories={{"favorites","Favorites"},{"custom-7","Renamed category"}};
        if(ResolveStartupTab("custom-7",openingLibrary)!="custom-7") throw std::runtime_error("Rename changed opening tab identity");
        if(ResolveStartupTab("deleted-category",openingLibrary)!="favorites") throw std::runtime_error("Missing opening tab fallback failed");
        if(ResolveStartupTab("",openingLibrary)!="" || ResolveStartupTab("#head",openingLibrary)!="#head" || ResolveStartupTab("#accessories",openingLibrary)!="#accessories") throw std::runtime_error("Built-in opening tab failed");
        openingLibrary.categories.clear();
        if(ResolveStartupTab("deleted-category",openingLibrary)!="") throw std::runtime_error("All fallback failed");
        {std::ofstream oldSettings(folder/"old-opening.json"); oldSettings << R"({"schema":1,"distance":220,"height":65,"orbit":0,"pitch":0,"fov":60})";}
        if(!StudioSettings{}.startupTab.empty() || !ReadStudioSettings(folder/"old-opening.json").startupTab.empty()) throw std::runtime_error("Unset opening tab must default to All");
        WriteStudioSettings(settings,folder/"settings.json");
        settings.headSlots=2;
        WriteStudioSettings(settings,folder/"head-settings.json");
        if(ReadStudioSettings(folder/"head-settings.json").headSlots!=2) throw std::runtime_error("Head slot preference lost");
        settings.columns=5; settings.distance=300; settings.showNames=false; settings.showCounts=false;
        WriteStudioSettings(settings,folder/"settings.json");
        const auto restored=ReadStudioSettings(folder/"settings.json");
        if(restored.distance!=300 || restored.height!=settings.height || restored.orbit!=settings.orbit || restored.pitch!=settings.pitch || restored.fov!=settings.fov || restored.columns!=5) throw std::runtime_error("settings replacement/roundtrip failed");
        caught=false; settings.fov=0;
        try { WriteStudioSettings(settings,folder/"settings.json"); } catch(const std::exception&) { caught=true; }
        if(!caught || ReadStudioSettings(folder/"settings.json").fov!=restored.fov) throw std::runtime_error("invalid settings overwrote good settings");
        if(restored.showNames || restored.showCounts) throw std::runtime_error("display preferences lost");
        Library lib{{{"one","普段着"},{"two","Favorites"}},{{"portrait-1.png",{"新しい名前",{"one","two"},false}}}};
        WriteLibrary(lib,folder/"library.json");
        lib.categories[0].name="Renamed tab"; lib.labels["portrait-1.png"].deleted=true;
        WriteLibrary(lib,folder/"library.json");
        auto read=ReadLibrary(folder/"library.json");
        std::vector<std::string> order{"a.png","b.png","c.png","d.png"};
        if(!MovePhoto(order,"a.png","c.png",true) || order!=std::vector<std::string>{"b.png","c.png","a.png","d.png"}) throw std::runtime_error("forward photo move failed");
        if(!MovePhoto(order,"d.png","b.png",false) || order!=std::vector<std::string>{"d.png","b.png","c.png","a.png"}) throw std::runtime_error("backward photo move failed");
        if(MovePhoto(order,"d.png","d.png",true) || MovePhoto(order,"missing.png","b.png",false)) throw std::runtime_error("invalid photo move accepted");
        read.order=order; WriteLibrary(read,folder/"library.json");
        if(ReadLibrary(folder/"library.json").order!=order) throw std::runtime_error("photo order persistence failed");
        auto duplicateOrder=read; duplicateOrder.order.push_back("a.png"); caught=false;
        try {WriteLibrary(duplicateOrder,folder/"library.json");}catch(const std::exception&){caught=true;}
        if(!caught || ReadLibrary(folder/"library.json").order!=order) throw std::runtime_error("invalid order overwrote library");
        if(read.categories[0].name!="Renamed tab" || read.labels.at("portrait-1.png").categories.size()!=2 || !read.labels.at("portrait-1.png").deleted || read.labels.at("portrait-1.png").name!="新しい名前") throw std::runtime_error("library rename/trash roundtrip failed");
        read.labels["portrait-1.png"].deleted=false; WriteLibrary(read,folder/"library.json");
        if(ReadLibrary(folder/"library.json").labels.at("portrait-1.png").deleted) throw std::runtime_error("trash restore failed");
        auto invalid=read; invalid.labels["portrait-1.png"].categories.push_back("missing"); caught=false;
        try {WriteLibrary(invalid,folder/"library.json");}catch(const std::exception&){caught=true;}
        if(!caught || ReadLibrary(folder/"library.json").labels.at("portrait-1.png").categories.size()!=2) throw std::runtime_error("invalid library overwrote valid data");
        WriteInputSettings({65,128,1.2f},folder/"hotkeys.json");
        auto keys=ReadInputSettings(folder/"hotkeys.json");
        if(keys.keyboard!=65 || keys.gamepad!=128 || keys.holdSeconds!=1.2f) throw std::runtime_error("hotkeys roundtrip failed");
        WriteInputSettings({66,0,.8f},folder/"hotkeys.json");
        if(ReadInputSettings(folder/"hotkeys.json").gamepad!=0) throw std::runtime_error("disable pad failed");
        InputSettings customKeys; customKeys.captureKeys={88,0,89};customKeys.capturePads={64,32768,16384};
        WriteInputSettings(customKeys,folder/"capture-keys.json");
        const auto keysAgain=ReadInputSettings(folder/"capture-keys.json");
        if(keysAgain.captureKeys!=customKeys.captureKeys || keysAgain.capturePads!=customKeys.capturePads) throw std::runtime_error("Capture bindings lost");
        customKeys.capturePads[0]=9; bool reservedRejected=false;
        try{WriteInputSettings(customKeys,folder/"capture-keys.json");}catch(const std::exception&){reservedRejected=true;}
        if(!reservedRejected) throw std::runtime_error("Navigation capture conflict allowed");
        {std::ofstream out(folder/"old-keys.json");out<<R"({"schema":1,"keyboard":66,"gamepad":64,"holdSeconds":0.8})";}
        if(ReadInputSettings(folder/"old-keys.json").capturePads[2]!=0) throw std::runtime_error("Old menu binding migration conflict");
        auto closeCamera=StudioSettings{};closeCamera.distance=10;WriteStudioSettings(closeCamera,folder/"close-camera.json");
        if(ReadStudioSettings(folder/"close-camera.json").distance!=10) throw std::runtime_error("Close camera not persisted");
        settings.fov=60; settings.language=0; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").language!=0) throw std::runtime_error("English preference lost");
        settings.language=1; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").language!=1) throw std::runtime_error("Japanese preference lost");
        settings.allowFreeCamera=true; WriteStudioSettings(settings,folder/"settings.json");
        if(!ReadStudioSettings(folder/"settings.json").allowFreeCamera) throw std::runtime_error("free camera preference lost");
        settings.addMissing=true; WriteStudioSettings(settings,folder/"settings.json");
        if(!ReadStudioSettings(folder/"settings.json").addMissing) throw std::runtime_error("add missing enabled preference lost");
        settings.addMissing=false; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").addMissing) throw std::runtime_error("add missing disabled preference lost");
        {std::ofstream out(folder/"legacy-settings.json"); out << R"({"schema":1,"distance":220,"height":65,"orbit":0,"pitch":0,"fov":60})";}
        if(ReadStudioSettings(folder/"legacy-settings.json").addMissing) throw std::runtime_error("legacy settings unexpectedly enable item creation");
        CameraBank bank{}; bank[0]=settings; bank[6]=settings; bank[6]->height=140;
        settings.lateral=42; settings.elevation=-35;
        WriteStudioSettings(settings,folder/"settings.json");
        const auto shifted=ReadStudioSettings(folder/"settings.json");
        if(shifted.lateral!=42 || shifted.elevation!=-35) throw std::runtime_error("camera movement settings lost");
        const auto legacyCamera=ReadStudioSettings(folder/"legacy-settings.json");
        if(legacyCamera.lateral!=0 || legacyCamera.elevation!=0) throw std::runtime_error("legacy composition changed");
        bank[6]->lateral=-55; bank[6]->elevation=40;
        WriteCameraBank(bank,folder/"camera.json");
        auto cameras=ReadCameraBank(folder/"camera.json");
        if(cameras[6]->lateral!=-55 || cameras[6]->elevation!=40) throw std::runtime_error("camera movement preset lost");
        if(!cameras[0] || cameras[1] || !cameras[6] || cameras[6]->height!=140 || cameras[0]->height!=settings.height) throw std::runtime_error("camera slots roundtrip failed");
        cameras[0]->distance=350; WriteCameraBank(cameras,folder/"camera.json");
        auto changed=ReadCameraBank(folder/"camera.json");
        if(changed[0]->distance!=350 || changed[6]->height!=140 || changed[3]) throw std::runtime_error("camera slot replacement changed other slots");
        cameras[6]->fov=0; caught=false;
        try {WriteCameraBank(cameras,folder/"camera.json");}catch(const std::exception&){caught=true;}
        if(!caught || ReadCameraBank(folder/"camera.json")[6]->fov!=settings.fov) throw std::runtime_error("invalid camera overwrote valid bank");
        auto independent=ReadPreset(folder/"valid.json");
        independent.name="A different display name";
        std::swap(independent.items[0],independent.items[2]);
        if(!SameEquipment(p,independent)) throw std::runtime_error("equipment comparison depends on name or order");
        independent.items[0].hand="left";
        if(SameEquipment(p,independent)) throw std::runtime_error("equipment comparison missed wrong hand");
        HoldButton hold;
        auto deletionRoot=folder/"deletion";
        auto trashPreset=p; trashPreset.photo="trash.png";
        WritePreset(trashPreset,deletionRoot/"Presets"/"record.json");
        std::filesystem::create_directories(deletionRoot/"Captures");
        {std::ofstream out(deletionRoot/"Captures"/"trash.png"); out<<"image";}
        auto keepPreset=p; keepPreset.photo="keep.png";
        WritePreset(keepPreset,deletionRoot/"Presets"/"keep.json");
        {std::ofstream out(deletionRoot/"Captures"/"keep.png"); out<<"keep";}
        Library trashLib; trashLib.labels["trash.png"].deleted=false;
        caught=false; try {DeleteTrashedFiles("trash.png",trashLib,deletionRoot);}catch(const std::exception&){caught=true;}
        if(!caught || !std::filesystem::exists(deletionRoot/"Presets"/"record.json")) throw std::runtime_error("non-trash deletion permitted");
        trashLib.labels["trash.png"].deleted=true;
        DeleteTrashedFiles("trash.png",trashLib,deletionRoot);
        if(!std::filesystem::exists(deletionRoot/"Presets"/"keep.json") || !std::filesystem::exists(deletionRoot/"Captures"/"keep.png")) throw std::runtime_error("unrelated files removed");
        if(std::filesystem::exists(deletionRoot/"Captures"/"trash.png") || std::filesystem::exists(deletionRoot/"Presets"/"record.json")) throw std::runtime_error("trash files not removed");
        trashLib.labels["../outside.png"].deleted=true; caught=false;
        try {DeleteTrashedFiles("../outside.png",trashLib,deletionRoot);}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("trash traversal accepted");
        if(hold.Update(true,true,0,.8f)||hold.Update(false,true,.3f,.8f)||hold.Update(false,false,.3f,.8f)) throw std::runtime_error("short press fired");
        if(hold.Update(true,true,0,.8f)||!hold.Update(false,true,.9f,.8f)||hold.Update(false,true,1.5f,.8f)||hold.Update(false,false,1.5f,.8f)) throw std::runtime_error("hold did not fire exactly once");
        if(hold.Update(true,true,0,.8f)||!hold.Update(false,true,.9f,.8f)) throw std::runtime_error("second hold failed");
        hold={}; if(hold.Update(false,true,1.f,.8f)) throw std::runtime_error("unarmed hold fired after rebind");
        std::filesystem::remove_all(folder);
        std::cout << "PASS: UTF-8 roundtrip, local IDs, dual-wield entries, traversal/ID/duplicate/hand rejection, broken JSON\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
