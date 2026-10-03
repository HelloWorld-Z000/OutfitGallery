#pragma once
#include "PreferredItem.h"
namespace Gallery {
PreferredItem RememberEnchanted(RE::Actor*, RE::ExtraDataList*);
std::uint64_t PreferredScope(std::uint16_t id);
void InitializeManagedItems();
void RegisterManagedItemEvents();
void BeginManagedAddition(RE::Actor*);
void AddManagedItems(RE::Actor*, RE::TESBoundObject*, int count, bool previouslyAbsent);
void TrackNewManagedItems(RE::Actor*);
void ReclaimManagedItems(RE::Actor*); // game thread, only after a verified successful change
}
