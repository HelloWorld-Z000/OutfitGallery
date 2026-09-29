#include "Capture.h"
#include "Presets.h"
#include "ManagedItems.h"
#include "HoldButton.h"
#pragma warning(push)
#pragma warning(disable: 4099 5054)
#include "SKSEMenuFramework.h"
#include "SmoothCamAPI.h"
#pragma push_macro("GetModuleHandle")
#undef GetModuleHandle
#define GetModuleHandle GetModuleHandleA
#include "TrueDirectionalMovementAPI.h"
#pragma pop_macro("GetModuleHandle")
#pragma warning(pop)

using namespace REL::literals;
SKSEPluginInfo(
    .Version = "1.0.0.0"_v,
    .Name = "OutfitGallery",
    .Author = "Outfit Gallery contributors",
    .RuntimeCompatibility = { "1.5.97.0"_v, "1.6.353.0"_v, "1.6.640.0"_v, "1.6.1130.0"_v, "1.6.1170.0"_v }
)

namespace Gallery {
namespace UI = ImGuiMCP;
#include "Localization.inc"
SKSEMenuFramework::Model::WindowInterface* window{};
SKSEMenuFramework::Model::WindowInterface* noticeWindow{};
std::atomic<ULONGLONG> noticeUntil{};
SmoothCamAPI::IVSmoothCam3* smooth{};
TDM_API::IVTDM2* tdm{};
std::atomic<bool> active{}, ready{}, pending{}, captureRequested{};
std::atomic<unsigned> command{}, epoch{};
std::atomic<unsigned> studioOpenSerial{};
unsigned captureDelay{}; // render-thread only, lets the last camera edit reach the game
const StudioSettings initialCamera{};
std::string startupTab=initialCamera.startupTab;
std::atomic<float> distance{initialCamera.distance}, height{initialCamera.height}, orbit{initialCamera.orbit}, pitch{initialCamera.pitch}, fov{initialCamera.fov}, lateral{initialCamera.lateral}, elevation{initialCamera.elevation};
bool showNames=true, showCounts=true;
bool addMissing{}, showCameraSettings{};
std::atomic<std::uint32_t> partialSlots{0x1803};
std::optional<Preset> revealAfterSave; // guarded by presetsMutex
int gridColumns=3; // render thread after initialization
std::atomic<bool> allowFreeCamera{};
CameraBank cameraBank;
int cameraSlot{};
bool cameraBankWritable=true;
const std::filesystem::path cameraBankFile="Data/SKSE/Plugins/OutfitGallery/CameraPresets.json";
ULONGLONG settingsChangedAt{};
const std::filesystem::path settingsFile="Data/SKSE/Plugins/OutfitGallery/StudioSettings.json";
void SaveSettingsIfDue(bool force=false) {
    if (!force && (!settingsChangedAt || GetTickCount64()-settingsChangedAt<600)) return;
    try { WriteStudioSettings({distance.load(),height.load(),orbit.load(),pitch.load(),fov.load(),gridColumns,showNames,showCounts,language,allowFreeCamera.load(),addMissing,lateral.load(),elevation.load(),partialSlots.load(),startupTab},settingsFile); }
    catch(const std::exception& e) { SKSE::log::error("Settings save: {}",e.what()); }
    settingsChangedAt=0;
}
std::mutex statusMutex;
std::string status = "Save equipment with F9; click a photo to apply a preset.";
std::string lastImage;
std::atomic<unsigned> hotkey{66}; // DIK_F8, configurable in the INI.
unsigned saveHotkey = 67; // legacy INI fallback
std::array<std::atomic<unsigned>,3> captureKeys{67,68,87}, capturePads{32768,16384,64};
std::atomic<unsigned> padEdges{}, padHeld{};
bool helpOpen{};
std::atomic<bool> captureAllowed{};
std::atomic<unsigned> gamepadHotkey{32}; // View / Back; configurable, 0 disables.
std::atomic<float> padHoldSeconds{.8f};
std::atomic<unsigned> bindingMode{}, capturedBinding{};
std::atomic<ULONGLONG> bindingDeadline{};
HoldButton padHold;
const std::filesystem::path inputSettingsFile="Data/SKSE/Plugins/OutfitGallery/Hotkeys.json";
std::atomic<bool> saveBusy{};
std::mutex presetsMutex;
std::vector<Preset> presets;
Preset stagedPreset, requestedPreset;
Preset verificationTarget;
std::atomic<unsigned> verificationTicks{}; // also read by the render-thread task scheduler
ULONGLONG verificationDue{};
std::string requestedName;


bool requestedAddMissing{};
const std::filesystem::path presetFolder = "Data/SKSE/Plugins/OutfitGallery/Presets";

void ReloadPresets() {
    std::vector<Preset> found;
    if (std::filesystem::exists(presetFolder)) {
        for (const auto& f : std::filesystem::directory_iterator(presetFolder)) {
            if (f.path().extension() != ".json") continue;
            try { found.push_back(ReadPreset(f.path())); }
            catch (const std::exception& e) { SKSE::log::warn("Skipped preset {}: {}",f.path().string(),e.what()); }
        }
    }
    std::sort(found.begin(),found.end(),[](const auto& a,const auto& b){return a.photo > b.photo;});
    std::scoped_lock lock(presetsMutex); presets = std::move(found);
}
bool ownsSmooth{}, ownsDirection{}, ownsHead{};
struct Snapshot {
    bool valid{}, firstPerson{}, menusVisible{};
    float worldFov{}, thirdZoom{}, thirdTargetZoom{}, thirdSavedZoom{};
    RE::NiPoint2 thirdRotation{};
    RE::NiPoint3 freeTranslation{};
    RE::BSTPoint2<float> freeRotation{};
    bool wasFree{};
} saved;

void SetStatus(std::string text) {
    SKSE::log::info("{}", text);
    std::scoped_lock lock(statusMutex);
    status = std::move(text);
}

// A non-blocking framework notice also works when the studio cannot open.
void Refuse(std::string reason) {
    SetStatus("Cannot open studio: " + reason);
    noticeUntil = GetTickCount64() + 8000;
    if (noticeWindow) noticeWindow->IsOpen = true;
}

void __stdcall RenderNotice() {
    if (GetTickCount64() >= noticeUntil) { noticeWindow->IsOpen = false; return; }
    UI::SetNextWindowPos({30.f, 60.f}, UI::ImGuiCond_Always);
    UI::SetNextWindowSize({620.f, 0.f}, UI::ImGuiCond_Always);
    if (UI::Begin("Outfit Gallery - Status", nullptr, UI::ImGuiWindowFlags_NoInputs | UI::ImGuiWindowFlags_NoCollapse | UI::ImGuiWindowFlags_NoResize)) {
        std::scoped_lock lock(statusMutex);
        UI::TextWrapped(Tr("%s"), StatusText(status).c_str());
        UI::TextUnformatted(Tr("Details: Documents/My Games/Skyrim Special Edition/SKSE/OutfitGallery.log"));
    }
    UI::End();
}

void Release() {
    const auto handle = SKSE::GetPluginHandle();
    if (ownsHead && tdm) { (void)tdm->ReleaseDisableHeadtracking(handle); ownsHead = false; }
    if (ownsDirection && tdm) { (void)tdm->ReleaseDisableDirectionalMovement(handle); ownsDirection = false; }
    if (ownsSmooth && smooth) { (void)smooth->ReleaseCameraControl(handle); ownsSmooth = false; }
}

// Game-thread only. Also called before a save is loaded so no previous-session
// camera snapshot is ever applied to the newly loaded game.
void Close() {
    bindingMode=0;
    active = false;
    captureRequested = false;
    saveBusy = false;
    if (window) window->IsOpen = false;
    if (saved.valid) {
        auto* camera = RE::PlayerCamera::GetSingleton();
        if (camera) {
            if (!saved.wasFree && camera->IsInFreeCameraMode()) camera->ToggleFreeCameraMode(false);
            // Select the original normal view before restoring its numeric state.
            if (!saved.wasFree && (camera->IsInFirstPerson() || camera->IsInThirdPerson())) {
                if (saved.firstPerson) camera->ForceFirstPerson();
                else camera->ForceThirdPerson();
            }
            camera->GetRuntimeData2().worldFOV = saved.worldFov;
            auto* third = static_cast<RE::ThirdPersonState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kThirdPerson].get());
            if (third) {
                third->currentZoomOffset = saved.thirdZoom;
                third->targetZoomOffset = saved.thirdTargetZoom;
                third->savedZoomOffset = saved.thirdSavedZoom;
                third->freeRotation = saved.thirdRotation;
            }
            auto* free = static_cast<RE::FreeCameraState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kFree].get());
            if (free) { free->translation = saved.freeTranslation; free->rotation = saved.freeRotation; }
        }
        if (auto* ui = RE::UI::GetSingleton()) ui->ShowMenus(saved.menusVisible);
        saved.valid = false;
    }
    Release();
}

