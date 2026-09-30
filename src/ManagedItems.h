#pragma once
namespace Gallery {
void InitializeManagedItems();
void RegisterManagedItemEvents();
void BeginManagedAddition(RE::Actor*);
void AddManagedItems(RE::Actor*, RE::TESBoundObject*, int count, bool previouslyAbsent);
void TrackNewManagedItems(RE::Actor*);
void ReclaimManagedItems(RE::Actor*); // game thread, only after a verified successful change
}
