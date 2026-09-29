std::string KeyLabel(unsigned key) {
    if(!key) return Tr("Disabled");
    wchar_t label[128]{};
    const LONG scan=static_cast<LONG>((key&0x7F)<<16)|((key&0x80)?(1<<24):0);
    if(!GetKeyNameTextW(scan,label,128)) return std::format("Key {}",key);
    char utf8[512]{}; WideCharToMultiByte(CP_UTF8,0,label,-1,utf8,512,nullptr,nullptr); return utf8;
}
std::string PadLabel(unsigned key) {
    switch(key) {
    case 0:return "Disabled"; case 1:return "D-pad Up";case 2:return "D-pad Down";case 4:return "D-pad Left";case 8:return "D-pad Right";
    case 9:return "LT / L2";case 10:return "RT / R2";case 16:return "Start / Menu";case 32:return "Back / View";
    case 64:return "Left stick click";case 128:return "Right stick click";case 256:return "LB / L1";case 512:return "RB / R1";
    case 4096:return "A / Cross";case 8192:return "B / Circle";case 16384:return "X / Square";case 32768:return "Y / Triangle";
    default:return std::format("Button {}",key);
    }
}
InputSettings CurrentInputSettings() {
    InputSettings s{hotkey.load(),gamepadHotkey.load(),padHoldSeconds.load()};
    for(unsigned n=0;n<3;++n) {s.captureKeys[n]=captureKeys[n];s.capturePads[n]=capturePads[n];} return s;
}
void SaveInputSettings(const InputSettings& s) {
    WriteInputSettings(s,inputSettingsFile);
    hotkey=s.keyboard; gamepadHotkey=s.gamepad; padHoldSeconds=s.holdSeconds;
    for(unsigned n=0;n<3;++n) {captureKeys[n]=s.captureKeys[n];capturePads[n]=s.capturePads[n];}
}
unsigned framePadEdges{},framePadRepeat{};
void UpdatePadFrame() {
    framePadEdges=padEdges.exchange(0); framePadRepeat=framePadEdges;
    const auto held=padHeld.load(); const auto now=GetTickCount64();
    static std::array<ULONGLONG,18> next{};
    for(unsigned n=0;n<18;++n) {const auto bit=1u<<n;
        if(framePadEdges&bit) next[n]=now+350;
        else if((held&bit) && now>=next[n]) {framePadRepeat|=bit;next[n]=now+100;}
        if(!(held&bit)) next[n]=now+350;
    }
}
bool RawPadPressed(unsigned code,bool repeat=false) {
    const unsigned bit=code==9?0x10000u:code==10?0x20000u:code;
    return code && ((repeat?framePadRepeat:framePadEdges)&bit)!=0;
}
bool PadPressed(UI::ImGuiKey key,bool repeat=false) {
    unsigned code=0;
    switch(key) {
    case UI::ImGuiKey_GamepadDpadUp:code=1;break;case UI::ImGuiKey_GamepadDpadDown:code=2;break;
    case UI::ImGuiKey_GamepadDpadLeft:code=4;break;case UI::ImGuiKey_GamepadDpadRight:code=8;break;
    case UI::ImGuiKey_GamepadL1:code=256;break;case UI::ImGuiKey_GamepadR1:code=512;break;
    case UI::ImGuiKey_GamepadL2:code=9;break;case UI::ImGuiKey_GamepadR2:code=10;break;
    case UI::ImGuiKey_GamepadFaceDown:code=4096;break;case UI::ImGuiKey_GamepadFaceRight:code=8192;break;
    case UI::ImGuiKey_GamepadR3:code=128;break;
    default:return UI::IsKeyPressed(key,repeat);
    } return RawPadPressed(code,repeat);
}
std::string CaptureHint(unsigned n) {
    return KeyLabel(captureKeys[n])+" / "+PadLabel(capturePads[n]);
}
void ApplyHotkeys(unsigned keyboard,unsigned pad,float seconds) {
    try {

        auto s=CurrentInputSettings(); s.keyboard=keyboard;s.gamepad=pad;s.holdSeconds=seconds; SaveInputSettings(s);
        hotkey=keyboard; gamepadHotkey=pad; padHoldSeconds=seconds;
        SetStatus("Menu hotkeys saved.");
    } catch(const std::exception& e) {SetStatus(e.what());}
}
void RenderHotkeys() {
    const auto result=capturedBinding.exchange(0);
    if(result) {
        if((result>>16)==1) ApplyHotkeys(result&0xFFFF,gamepadHotkey.load(),padHoldSeconds.load());
        else if((result>>16)==2) ApplyHotkeys(hotkey.load(),result&0xFFFF,padHoldSeconds.load());
        else {try {auto s=CurrentInputSettings();const unsigned mode=result>>16,n=(mode-3)/2;
            if(n<3) {if(mode%2) s.captureKeys[n]=result&0xFFFF;else s.capturePads[n]=result&0xFFFF; SaveInputSettings(s);}
        }catch(const std::exception& e){SetStatus(e.what());}}
    }
    if(bindingMode && GetTickCount64()>bindingDeadline) bindingMode=0;
    UI::Text(Tr("Open menu (keyboard): %s"),KeyLabel(hotkey.load()).c_str());
    if(UI::Button(Label("Change keyboard key"))) {bindingMode=1;bindingDeadline=GetTickCount64()+10000;}
    UI::Text(Tr("Open menu (gamepad): %s / hold %.1f s"),Tr(PadLabel(gamepadHotkey.load()).c_str()),padHoldSeconds.load());
    if(UI::Button(Label("Change gamepad button"))) {bindingMode=2;bindingDeadline=GetTickCount64()+10000;}
    UI::SameLine(); if(UI::Button(Label("Disable gamepad hotkey"))) ApplyHotkeys(hotkey.load(),0,padHoldSeconds.load());
    float seconds=padHoldSeconds.load();
    if(UI::SliderFloat(Label("Hold duration"),&seconds,.4f,3.f,"%.1f s")) padHoldSeconds=seconds;
    if(UI::IsItemDeactivatedAfterEdit()) ApplyHotkeys(hotkey.load(),gamepadHotkey.load(),padHoldSeconds.load());
    if(UI::CollapsingHeader(Label("Capture keys"))) {
        const char* names[]{"Save outfit","Save headgear","Save accessories"};
        for(unsigned n=0;n<3;++n) {
            UI::Text("%s: %s",Tr(names[n]),CaptureHint(n).c_str());
            if(UI::Button((std::string(Tr("Keyboard"))+"##capture"+std::to_string(n)).c_str())) {bindingMode=3+n*2;bindingDeadline=GetTickCount64()+10000;}
            UI::SameLine();
            if(UI::Button((std::string(Tr("Gamepad"))+"##capture"+std::to_string(n)).c_str())) {bindingMode=4+n*2;bindingDeadline=GetTickCount64()+10000;}
            UI::SameLine();
            if(UI::Button((std::string(Tr("Disable"))+"##capture"+std::to_string(n)).c_str())) {try {auto s=CurrentInputSettings();s.captureKeys[n]=0;s.capturePads[n]=0;SaveInputSettings(s);}catch(const std::exception& e){SetStatus(e.what());}}
        }
        if(UI::Button(Label("Reset capture keys"))) {try {auto s=CurrentInputSettings();s.captureKeys={67,68,87};s.capturePads={32768,16384,64};SaveInputSettings(s);}catch(const std::exception& e){SetStatus(e.what());}}
        UI::TextWrapped(Tr("Navigation buttons are reserved. Disable a conflicting capture binding before assigning it."));
    }
    if(bindingMode) {
        UI::TextUnformatted(Tr("Press the new key/button. Escape or Cancel stops capture."));
        if(UI::Button(Label("Cancel binding"))) bindingMode=0;
    }
    UI::TextWrapped(Tr("Short gamepad presses pass through. Another mod may also react during a long press; choose an unused button if needed."));
    UI::Separator();
}