bool Acquire() {
    const auto handle = SKSE::GetPluginHandle();
    if (GetModuleHandleW(L"SmoothCam.dll")) {
        if (!smooth) { SetStatus("SmoothCam is installed but its API is unavailable."); return false; }
        auto result = smooth->RequestCameraControl(handle);
        if (result != SmoothCamAPI::APIResult::OK && result != SmoothCamAPI::APIResult::AlreadyGiven) {
            SetStatus(std::format("SmoothCam did not grant control ({})", static_cast<int>(result))); return false;
        }
        ownsSmooth = true;
    }
    if (GetModuleHandleW(L"TrueDirectionalMovement.dll")) {
        if (!tdm) { Release(); SetStatus("TDM is installed but its API is unavailable."); return false; }
        if (tdm->GetTargetLockState()) { Release(); SetStatus("Exit target lock before opening the studio."); return false; }
        auto result = tdm->RequestDisableDirectionalMovement(handle);
        if (result != TDM_API::APIResult::OK && result != TDM_API::APIResult::AlreadyGiven) {
            Release(); SetStatus("TDM did not grant movement control."); return false;
        }
        ownsDirection = true;
        result = tdm->RequestDisableHeadtracking(handle);
        if (result != TDM_API::APIResult::OK && result != TDM_API::APIResult::AlreadyGiven) {
            Release(); SetStatus("TDM did not grant head tracking control."); return false;
        }
        ownsHead = true;
    }
    return true;
}

