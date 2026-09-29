#pragma once
namespace Gallery {
void InitializeManagedItems();
void RegisterManagedItemEvents();
void BeginManagedAddition();
void AddManagedItems(RE::PlayerCharacter*, RE::TESBoundObject*, int count, bool previouslyAbsent);
void TrackNewManagedItems();
void ReclaimManagedItems(); // game thread, only after a verified successful change
}
