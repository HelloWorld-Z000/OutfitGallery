#pragma once
namespace Gallery {
void ClearFollowerOutfits();
void SaveFollowerOutfits(SKSE::SerializationInterface*);
bool LoadFollowerOutfit(SKSE::SerializationInterface*,std::uint32_t type,std::uint32_t version,std::uint32_t length);
bool IsFollowerOutfitMaintained(RE::Actor*); // game thread
void RememberFollowerOutfit(RE::Actor*); // current complete armor set, no weapons
void ReleaseFollowerOutfit(RE::Actor*);
void RefreshFollowerOutfit(RE::Actor*); // only when already opted in
bool FollowerMaintenanceDue(); // safe to read from render/input threads
void MaintainFollowerOutfits(); // game thread only, studio closed
Preset DefaultFollowerOutfit(RE::Actor*); // validate before any inventory change
}