void Open() {
    if (!ready || active || !window) return;
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* camera = RE::PlayerCamera::GetSingleton();
    auto* ui = RE::UI::GetSingleton();
    if (!player || !player->Get3D() || !camera || !ui) { SetStatus("Load a game before opening the studio."); return; }
    const bool paused = ui->GameIsPaused(), dead = player->IsDead(), combat = player->IsInCombat();
    // ActorState moved from 0xB8 to 0xC0 in AE 1.6.629+. An inherited
    // ActorState call uses the compile-time base offset in this multi-runtime
    // build; use CommonLib's version-aware base accessor instead.
    const auto* actorState = player->AsActorState();
    const bool mounted = player->IsOnMount(), swimming = actorState->IsSwimming();
    const auto sitting = actorState->GetSitSleepState();
    SKSE::log::info("ActorState runtime offset: {:#x}", reinterpret_cast<std::uintptr_t>(actorState) - reinterpret_cast<std::uintptr_t>(player));
    SKSE::log::info("Open request: paused={}, pauseCount={}, dead={}, combat={}, mounted={}, swimming={}, sitSleep={}, firstPerson={}, thirdPerson={}",
        paused, ui->numPausesGame, dead, combat, mounted, swimming, static_cast<unsigned>(sitting), camera->IsInFirstPerson(), camera->IsInThirdPerson());
    if (paused || dead || combat || mounted || swimming || sitting != RE::SIT_SLEEP_STATE::kNormal) {
        std::string reason;
        if (paused) reason += "Game paused. ";
        if (dead) reason += "Player dead. ";
        if (combat) reason += "Player in combat. ";
        if (mounted) reason += "Player mounted. ";
        if (swimming) reason += "Player swimming. ";
        if (sitting != RE::SIT_SLEEP_STATE::kNormal) reason += std::format("Sit/sleep state {}. ", static_cast<unsigned>(sitting));
        Refuse(reason); return;
    }
    const bool wasFree=camera->IsInFreeCameraMode();
    if (!camera->IsInFirstPerson() && !camera->IsInThirdPerson() && !(wasFree && allowFreeCamera.load())) { SetStatus("Use a normal first/third-person view, or enable free-camera entry."); return; }
    auto* third = static_cast<RE::ThirdPersonState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kThirdPerson].get());
    auto* free = static_cast<RE::FreeCameraState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kFree].get());
    if (!third || !free || !Acquire()) return;
    saved = {true, camera->IsInFirstPerson(), ui->IsShowingMenus(), camera->GetRuntimeData2().worldFOV,
        third->currentZoomOffset, third->targetZoomOffset, third->savedZoomOffset, third->freeRotation, free->translation, free->rotation};
    saved.wasFree=wasFree;
    if(!wasFree) {camera->ForceThirdPerson(); camera->ToggleFreeCameraMode(false);}
    if (!camera->IsInFreeCameraMode()) { Close(); SetStatus("Could not enter the studio camera."); return; }
    ui->ShowMenus(false);
    active = true;
    ++studioOpenSerial;
    window->IsOpen = true;
    SetStatus("Adjust composition and press F9/Y to save.");
}

