// Included inside Gallery; UI-owned state is only changed on the render thread.
Library library;
bool libraryWritable=true;

const std::filesystem::path libraryFile="Data/SKSE/Plugins/OutfitGallery/Library.json";

bool CommitLibrary(Library next) {
    if(!libraryWritable) { SetStatus("Library could not be read. Restore Library.json before editing."); return false; }
    try { WriteLibrary(next,libraryFile); library=std::move(next); return true; }
    catch(const std::exception& e) { SetStatus(e.what()); return false; }
}

void RenderBrowser(bool& addMissing) {
    static char search[160]{}, editName[241]{}, tabName[241]{};
    static std::string category, selected, contextPhoto, editCategory;
    static bool newCategory{};
    static std::vector<std::string> deletePhotos;
    bool openDelete=false;
    static std::vector<std::string> loaded;
    static std::string oldFilter;
    static bool tabsFocused{}, columnsFocused{};
    static unsigned lastOpen{};
    if(lastOpen!=studioOpenSerial.load()) {
        lastOpen=studioOpenSerial.load(); tabsFocused=false; columnsFocused=false; selected.clear(); oldFilter.clear(); search[0]=0;
        category=ResolveStartupTab(startupTab,library);
        UI::ClearActiveID();
    }
    std::optional<Preset> reveal;
    {std::scoped_lock lock(presetsMutex); reveal=std::move(revealAfterSave); revealAfterSave.reset();}
    if(reveal) {
        category=reveal->accessories?"#accessories":reveal->slotMask?"#head":"";
        selected=reveal->photo; search[0]=0; oldFilter.clear(); tabsFocused=false; columnsFocused=false;
        UI::ClearActiveID();
    }
    bool openEdit=false, openTab=false;
    if(UI::CollapsingHeader(Label("Options"))) {
        if(UI::BeginCombo("Language / 言語",language==1?"日本語":"English")) {
            if(UI::Selectable("日本語",language==1)) {language=1; settingsChangedAt=GetTickCount64();}
            if(UI::Selectable("English",language==0)) {language=0; settingsChangedAt=GetTickCount64();}
            UI::EndCombo();
        }
        RenderHotkeys();
        bool followers=followerTargeting.load();
        if(UI::Checkbox(Label("Select follower under crosshair (experimental)"),&followers)) {followerTargeting=followers; settingsChangedAt=GetTickCount64();}
        if(followers) UI::TextWrapped(Tr("Close the gallery, aim at a current humanoid follower and press the opening hotkey. Look away to select yourself. Target is fixed until close."));
        bool preview=detachedPreview.load();
        UI::BeginDisabled(saveBusy);
        if(UI::Checkbox(Label("Independent preview"),&preview)) {
            detachedPreview=preview; previewSettled=0; resetPreviewRect=true;
            settingsChangedAt=GetTickCount64();
        }
        if(preview) {
            UI::TextWrapped(Tr("Drag the preview title to move it; drag its corner to resize. Photos use the centered view. Effects depend on your graphics setup."));
            if(UI::Button(Label("Reset preview position"))) {previewRect={.54f,.08f,.45f,.84f}; resetPreviewRect=true; settingsChangedAt=GetTickCount64();}
        }
        UI::EndDisabled();
        std::vector<Category> openingTabs{{"","All"}};
        openingTabs.insert(openingTabs.end(),library.categories.begin(),library.categories.end());
        openingTabs.push_back({"#head","Head equipment"});
        openingTabs.push_back({"#accessories","Accessories"});
        openingTabs.push_back({"#trash","Trash"});
        const auto resolvedOpening=ResolveStartupTab(startupTab,library);
        const auto opening=std::find_if(openingTabs.begin(),openingTabs.end(),[&](const auto& c){return c.id==resolvedOpening;});
        const auto openingTitle=opening==openingTabs.end()?std::string(Tr("All")):CategoryTitle(*opening);
        if(UI::BeginCombo(Label("Opening tab"),openingTitle.c_str())) {
            for(const auto& c:openingTabs) {
                if(UI::Selectable((CategoryTitle(c)+"##opening"+c.id).c_str(),c.id==resolvedOpening)) {
                    startupTab=c.id; SaveSettingsIfDue(true);
                }
            }
            UI::EndCombo();
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Choose the tab shown when opening the gallery. Registration still opens the matching tab."));
        if(UI::CollapsingHeader(Label("Head equipment slots"))) {
            auto mask=partialSlots.load();
            const unsigned slots[]{30,31,41,42};
            for(auto slot:slots) {
                if(slot!=30) UI::SameLine();
                bool checked=(mask&(1u<<(slot-30)))!=0;
                if(UI::Checkbox(std::to_string(slot).c_str(),&checked)) {
                    if(checked) mask|=1u<<(slot-30); else mask&=~(1u<<(slot-30));
                    partialSlots=mask; SaveSettingsIfDue(true);
                }
            }
            UI::TextWrapped(Tr("Head capture uses these slots. Saved photos keep their own scope."));
        }
        bool changed=UI::Checkbox(Label("Show preset names"),&showNames);
        changed=UI::Checkbox(Label("Show item counts"),&showCounts)||changed;
        changed=UI::SliderInt(Label("Columns"),&gridColumns,2,5)||changed;
        if(changed) settingsChangedAt=GetTickCount64();
        if(UI::Button(Label("+ Tab"))) {newCategory=true; tabName[0]=0; openTab=true;}
        UI::SameLine();
        UI::BeginDisabled(category.empty() || category.starts_with("#"));
        if(UI::Button(Label("Edit tab"))) {
            auto it=std::find_if(library.categories.begin(),library.categories.end(),[&](const auto& c){return c.id==category;});
            if(it!=library.categories.end()) {newCategory=false; editCategory=category; strcpy_s(tabName,it->name.c_str()); openTab=true;}
        }
        UI::EndDisabled();
        if(UI::Checkbox(Label("Add missing base items (new game)"),&addMissing)) { SaveSettingsIfDue(true); }
        UI::TextWrapped(Tr("Base items only. Upgrades, custom enchantments and spells are not saved."));
    }
    UI::InputText(Label("Search outfits"),search,sizeof(search));

    const bool modal=UI::IsPopupOpen(nullptr,UI::ImGuiPopupFlags_AnyPopupId);
    const bool padIntent=framePadEdges || framePadRepeat;
    if(padIntent && !modal && !bindingMode && !UI::IsMouseDown(0)) UI::ClearActiveID();
    const bool navigating=!modal && !helpOpen && !bindingMode && (!UI::GetIO()->WantTextInput || padIntent) && !UI::IsAnyItemActive() && !saveBusy;
    const bool down=PadPressed(UI::ImGuiKey_GamepadDpadDown,true)||PadPressed(UI::ImGuiKey_GamepadLStickDown,true);
    const bool up=PadPressed(UI::ImGuiKey_GamepadDpadUp,true)||PadPressed(UI::ImGuiKey_GamepadLStickUp,true);
    const bool left=PadPressed(UI::ImGuiKey_GamepadDpadLeft,true)||PadPressed(UI::ImGuiKey_GamepadLStickLeft,true);
    const bool right=PadPressed(UI::ImGuiKey_GamepadDpadRight,true)||PadPressed(UI::ImGuiKey_GamepadLStickRight,true);
    bool tabChanged=false;
    const bool browse=navigating && !reveal;
    captureAllowed=browse;
    std::vector<Category> tabs{{"","All"}};
    tabs.insert(tabs.end(),library.categories.begin(),library.categories.end());
    tabs.push_back({"#head","Head equipment"});
    tabs.push_back({"#accessories","Accessories"});
    {std::scoped_lock lock(presetsMutex); if(std::any_of(presets.begin(),presets.end(),[](const auto& p){return !p.accessories && p.slotMask && !SlotFits(p.slotMask,0x1803);})) tabs.push_back({"#legacy","Legacy parts"});}
    tabs.push_back({"#trash","Trash"});
    if(browse) {
        auto it=std::find_if(tabs.begin(),tabs.end(),[&](const auto& c){return c.id==category;});
        int idx=it==tabs.end()?0:static_cast<int>(it-tabs.begin());
        if(tabsFocused && (left || right)) {
            int target=columnsFocused?static_cast<int>(tabs.size()):idx;
            target=(target+(right?1:-1)+static_cast<int>(tabs.size())+1)%(static_cast<int>(tabs.size())+1);
            columnsFocused=target==static_cast<int>(tabs.size());
            if(!columnsFocused) {idx=target; tabChanged=true;}
        }
        if(PadPressed(UI::ImGuiKey_GamepadL1,false)) {idx=(idx+static_cast<int>(tabs.size())-1)%static_cast<int>(tabs.size()); tabsFocused=true; columnsFocused=false; tabChanged=true;}
        if(PadPressed(UI::ImGuiKey_GamepadR1,false)) {idx=(idx+1)%static_cast<int>(tabs.size()); tabsFocused=true; columnsFocused=false; tabChanged=true;}
        category=tabs[idx].id;
        if(PadPressed(UI::ImGuiKey_GamepadL2,false)) {category="#head";tabsFocused=true;columnsFocused=false;tabChanged=true;}
        if(PadPressed(UI::ImGuiKey_GamepadR2,false)) {category="#accessories";tabsFocused=true;columnsFocused=false;tabChanged=true;}
    }
    const auto* style=UI::GetStyle();
    const float tabHeight=UI::GetTextLineHeightWithSpacing();
    float tabTotal=70.f;
    for(const auto& c:tabs) tabTotal+=std::max(105.f,UI::CalcTextSize(CategoryTitle(c).c_str()).x+style->FramePadding.x*2)+style->ItemSpacing.x;
    const bool tabOverflow=tabTotal>UI::GetContentRegionAvail().x-style->WindowPadding.x*2;
    const float stripHeight=tabHeight+style->WindowPadding.y*2+(tabOverflow?style->ScrollbarSize:0);
    UI::BeginChild("Categories",{0,stripHeight},0,UI::ImGuiWindowFlags_HorizontalScrollbar|UI::ImGuiWindowFlags_NoNavInputs);
    for(std::size_t n=0;n<tabs.size();++n) {
        if(n) UI::SameLine();
        const auto& c=tabs[n];
        const float tabWidth=std::max(105.f,UI::CalcTextSize(CategoryTitle(c).c_str()).x+style->FramePadding.x*2);
        const bool icon=c.id=="#trash" || c.id=="#head" || c.id=="#accessories";
        UI::PushStyleColor(UI::ImGuiCol_Header,{.42f,.29f,.07f,1.f});
        UI::PushStyleColor(UI::ImGuiCol_HeaderHovered,{.55f,.38f,.10f,1.f});
        UI::PushStyleColor(UI::ImGuiCol_HeaderActive,{.65f,.46f,.12f,1.f});
        if(UI::Selectable((icon?"##icon"+c.id:CategoryTitle(c)+"##tab"+c.id).c_str(),category==c.id,0,{icon?tabHeight+20.f:tabWidth,tabHeight})) {category=c.id; tabsFocused=true; columnsFocused=false; tabChanged=true;}
        UI::PopStyleColor(3);
        if(category==c.id) {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax(); auto* draw=UI::GetWindowDrawList();
            UI::ImDrawListManager::AddLine(draw,{a.x+2,b.y-2},{b.x-2,b.y-2},0xFF30C8FF,3.f);

        }
        if(UI::IsItemHovered()) {
            const auto shortcut=c.id=="#head" ? "\nLT / L2" : c.id=="#accessories" ? "\nRT / R2" : "";
            const char* explanation=c.id=="#head" ? "Person icon: change only head equipment, keeping your clothes." :
                c.id=="#accessories" ? "Ring icon: add the pictured items, keeping your clothes; replace items in matching slots." :
                (c.id=="#trash" || c.id=="#legacy") ? nullptr : "Text tabs: replace the full outfit (organize with tabs).";
            if(explanation) UI::SetTooltip("%s\n%s%s",CategoryTitle(c).c_str(),Tr(explanation),shortcut);
            else UI::SetTooltip("%s%s",CategoryTitle(c).c_str(),shortcut);
        }
        if(c.id=="#accessories") {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
            auto* draw=UI::GetWindowDrawList(); const float x=(a.x+b.x)*.5f, r=tabHeight*.27f;
            UI::ImDrawListManager::AddCircle(draw,{x,a.y+tabHeight*.60f},r,0xFFE0E0E0,20,2.f);
            const float y=a.y+tabHeight*.18f,d=tabHeight*.12f;
            UI::ImDrawListManager::AddLine(draw,{x-d,y},{x,y-d},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x,y-d},{x+d,y},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+d,y},{x,y+d},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x,y+d},{x-d,y},0xFFE0E0E0,2.f);
        }
        if(c.id=="#head") {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
            auto* draw=UI::GetWindowDrawList(); const float x=(a.x+b.x)*.5f, r=tabHeight*.19f;
            UI::ImDrawListManager::AddCircle(draw,{x,a.y+tabHeight*.30f},r,0xFFE0E0E0,16,2.f);
            UI::ImDrawListManager::AddLine(draw,{x-r*1.8f,a.y+tabHeight*.85f},{x-r,a.y+tabHeight*.59f},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x-r,a.y+tabHeight*.59f},{x+r,a.y+tabHeight*.59f},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+r,a.y+tabHeight*.59f},{x+r*1.8f,a.y+tabHeight*.85f},0xFFE0E0E0,2.f);
        }
        if(c.id=="#trash") {
            const auto a=UI::GetItemRectMin(); auto* draw=UI::GetWindowDrawList();
            const float unit=tabHeight/20.f, x=a.x+7.f, y=a.y+2.f;
            UI::ImDrawListManager::AddRect(draw,{x+3*unit,y+5*unit},{x+15*unit,y+17*unit},0xFFE0E0E0,1.f,0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+1*unit,y+3*unit},{x+17*unit,y+3*unit},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+6*unit,y},{x+12*unit,y},0xFFE0E0E0,2.f);
            for(int k=7;k<=11;k+=4) UI::ImDrawListManager::AddLine(draw,{x+k*unit,y+8*unit},{x+k*unit,y+14*unit},0xFFE0E0E0,1.5f);
            if(UI::BeginPopupContextItem("Trash actions")) {
                if(UI::MenuItem(Label("Empty Trash"),nullptr,false,libraryWritable&&!saveBusy)) {
                    deletePhotos.clear(); for(const auto& [photo,label]:library.labels) if(label.deleted) deletePhotos.push_back(photo);
                    openDelete=!deletePhotos.empty();
                }
                UI::EndPopup();
            }
        }
        UI::PushStyleColor(UI::ImGuiCol_DragDropTarget,{1.f,.91f,.44f,1.f});
        if(!c.id.empty() && (!c.id.starts_with("#") || c.id=="#trash") && libraryWritable && !saveBusy && UI::BeginDragDropTarget()) {
            if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_PHOTO")) {
                if(payload->Data && payload->DataSize>1 && payload->DataSize<512 && static_cast<const char*>(payload->Data)[payload->DataSize-1]=='\0') {
                    const std::string photo(static_cast<const char*>(payload->Data));
                    auto next=library; auto& label=next.labels[photo];
                    if(c.id=="#trash") {label.deleted=true; CommitLibrary(std::move(next));}
                    else if(!label.deleted && [&]{std::scoped_lock lock(presetsMutex); return std::any_of(presets.begin(),presets.end(),[&](const auto& p){return p.photo==photo && !p.slotMask;});}()) {if(std::find(label.categories.begin(),label.categories.end(),c.id)==label.categories.end()) label.categories.push_back(c.id); CommitLibrary(std::move(next));}
                }
            }
            UI::EndDragDropTarget();
        }
        UI::PopStyleColor();
        if(!c.id.empty() && !c.id.starts_with("#") && UI::IsItemClicked(1)) {
            editCategory=c.id; newCategory=false; strcpy_s(tabName,c.name.c_str()); openTab=true;
        }
        if(category==c.id && (tabChanged || reveal)) UI::SetScrollHereX(.5f);
    }
    UI::SameLine();
    const bool cycleWithPad=browse && tabsFocused && columnsFocused && PadPressed(UI::ImGuiKey_GamepadFaceDown,false);
    if(UI::Button("##cycleColumns",{tabHeight+20.f,tabHeight}) || cycleWithPad) {gridColumns=gridColumns==5?2:gridColumns+1; SaveSettingsIfDue(true);}
    if(tabsFocused && columnsFocused) {
        UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
        if(left || right) UI::SetScrollHereX(.5f);
    }
    {
        const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
        const float side=tabHeight*.28f, gap=tabHeight*.12f;
        const float x=(a.x+b.x)*.5f-side-gap*.5f, y=(a.y+b.y)*.5f-side-gap*.5f;
        auto* draw=UI::GetWindowDrawList();
        for(int row=0;row<2;++row) for(int col=0;col<2;++col) {
            const UI::ImVec2 p{x+col*(side+gap),y+row*(side+gap)};
            UI::ImDrawListManager::AddRectFilled(draw,p,{p.x+side,p.y+side},0xFFE0E0E0,0,0);
        }
    }
    if(UI::IsItemHovered()) UI::SetTooltip(Tr("Columns: %d (click to cycle)"),gridColumns);
    UI::EndChild();
    std::vector<Preset> cards;
    {std::scoped_lock lock(presetsMutex); cards=presets;}
    std::map<std::string,int> ranks;
    for(std::size_t n=0;n<library.order.size();++n) ranks[library.order[n]]=static_cast<int>(n);
    auto rank=[&](const auto& p){const auto it=ranks.find(p.photo); return it==ranks.end()?-1:it->second;};
    std::stable_sort(cards.begin(),cards.end(),[&](const auto& a,const auto& b){return rank(a)<rank(b);});
    std::vector<std::string> fullOrder;
    for(const auto& p:cards) fullOrder.push_back(p.photo);
    for(auto& p:cards) {
        const auto it=library.labels.find(p.photo);
        if(it!=library.labels.end() && !it->second.name.empty()) p.name=it->second.name;
    }
    std::erase_if(cards,[&](const auto& p){
        const auto it=library.labels.find(p.photo);
        const bool deleted=it!=library.labels.end() && it->second.deleted;
        if(deleted!=(category=="#trash")) return true;
        if(!CollectionFits(p.slotMask,category,p.accessories)) return true;
        if(!category.empty() && !category.starts_with("#") && (it==library.labels.end() || std::find(it->second.categories.begin(),it->second.categories.end(),category)==it->second.categories.end())) return true;
        return search[0] && p.name.find(search)==std::string::npos;
    });
    if(cards.empty()) {tabsFocused=true; selected.clear();}
    const auto filter=category+"|"+search+"|"+std::to_string(gridColumns)+std::to_string(showNames)+std::to_string(showCounts);
    const bool reset=filter!=oldFilter; oldFilter=filter;
    int index=0;
    auto chosen=std::find_if(cards.begin(),cards.end(),[&](const auto& p){return p.photo==selected;});
    if(chosen!=cards.end() && !tabChanged) index=static_cast<int>(chosen-cards.begin());
    const int before=index;
    bool moved=reveal.has_value();
    bool enteredPhotos=false;
    if(browse && !cards.empty() && tabsFocused && (down || (!columnsFocused && PadPressed(UI::ImGuiKey_GamepadFaceDown,false)))) {tabsFocused=false; columnsFocused=false; index=0; moved=true; enteredPhotos=true;}
    if(browse && !cards.empty() && !tabsFocused && !enteredPhotos && !tabChanged) {
        if(down) {index+=gridColumns; moved=true;}
        if(up) {if(index<gridColumns) tabsFocused=true; else index-=gridColumns; moved=true;}
        if(left) {--index; moved=true;}
        if(right) {++index; moved=true;}


        index=std::clamp(index,0,static_cast<int>(cards.size())-1);
    }
    if(!cards.empty()) selected=cards[index].photo;
    auto queueApply=[&](const Preset& p){
        if(saveBusy || category=="#trash") return;
        std::scoped_lock lock(presetsMutex); requestedPreset=p; requestedAddMissing=addMissing; command=4;
    };
    if(browse && !cards.empty() && !tabsFocused && !enteredPhotos) {
        if(PadPressed(UI::ImGuiKey_GamepadFaceDown,false)) queueApply(cards[index]);
        if(PadPressed(UI::ImGuiKey_GamepadR3,false)) {contextPhoto=cards[index].photo; strcpy_s(editName,cards[index].name.c_str()); openEdit=true;}
    }
    if(browse && tabsFocused && !columnsFocused && PadPressed(UI::ImGuiKey_GamepadR3,false) && !category.empty() && !category.starts_with("#")) {
        auto it=std::find_if(library.categories.begin(),library.categories.end(),[&](const auto& c){return c.id==category;});
        if(it!=library.categories.end()) {newCategory=false; editCategory=category; strcpy_s(tabName,it->name.c_str()); openTab=true;}
    }
    if(browse && PadPressed(UI::ImGuiKey_GamepadFaceRight,false)) command=2;
    if(browse && RawPadPressed(capturePads[1]) && !saveBusy.exchange(true)) {
        {std::scoped_lock lock(presetsMutex); requestedName.clear();} command=5;
    }
    if(browse && RawPadPressed(capturePads[2]) && !saveBusy.exchange(true)) {
        {std::scoped_lock lock(presetsMutex); requestedName.clear();} command=6;
    }
    if(browse && RawPadPressed(capturePads[0])) {
        if(!saveBusy.exchange(true)) {
            {std::scoped_lock lock(presetsMutex); requestedName.clear();}
            command=3;
        }
    }
    const auto screen=UI::GetIO()->DisplaySize;
    const float footer=UI::GetTextLineHeightWithSpacing()*1.4f;
    UI::BeginChild("Presets",{0,std::max(80.f,UI::GetContentRegionAvail().y-footer)},0,UI::ImGuiWindowFlags_NoNavInputs);
    const float stepX=(UI::GetContentRegionAvail().x-16.f)/gridColumns;
    const float w=std::max(32.f,stepX-12.f), h=w*(screen.y*.84f)/(screen.x*.45f);
    const float textHeight=UI::GetTextLineHeightWithSpacing();
    const float stepY=h+16.f+(showNames?textHeight:0)+(showCounts?textHeight:0);
    const int totalRows=(static_cast<int>(cards.size())+gridColumns-1)/gridColumns;
    float scroll=reset?0.f:UI::GetScrollY();
    const float viewport=std::max(1.f,UI::GetWindowHeight()-16.f);
    if(reset) UI::SetScrollY(0);
    if(moved || index!=before) {
        const float y=(index/gridColumns)*stepY;
        if(y<scroll) scroll=y; else if(y+stepY>scroll+viewport) scroll=y+stepY-viewport;
        scroll=std::max(0.f,scroll); UI::SetScrollY(scroll);
    }
    const int start=std::clamp(static_cast<int>(scroll/stepY)-1,0,totalRows);
    const int finish=std::min(totalRows,start+static_cast<int>(viewport/stepY)+4);
    std::vector<std::string> visible;
    const float origin=UI::GetCursorPosY();
    UI::BeginDisabled(saveBusy);
    for(int row=start;row<finish;++row) for(int col=0;col<gridColumns;++col) {
        const int n=row*gridColumns+col; if(n>=static_cast<int>(cards.size())) break;
        const auto& p=cards[n];
        UI::SetCursorPos({8.f+col*stepX,origin+row*stepY});
        UI::BeginGroup();
        const auto path=(std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/p.photo).generic_string();
        visible.push_back(path);
        const auto texture=LoadPortraitView(path);
        if(p.photo==selected && !tabsFocused) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
        // The overlaid trash X must be able to receive hover/clicks instead of
        // the full-size image button claiming that area first.
        if(category=="#trash") UI::SetNextItemAllowOverlap();
        const bool clicked=texture?UI::ImageButton(p.photo.c_str(),texture,{w,h}):UI::Button(("Missing photo##"+p.photo).c_str(),{w,h});
        if(p.photo==selected && !tabsFocused) UI::PopStyleColor();
        const bool hover=UI::IsItemHovered();
        const auto photoMin=UI::GetItemRectMin(), photoMax=UI::GetItemRectMax();
        if(p.photo==selected && !tabsFocused) {
            auto* draw=UI::GetWindowDrawList();
            UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF171717,0,0,7.f);
            UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF70C6E8,0,0,4.f);
        }
        const auto label=library.labels.find(p.photo);
        if(!p.slotMask && label!=library.labels.end() && std::find(label->second.categories.begin(),label->second.categories.end(),"favorites")!=label->second.categories.end()) {
            auto* draw=UI::GetWindowDrawList();
            const float radius=std::clamp(w*.055f,8.f,15.f);
            const UI::ImVec2 center{photoMax.x-radius-6.f,photoMin.y+radius+6.f};
            UI::ImDrawListManager::AddCircleFilled(draw,center,radius+3.f,0xD0202020,20);
            UI::ImVec2 points[10];
            for(int k=0;k<10;++k) {const float angle=-std::numbers::pi_v<float>/2+k*std::numbers::pi_v<float>/5; const float r=k%2?radius*.43f:radius; points[k]={center.x+std::cos(angle)*r,center.y+std::sin(angle)*r};}
            for(int k=0;k<10;++k) UI::ImDrawListManager::AddLine(draw,points[k],points[(k+1)%10],0xFF55DFFF,2.f);
        }
        if(hover) {
            std::string scope;
            if(p.slotMask) for(unsigned bit=0;bit<32;++bit) if(p.slotMask&(1u<<bit)) scope+=" "+std::to_string(bit+30);
            UI::SetTooltip("%s (%u)\n%s%s",p.name.c_str(),static_cast<unsigned>(p.items.size()),Tr(p.slotMask?"Selected slots:":"Full outfit"),scope.c_str());
        }
        if(UI::IsItemClicked(1)) {contextPhoto=p.photo; strcpy_s(editName,p.name.c_str()); openEdit=true; selected=p.photo;}
        // Gamepad A is handled once above; avoid native-nav activation applying another card.
        if(clicked && !UI::GetDragDropPayload() && !PadPressed(UI::ImGuiKey_GamepadFaceDown,false)) {selected=p.photo; tabsFocused=false; columnsFocused=false; queueApply(p);}
        UI::PushStyleColor(UI::ImGuiCol_PopupBg,{.07f,.07f,.07f,1.f});
        if(libraryWritable && UI::BeginDragDropSource()) {
            UI::SetDragDropPayload("OUTFIT_PHOTO",p.photo.c_str(),p.photo.size()+1);
            UI::TextUnformatted(p.name.c_str());
            if(texture) UI::Image(texture,{w*.5f,h*.5f});
            UI::EndDragDropSource();
        }
        UI::PopStyleColor();
        if(libraryWritable && UI::BeginDragDropTarget()) {
            if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_PHOTO",UI::ImGuiDragDropFlags_AcceptBeforeDelivery|UI::ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                if(payload->Data && payload->DataSize>1 && payload->DataSize<512 && static_cast<const char*>(payload->Data)[payload->DataSize-1]=='\0') {
                    const std::string source(static_cast<const char*>(payload->Data));
                    const bool after=UI::GetIO()->MousePos.x>(photoMin.x+photoMax.x)*.5f;
                    if(source!=p.photo && payload->Preview) {
                        auto* draw=UI::GetWindowDrawList();
                        UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF171717,0,0,7.f);
                        UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF70E8FF,0,0,3.f);
                        const float x=after?photoMax.x-5:photoMin.x+5;
                        UI::ImDrawListManager::AddLine(draw,{x,photoMin.y+5},{x,photoMax.y-5},0xFF171717,10.f);
                        UI::ImDrawListManager::AddLine(draw,{x,photoMin.y+5},{x,photoMax.y-5},0xFF70E8FF,6.f);
                    }
                    if(payload->Delivery) {
                        auto next=library; next.order=fullOrder;
                        if(MovePhoto(next.order,source,p.photo,after) && CommitLibrary(std::move(next))) selected=source;
                    }
                }
            }
            UI::EndDragDropTarget();
        }
        if(category=="#trash") {
            const auto cursor=UI::GetCursorPos();
            const float size=UI::GetFrameHeight();
            UI::SetCursorScreenPos({photoMax.x-size-3.f,photoMin.y+3.f});
            UI::BeginDisabled(!libraryWritable);
            if(UI::Button(("X##delete"+p.photo).c_str(),{size,size})) {deletePhotos={p.photo}; openDelete=true;}
            UI::EndDisabled();
            UI::SetCursorPos(cursor);
        }
        if(showNames) {
            auto shortName=p.name;
            if(UI::CalcTextSize(shortName.c_str()).x>w) {
                while(!shortName.empty() && UI::CalcTextSize((shortName+"...").c_str()).x>w) {
                    auto last=shortName.size()-1;
                    while(last && (static_cast<unsigned char>(shortName[last])&0xC0)==0x80) --last;
                    shortName.resize(last);
                }
                shortName+="...";
            }
            UI::TextUnformatted(shortName.c_str());
        }
        if(showCounts) UI::Text(Tr("%u items"),static_cast<unsigned>(p.items.size()));
        UI::EndGroup();
    }
    UI::EndDisabled();
    UI::SetCursorPos({8,origin+totalRows*stepY}); UI::Dummy({1,1});
    UI::EndChild();
    for(const auto& path:loaded) if(std::find(visible.begin(),visible.end(),path)==visible.end()) DisposePortraitView(path);
    loaded=std::move(visible);
    auto current=std::find_if(cards.begin(),cards.end(),[&](const auto& p){return p.photo==selected;});
    if(current!=cards.end()) UI::Text(Tr("%s  (%u items)"),current->name.c_str(),static_cast<unsigned>(current->items.size()));
    else UI::TextUnformatted(Tr("No outfits in this view."));
    if(current!=cards.end() && current->slotMask) {UI::SameLine(); UI::TextUnformatted(Tr(current->accessories?"[Accessories]":"[Selected slots]"));}
    UI::SameLine(0,24); UI::Text(Tr("%u outfits"),static_cast<unsigned>(cards.size()));
    if(openDelete) UI::OpenPopup(Label("Confirm permanent deletion"));
    if(UI::BeginPopupModal(Label("Confirm permanent deletion"),nullptr,UI::ImGuiWindowFlags_AlwaysAutoResize)) {
        UI::TextWrapped(Tr("Permanently delete these presets and photos? This cannot be undone."));
        UI::Text(Tr("%u outfits"),static_cast<unsigned>(deletePhotos.size()));
        if(UI::Button(Label("Delete permanently"))) {
            try {
                for(const auto& photo:deletePhotos) {
                    const auto path=(std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/photo).generic_string();
                    DisposePortraitView(path);
                    DeleteTrashedFiles(photo,library,"Data/SKSE/Plugins/OutfitGallery");
                    auto next=library; next.labels.erase(photo); std::erase(next.order,photo);
                    if(!CommitLibrary(std::move(next))) break;
                }
            }catch(const std::exception& e){SetStatus(e.what());}
            try {ReloadPresets();}catch(const std::exception& e){SetStatus(e.what());}
            deletePhotos.clear(); UI::CloseCurrentPopup();
        }
        UI::SameLine();
        if(UI::Button(Label("Cancel"))) {deletePhotos.clear(); UI::CloseCurrentPopup();}
        UI::EndPopup();
    }
    if(openEdit) UI::OpenPopup("Outfit actions");
    if(UI::BeginPopup("Outfit actions")) {
        UI::InputText(Label("Name"),editName,sizeof(editName));
        if(UI::Button(Label("Rename")) && editName[0]) {auto next=library; next.labels[contextPhoto].name=editName; CommitLibrary(std::move(next));}
        UI::Separator();
        if(UI::Button(Label("New tab"))) {newCategory=true; tabName[0]=0; openTab=true; UI::CloseCurrentPopup();}
        if(!category.empty() && !category.starts_with("#") && UI::Button(Label("Edit current tab"))) {
            auto it=std::find_if(library.categories.begin(),library.categories.end(),[&](const auto& c){return c.id==category;});
            if(it!=library.categories.end()) {newCategory=false; editCategory=category; strcpy_s(tabName,it->name.c_str()); openTab=true; UI::CloseCurrentPopup();}
        }
        bool partialContext=false;
        {std::scoped_lock lock(presetsMutex); for(const auto& p:presets) if(p.photo==contextPhoto) partialContext=p.slotMask!=0;}
        if(!partialContext) UI::TextUnformatted(Tr("Categories (multiple allowed)"));
        if(!partialContext) for(const auto& c:library.categories) {
            const auto found=library.labels.find(contextPhoto);
            bool member=found!=library.labels.end() && std::find(found->second.categories.begin(),found->second.categories.end(),c.id)!=found->second.categories.end();
            if(UI::Checkbox((CategoryTitle(c)+"##"+c.id).c_str(),&member)) {
                auto next=library; auto& list=next.labels[contextPhoto].categories;
                if(member) list.push_back(c.id); else std::erase(list,c.id);
                CommitLibrary(std::move(next)); break;
            }
        }
        const auto found=library.labels.find(contextPhoto);
        const bool deleted=found!=library.labels.end()&&found->second.deleted;
        if(UI::Button(Label(deleted?"Restore from Trash":"Move to Trash"))) {auto next=library; next.labels[contextPhoto].deleted=!deleted; if(CommitLibrary(std::move(next))) UI::CloseCurrentPopup();}
        if(UI::Button(Label("Close"))) UI::CloseCurrentPopup();
        UI::EndPopup();
    }
    if(openTab) UI::OpenPopup("Edit category");
    if(UI::BeginPopup("Edit category")) {
        UI::InputText(Label("Tab name"),tabName,sizeof(tabName));
        if(UI::Button(Label("Save tab"))&&tabName[0]) {
            auto next=library;
            if(newCategory) {const auto id="tab-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()); next.categories.push_back({id,tabName});}
            else for(auto& c:next.categories) if(c.id==editCategory) c.name=tabName;
            if(CommitLibrary(std::move(next))) UI::CloseCurrentPopup();
        }
        if(!newCategory && UI::Button(Label("Remove tab (keep outfits)"))) {
            auto next=library; std::erase_if(next.categories,[&](const auto& c){return c.id==editCategory;});
            for(auto& [key,label]:next.labels) std::erase(label.categories,editCategory);
            if(CommitLibrary(std::move(next))) {category.clear(); UI::CloseCurrentPopup();}
        }
        if(UI::Button(Label("Cancel"))) UI::CloseCurrentPopup();
        UI::EndPopup();
    }
}




