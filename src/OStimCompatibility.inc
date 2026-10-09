// Game-thread read-only bridge to OStim Standalone 7.4a's published faction lifecycle.
// See OSTIM-COMPATIBILITY.md for exact upstream sources and limits.
std::atomic<int> ostimRecoverySeconds{10};
RE::TESFaction* ostimSceneFaction{}; // resolved once after data load, never serialized
bool ostimInstalled{}, ostimSupported{};
AutoOutfit::SceneSuspend ostimSuspend; // game thread only
std::atomic<AutoOutfit::SceneState> ostimSceneState{AutoOutfit::SceneState::NotInstalled};
const char* OStimStateLabel(AutoOutfit::SceneState state) {
 switch(state) {
 case AutoOutfit::SceneState::NotInstalled:return "OStim not installed";
 case AutoOutfit::SceneState::Idle:return "OStim protection ready";
 case AutoOutfit::SceneState::Active:return "OStim scene: automatic outfits suspended";
 case AutoOutfit::SceneState::Recovering:return "Waiting after OStim";
 default:return "OStim detection unavailable: scene protection inactive; automatic outfits continue";
 }
}
void InitializeOStimCompatibility() {
 auto* data=RE::TESDataHandler::GetSingleton();
 const auto* file=FindActivePlugin(data,"OStim.esp");
 const bool plugin=file!=nullptr;
 const bool module=GetModuleHandleW(L"OStim.dll")!=nullptr;
 ostimInstalled=plugin || module;
 ostimSceneFaction=plugin?data->LookupForm<RE::TESFaction>(0xD93,"OStim.esp"):nullptr;
 ostimSupported=plugin && module && ostimSceneFaction && !ostimSceneFaction->IsDeleted();
 ostimSuspend={};
 ostimSceneState=ostimSuspend.Observe(ostimInstalled,ostimSupported,false,GetTickCount64());
 SKSE::log::info("OG-AUTO OStim bridge installed={} supported={} pluginLoaded={} moduleLoaded={} compileIndex={} lightIndex={} faction={:08X} factionDeleted={}; read-only faction lifecycle, no native ABI calls",
  ostimInstalled,ostimSupported,plugin,module,file?static_cast<int>(file->compileIndex):-1,
  file?static_cast<int>(file->smallFileCompileIndex):-1,ostimSceneFaction?ostimSceneFaction->GetFormID():0,
  ostimSceneFaction && ostimSceneFaction->IsDeleted());
 if(ostimInstalled && !ostimSupported)
  SKSE::log::warn("OG-AUTO OStim scene protection unavailable; automatic outfits remain enabled by user choice. Use manual pause during scenes.");
}
bool OStimBlocksAutomatic(RE::PlayerCharacter* player) {
 // Membership, not a positive rank: excitement rank zero is still an active participant.
 const bool member=ostimSupported && player && player->IsInFaction(ostimSceneFaction);
 const auto state=ostimSuspend.Observe(ostimInstalled,ostimSupported,member,GetTickCount64(),static_cast<std::uint64_t>(ostimRecoverySeconds.load())*1000);
 if(ostimSceneState.exchange(state)!=state)
  SKSE::log::info("OG-AUTO OStim state={} playerMember={} recoverySeconds={}",OStimStateLabel(state),member,ostimRecoverySeconds.load());
 return AutoOutfit::SceneBlocks(state);
}