void VerifyEquipment(bool immediate=false) {
    if(!verificationTicks) return;
    const bool due=GetTickCount64()>=verificationDue;
    if(!immediate && !due) return;
    try {
        const auto actual=SnapshotEquipment(verificationTarget.slotMask);
        if(SameEquipment(verificationTarget,actual)) {
            verificationTicks=0;
            ReclaimManagedItems();
            SetStatus("Outfit restored: "+verificationTarget.name);
        } else if(due) {
            verificationTicks=0;
            SetStatus("Equipment differs after apply: "+verificationTarget.name);
        }
    }catch(const std::exception& e) {
        if(due) {verificationTicks=0; SetStatus(std::string("Equipment verification: ")+e.what());}
    }
}
void Tick() {
    const unsigned requested = command.exchange(0);
    // Pending ownership cleanup outlives the camera window, but is cancelled
    // explicitly at save/new-game boundaries before touching another character.
    VerifyEquipment(requested==2 || requested==4);
    if (requested == 2) { Close(); return; }
    if (requested == 1) Open();
    if (!active) return;
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!window->IsOpen || !player || !player->Get3D() || !camera || !camera->IsInFreeCameraMode() || player->IsDead() || player->IsInCombat()) { Close(); return; }
    if (requested == 3 || requested == 5 || requested == 6) {
        try {
            const auto scope=requested==5?partialSlots.load():0;
            if(requested==5 && !scope) throw std::runtime_error("Select at least one slot.");
            auto p = requested==6?SnapshotAccessories():SnapshotEquipment(scope);
            { std::scoped_lock lock(presetsMutex); p.name = requestedName.empty() ? "Outfit " + std::to_string(presets.size()+1) : requestedName; stagedPreset = std::move(p); }
            captureRequested = true;
        } catch (const std::exception& e) { saveBusy = false; SetStatus(e.what()); }
    }
    if (requested == 4 && !saveBusy) {
        verificationTicks=0;
        try {
            Preset p; bool add;
            { std::scoped_lock lock(presetsMutex); p = requestedPreset; add = requestedAddMissing; }
            SetStatus(ApplyEquipment(p,add)); verificationTarget=p; verificationDue=GetTickCount64()+750; verificationTicks=1;
            VerifyEquipment(true);
        } catch (const std::exception& e) { SetStatus(e.what()); }
    }
    auto* free = static_cast<RE::FreeCameraState*>(camera->currentState.get());
    const float radians = std::numbers::pi_v<float> / 180.f;
    const float radial = player->GetAngleZ() + orbit.load() * radians;
    const float yaw = radial + std::numbers::pi_v<float>;
    const float dist = distance.load();
    const float angle=elevation.load()*radians;
    const float planar=dist*std::cos(angle);
    const float side = dist * std::tan(fov.load() * radians * 0.5f) * 0.53f + lateral.load();
    const auto pos = player->GetPosition();
    free->translation = {pos.x + std::sin(radial)*planar - std::cos(yaw)*side,
        pos.y + std::cos(radial)*planar + std::sin(yaw)*side, pos.z + height.load() + dist*std::sin(angle)};
    free->rotation = {pitch.load() * radians + angle, yaw};
    camera->GetRuntimeData2().worldFOV = fov.load();
    camera->rotationInput = {};
    camera->translationInput = {};
    camera->zoomInput = 0;
}

void QueueTick() {
    if ((active || command.load() || verificationTicks.load()) && !pending.exchange(true)) {
        const unsigned generation = epoch.load();
        SKSE::GetTaskInterface()->AddTask([generation] {
            if (generation == epoch && ready) Tick();
            pending = false;
        });
    }
}

void __stdcall OnFrame(SKSEMenuFramework::Model::EventType event) {
    if (event != SKSEMenuFramework::Model::kBeforeRender || !ready) return;
    SaveSettingsIfDue();
    // The framework dispatches listeners under its own lock. Never call back into
    // its registration/lookup APIs here. Game mutations go to one coalesced task.
    QueueTick();
    if (active && captureRequested.exchange(false)) captureDelay = 3;
    if (!active) captureDelay = 0;
    if (captureDelay && --captureDelay == 0 && active) {
        try {
            const auto file = CapturePortrait();
            { std::scoped_lock lock(statusMutex); lastImage = file.generic_string(); }
            Preset p;
            { std::scoped_lock lock(presetsMutex); p = stagedPreset; }
            p.photo = file.filename().string();
            WritePreset(p,presetFolder / (file.stem().string() + ".json"));
            ReloadPresets();
            {std::scoped_lock lock(presetsMutex); revealAfterSave=p;}
            SetStatus("Photo + outfit saved: " + p.name);
        } catch (const std::exception& e) { SetStatus(e.what()); }
        saveBusy = false;
    }
}

void Slider(const char* name, std::atomic<float>& value, float min, float max) {
    float local = value.load();
    if (UI::SliderFloat(Label(name), &local, min, max)) { value = local; settingsChangedAt=GetTickCount64(); }
}

#include "HotkeyUI.inc"
void SaveCameraSlot() {
    if(cameraSlot==0) return; // Slot 1 is always the built-in rescue composition.
    if(!cameraBankWritable) {SetStatus("Camera presets could not be read; writing is disabled."); return;}
    auto next=cameraBank;
    next[cameraSlot]=StudioSettings{distance.load(),height.load(),orbit.load(),pitch.load(),fov.load()};
    next[cameraSlot]->lateral=lateral.load(); next[cameraSlot]->elevation=elevation.load();
    try {WriteCameraBank(next,cameraBankFile); cameraBank=std::move(next); SetStatus("Camera preset saved: "+std::to_string(cameraSlot+1));}
    catch(const std::exception& e){SetStatus(e.what());}
}
void RenderCameraSettings() {
    Slider("Distance",distance,10,400); Slider("Horizontal position",lateral,-150,150); Slider("Vertical position",height,20,180);
    Slider("Orbit",orbit,-180,180); Slider("Look up / down",elevation,-60,60); Slider("Field of view",fov,35,90);
    if(UI::CollapsingHeader(Label("Fine angle adjustment"))) Slider("Pitch",pitch,-25,25);
    UI::TextUnformatted(Tr("Camera and display settings save automatically."));
    int shownSlot=cameraSlot+1;
    if(UI::SliderInt(Label("Camera preset slot"),&shownSlot,1,7)) cameraSlot=shownSlot-1;
    UI::BeginDisabled(cameraSlot==0);
    if(UI::Button(Label("Save current camera to selected slot"))) SaveCameraSlot();
    UI::EndDisabled();
    if(cameraSlot==0) UI::TextUnformatted(Tr("Default camera (fixed)"));
    if(cameraBank[0] && UI::Button(Label("Load previous custom slot 1"))) {
        const auto& s=*cameraBank[0]; distance=s.distance; height=s.height; orbit=s.orbit; pitch=s.pitch; fov=s.fov; lateral=s.lateral; elevation=s.elevation;
        settingsChangedAt=GetTickCount64();
    }
    bool freeEntry=allowFreeCamera.load();
    if(UI::Checkbox(Label("Allow opening from free camera (experimental)"),&freeEntry)) {allowFreeCamera=freeEntry; settingsChangedAt=GetTickCount64();}

}
void RenderCameraSlots() {
    UI::BeginDisabled(saveBusy);
    UI::SameLine(); UI::TextUnformatted(Tr("Camera presets"));
    for(int n=0;n<7;++n) {
        if(UI::GetWindowPos().x+UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x-UI::GetItemRectMax().x>UI::GetFrameHeight()+UI::GetStyle()->ItemSpacing.x) UI::SameLine();
        if(n==cameraSlot) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
        const bool clicked=UI::Button((std::to_string(n+1)+"##camera").c_str(),{UI::GetFrameHeight(),0});
        if(n==cameraSlot) UI::PopStyleColor();
        if(UI::IsItemHovered()) {
            if(n==0) UI::SetTooltip("%s",Tr("Default camera (fixed)"));
            else UI::SetTooltip(Tr(cameraBank[n]?"Load camera preset %d":"Camera preset %d is empty"),n+1);
        }
        if(clicked) {
            cameraSlot=n;
            if(n==0 || cameraBank[n]) {
                const auto s=n==0 ? StudioSettings{} : *cameraBank[n]; distance=s.distance; height=s.height; orbit=s.orbit; pitch=s.pitch; fov=s.fov; lateral=s.lateral; elevation=s.elevation;
                settingsChangedAt=GetTickCount64();
            } else SetStatus("Empty camera slot: "+std::to_string(n+1));
        }
    }
    UI::EndDisabled();
    const float available=UI::GetWindowPos().x+UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x-UI::GetItemRectMax().x;
    const float needed=UI::GetFrameHeight()+UI::GetStyle()->ItemInnerSpacing.x+UI::CalcTextSize(Tr("Camera settings")).x+UI::GetStyle()->ItemSpacing.x;
    const float extraGap=UI::CalcTextSize(" ").x;
    if(available>needed+extraGap) UI::SameLine(0,UI::GetStyle()->ItemSpacing.x+extraGap);
    UI::Checkbox(Label("Camera settings"),&showCameraSettings);
    if(showCameraSettings) RenderCameraSettings();
}
#include "GalleryBrowser.inc"

void __stdcall RenderStudio() {
    static char name[160]{};
    UpdatePadFrame();
    static unsigned openedSerial{};


    if(openedSerial!=studioOpenSerial.load()) {openedSerial=studioOpenSerial.load(); showCameraSettings=false;}


    const auto screen = UI::GetIO()->DisplaySize;
    UI::SetNextWindowPos({screen.x*0.01f, screen.y*0.015f}, UI::ImGuiCond_Always);
    UI::SetNextWindowSize({screen.x*0.515f, screen.y*0.97f}, UI::ImGuiCond_Always);
    bool opened = true;
    if (UI::Begin("Outfit Gallery", &opened, UI::ImGuiWindowFlags_NoResize | UI::ImGuiWindowFlags_NoMove | UI::ImGuiWindowFlags_NoCollapse | UI::ImGuiWindowFlags_NoNavInputs)) {
        UI::InputText(Label("Preset name"),name,sizeof(name));
        UI::SameLine(); UI::SetCursorPosX(UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x-UI::GetFrameHeight());
        if(UI::Button("?##help",{UI::GetFrameHeight(),0})) helpOpen=!helpOpen;
        UI::BeginDisabled(saveBusy);

        if (UI::Button(Label("Save outfit"))) {
            if (!saveBusy.exchange(true)) {
                { std::scoped_lock lock(presetsMutex); requestedName = name; }
                command = 3;
            }
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s\n%s",Tr("Register all worn equipment. Applying replaces the full outfit."),CaptureHint(0).c_str());
        UI::EndDisabled();
        UI::SameLine();
        UI::BeginDisabled(saveBusy);
        if(UI::Button(Label("Save head equipment")) && !saveBusy.exchange(true)) {
            {std::scoped_lock lock(presetsMutex); requestedName=name;} command=5;
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s\n%s",Tr("Register head equipment. Applying keeps clothes and weapons."),CaptureHint(1).c_str());
        UI::EndDisabled();
        UI::SameLine();
        UI::BeginDisabled(saveBusy);
        if(UI::Button(Label("Save accessories")) && !saveBusy.exchange(true)) {
            {std::scoped_lock lock(presetsMutex); requestedName=name;} command=6;
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s\n%s",Tr("Wear only the accessories to register. Applying adds or replaces their slots."),CaptureHint(2).c_str());
        UI::EndDisabled();
        RenderCameraSlots();
        std::string text, image;
        { std::scoped_lock lock(statusMutex); text = status; image = lastImage; }
        if(text!="Adjust composition and press F9/Y to save." && text!="Save equipment with F9; click a photo to apply a preset.") UI::TextWrapped(Tr("%s"), StatusText(text).c_str());
        UI::Separator();
        RenderBrowser(addMissing);
    }
    UI::End();
    if(helpOpen) {
        UI::SetNextWindowSize({screen.x*.40f,screen.y*.60f},UI::ImGuiCond_FirstUseEver);
        if(UI::Begin(Label("Help"),&helpOpen,UI::ImGuiWindowFlags_NoCollapse)) {
            UI::TextWrapped(Tr("1. Wear the items to register, then use a registration button or its assigned key."));
            UI::TextWrapped(Tr("Outfit: register all worn equipment."));
            UI::Text("%s",CaptureHint(0).c_str());
            UI::TextWrapped(Tr("Head: register head equipment only."));
            UI::Text("%s",CaptureHint(1).c_str());
            UI::TextWrapped(Tr("Accessories: wear only the items you want to add, then register."));
            UI::Text("%s",CaptureHint(2).c_str());
            UI::Spacing(); UI::Spacing();
            UI::TextWrapped(Tr("Select a photo below to apply its equipment."));
            UI::TextWrapped(Tr("Text tabs: replace the full outfit (organize with tabs)."));
            UI::TextWrapped(Tr("Person icon: change only head equipment, keeping your clothes."));
            UI::TextWrapped(Tr("Ring icon: add the pictured items, keeping your clothes; replace items in matching slots."));
            UI::Spacing();
            UI::TextWrapped(Tr("D-pad: select. A: apply. LB/RB: tabs. LT: head. RT: accessories. R3: photo menu. B: close."));
            UI::TextWrapped(Tr("Enable Add missing base items to restore unowned items. Items spanning unrelated slots may block a partial change."));
            UI::TextWrapped(Tr("Drag inside the camera frame to move the subject. Use the wheel to change distance."));
            UI::TextWrapped(Tr("Right-drag horizontally to orbit; vertically to look up or down."));
            if(UI::Button(Label("Close"))) helpOpen=false;
        }
        UI::End();
        if(RawPadPressed(8192)) helpOpen=false;
    }
    // A transparent input surface makes the photo frame a mouse target. ImGui
    // handles hit testing, so an overlapping help window or popup wins normally.
    UI::SetNextWindowPos({screen.x*.54f,screen.y*.08f},UI::ImGuiCond_Always);
    UI::SetNextWindowSize({screen.x*.45f,screen.y*.84f},UI::ImGuiCond_Always);
    UI::PushStyleVar(UI::ImGuiStyleVar_WindowPadding,{0,0});
    if(UI::Begin("##cameraMouse",nullptr,UI::ImGuiWindowFlags_NoDecoration|UI::ImGuiWindowFlags_NoMove|UI::ImGuiWindowFlags_NoBackground|UI::ImGuiWindowFlags_NoSavedSettings|UI::ImGuiWindowFlags_NoNav|UI::ImGuiWindowFlags_NoBringToFrontOnFocus|UI::ImGuiWindowFlags_NoScrollWithMouse)) {
        UI::InvisibleButton("##pan",UI::GetContentRegionAvail(),UI::ImGuiButtonFlags_MouseButtonLeft|UI::ImGuiButtonFlags_MouseButtonRight);
        if(!saveBusy && !helpOpen && !bindingMode) {
            auto* io=UI::GetIO();
            const float units=2.f*distance.load()*std::tan(fov.load()*.00872664626f)/std::max(1.f,screen.x);
            bool changed=false;
            if(UI::IsItemActive() && UI::IsMouseDragging(0) && !UI::IsMouseDown(1)) {
                lateral=std::clamp(lateral.load()+io->MouseDelta.x*units,-150.f,150.f);
                height=std::clamp(height.load()+io->MouseDelta.y*units,20.f,180.f);
                changed=io->MouseDelta.x!=0 || io->MouseDelta.y!=0;
            }
            if(UI::IsItemActive() && UI::IsMouseDragging(1) && !UI::IsMouseDown(0)) {
                // Screen-relative sensitivity keeps the same gesture consistent
                // across resolutions. Wrap orbit continuously; limit elevation
                // to the same range as its slider.
                orbit=std::remainder(orbit.load()+io->MouseDelta.x*180.f/std::max(1.f,screen.x),360.f);
                elevation=std::clamp(elevation.load()+io->MouseDelta.y*180.f/std::max(1.f,screen.y),-60.f,60.f);
                changed=io->MouseDelta.x!=0 || io->MouseDelta.y!=0;
            }
            if(UI::IsItemHovered() && io->MouseWheel!=0) {
                distance=std::clamp(distance.load()*std::pow(.9f,io->MouseWheel),10.f,400.f); changed=true;
            }
            if(changed) settingsChangedAt=GetTickCount64();
        }
    }
    UI::End(); UI::PopStyleVar();
    auto* draw = UI::GetForegroundDrawList();
    UI::ImDrawListManager::AddRect(draw, {screen.x*.54f,screen.y*.08f}, {screen.x*.99f,screen.y*.92f}, 0xFF9BC6E8, 0, 0, 2);
    if (!opened || UI::IsKeyPressed(UI::ImGuiKey_Escape, false)) command = 2;
}

void __stdcall RenderSettings() {
    RenderHotkeys();
    UI::TextUnformatted(Tr("Outfit Gallery 1.0.0 - photo presets"));
    UI::TextWrapped(Tr("Close this menu and press the configured hotkey (default F8) while standing in a safe open area."));
    UI::TextUnformatted(Tr("F9 in the studio saves a photo and equipment. Click a preset photo to apply."));
    UI::Text(Tr("SmoothCam API: %s / TDM API: %s"), smooth ? "ready" : "not available", tdm ? "ready" : "not available");
    std::scoped_lock lock(statusMutex);
    UI::TextWrapped(Tr("%s"), StatusText(status).c_str());
}

bool __stdcall Input(RE::InputEvent* event) {
    if (!event || !ready) return false;
    auto* button = event->AsButtonEvent();
    if (!button) return false;
    const auto device=button->GetDevice();
    const auto code=button->GetIDCode();
    const unsigned binding=bindingMode.load();
    if(binding && GetTickCount64()<=bindingDeadline && button->IsDown()) {
        if(device==RE::INPUT_DEVICE::kKeyboard && code==1) {bindingMode=0; return true;}
        if(((binding%2)==1 && device==RE::INPUT_DEVICE::kKeyboard) || ((binding%2)==0 && device==RE::INPUT_DEVICE::kGamepad)) {
            capturedBinding=(binding<<16)|code; bindingMode=0; padHold={}; return true;
        }
    }
    if(device==RE::INPUT_DEVICE::kGamepad) {
        const unsigned bit=code==9?0x10000u:code==10?0x20000u:code;
        if(button->IsDown() && active) padEdges.fetch_or(bit);
        if(button->IsPressed()) padHeld.fetch_or(bit); else padHeld.fetch_and(~bit);
    }
    if(device==RE::INPUT_DEVICE::kGamepad && gamepadHotkey && code==gamepadHotkey) {
        if(padHold.Update(button->IsDown(),button->IsPressed(),button->HeldDuration(),padHoldSeconds.load())) {
            command=active?2:1; QueueTick();
        }
        // Observe, never consume: short presses and release reach the game/other mods.
        return false;
    }
    if(device!=RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) return false;
    for(unsigned action=0;action<3;++action) if (active && captureAllowed && captureKeys[action] && code==captureKeys[action]) {
        if (!saveBusy.exchange(true)) {
            { std::scoped_lock lock(presetsMutex); requestedName.clear(); }
            command = action==0?3:action==1?5:6; QueueTick();
        }
        return true;
    }
    if (button->GetIDCode() == hotkey || (active && button->GetIDCode() == 1)) {
        command = active ? 2 : 1;
        // Closing must also work if the next UI render callback never arrives.
        QueueTick();
        return true;
    }
    return false;
}

void Message(SKSE::MessagingInterface::Message* message) {
    if (!message) return;
    switch (message->type) {
    case SKSE::MessagingInterface::kPostLoad:
        (void)SmoothCamAPI::RegisterInterfaceLoaderCallback(SKSE::GetMessagingInterface(), [](void* api, SmoothCamAPI::InterfaceVersion v) {
            if (v == SmoothCamAPI::InterfaceVersion::V3) smooth = static_cast<SmoothCamAPI::IVSmoothCam3*>(api);
        });
        if (GetModuleHandleW(L"TrueDirectionalMovement.dll")) tdm = static_cast<TDM_API::IVTDM2*>(TDM_API::RequestPluginAPI(TDM_API::InterfaceVersion::V2));
        break;
    case SKSE::MessagingInterface::kPostPostLoad:
        (void)SmoothCamAPI::RequestInterface(SKSE::GetMessagingInterface());
        break;
    case SKSE::MessagingInterface::kDataLoaded: {
        RegisterManagedItemEvents();
        if (!GetMenuFrameworkModule() || !GetProcAddress(GetMenuFrameworkModule(), "RegisterEventPriority") || !GetProcAddress(GetMenuFrameworkModule(), "RegisterInpoutEvent")) {
            SetStatus("A compatible SKSE Menu Framework is required."); break;
        }
        window = SKSEMenuFramework::AddWindow(RenderStudio, true);
        if (!window) { SetStatus("Could not create framework window."); break; }
        noticeWindow = SKSEMenuFramework::AddWindow(RenderNotice, false);
        SKSEMenuFramework::SetSection("Outfit Gallery");
        SKSEMenuFramework::AddSectionItem("Photo presets", RenderSettings);
        (void)SKSEMenuFramework::AddEvent(OnFrame, 0);
        (void)SKSEMenuFramework::AddInputEvent(Input);
        try {
            if(std::filesystem::exists(settingsFile)) {
                const auto s=ReadStudioSettings(settingsFile);
                distance=s.distance; height=s.height; orbit=s.orbit; pitch=s.pitch; fov=s.fov; lateral=s.lateral; elevation=s.elevation; gridColumns=s.columns; showNames=s.showNames; showCounts=s.showCounts; language=s.language; allowFreeCamera=s.allowFreeCamera; addMissing=s.addMissing; partialSlots=s.headSlots; startupTab=s.startupTab;
                SKSE::log::info("Loaded persistent studio settings");
            }
        } catch(const std::exception& e) { SetStatus(std::string("Camera settings: ")+e.what()); }
        try {
            if(std::filesystem::exists(libraryFile)) library=ReadLibrary(libraryFile);
            else library.categories={{"favorites","Favorites"},{"everyday","Everyday"},{"battle","Battle"},{"dress","Custom"}};
        } catch(const std::exception& e) { libraryWritable=false; SetStatus(std::string("Library read failed: ")+e.what()); }
        try {
            if(std::filesystem::exists(inputSettingsFile)) {
                auto keys=ReadInputSettings(inputSettingsFile);
                hotkey=keys.keyboard; gamepadHotkey=keys.gamepad; padHoldSeconds=keys.holdSeconds;
                for(unsigned n=0;n<3;++n) {captureKeys[n]=keys.captureKeys[n];capturePads[n]=keys.capturePads[n];}
            }
        }catch(const std::exception& e){SetStatus(std::string("Hotkey settings: ")+e.what());}
        try {if(std::filesystem::exists(cameraBankFile)) cameraBank=ReadCameraBank(cameraBankFile);}
        catch(const std::exception& e){cameraBankWritable=false; SetStatus(e.what());}
        ready = true;
        try { ReloadPresets(); } catch (const std::exception& e) { SetStatus(e.what()); }
        SKSE::log::info("Registered gallery UI; SmoothCam={}, TDM={}", smooth != nullptr, tdm != nullptr);
        break;
    }
    case SKSE::MessagingInterface::kPreLoadGame:
        ++epoch; command = 0; verificationTicks=0; Close(); ready = false; break;
    case SKSE::MessagingInterface::kPostLoadGame:
    case SKSE::MessagingInterface::kNewGame:
        ++epoch; command = 0; verificationTicks=0; ready = window != nullptr; break;
    default: break;
    }
}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    if (skse->IsEditor()) return false;
    const auto logs = SKSE::log::log_directory();
    if (!logs) return false;
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*logs / "OutfitGallery.log").string(), true);
    auto logger = std::make_shared<spdlog::logger>("OutfitGallery", sink);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
    const auto runtime=skse->RuntimeVersion();
    const auto supported=SKSEPlugin_Version.GetRuntimeCompatibility().GetCompatibleRuntimeVersions();
    if(std::none_of(supported.begin(),supported.end(),[&](const auto& version){return static_cast<REL::Version>(version)==runtime;})) {
        SKSE::log::error("Unsupported Skyrim runtime {}. Supported targets: 1.5.97, 1.6.353, 1.6.640, 1.6.1130, 1.6.1170. VR/GOG are not enabled.",runtime.string());
        return false;
    }
    SKSE::log::info("Runtime {} accepted; release runtime target; development testing reported on 1.5.97 and 1.6.1170.",runtime.string());
    SKSE::Init(skse, false);
    Gallery::InitializeManagedItems();
    const auto ini = std::filesystem::absolute("Data/SKSE/Plugins/OutfitGallery.ini");
    Gallery::hotkey = GetPrivateProfileIntW(L"Input", L"Hotkey", 66, ini.c_str());
    Gallery::saveHotkey = GetPrivateProfileIntW(L"Input", L"SaveHotkey", 67, ini.c_str());
    Gallery::captureKeys[0]=Gallery::saveHotkey;
    Gallery::gamepadHotkey = GetPrivateProfileIntW(L"Input", L"GamepadHotkey", 32, ini.c_str());
    SKSE::log::info("OutfitGallery 1.0.0 release; runtime {}; key={}", runtime.string(), Gallery::hotkey.load());
    return SKSE::GetMessagingInterface()->RegisterListener(Gallery::Message);
}








