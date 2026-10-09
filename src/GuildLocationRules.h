#pragma once
#include "AutoOutfitPolicy.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Gallery::AutoOutfit {
struct GuildLocationRule {std::string_view plugin;std::uint32_t localID;Place place;std::uint32_t requiredCell{};};
inline bool GuildCellAllowed(const GuildLocationRule& rule,bool resolvedCellMatches) {
 return !rule.requiredCell || resolvedCellMatches;
}
inline constexpr std::array<GuildLocationRule,14> GuildLocations{{
 {"Dawnguard.esm",0x0128FE,Place::Dawnguard},
 {"Skyrim.esm",0x01EB7A,Place::College},
 {"Skyrim.esm",0x01EB79,Place::College},
 {"Skyrim.esm",0x01EB74,Place::College},
 {"Skyrim.esm",0x0D16E5,Place::College},
 {"Skyrim.esm",0x0D16E6,Place::College},
 {"Skyrim.esm",0x03444E,Place::Brotherhood},
 {"Skyrim.esm",0x019429,Place::Brotherhood},
 {"Skyrim.esm",0x01F877,Place::Companions},
 {"Skyrim.esm",0x023E5D,Place::Companions},
 {"Skyrim.esm",0x020082,Place::Bards},
 {"Skyrim.esm",0x02264A,Place::Thieves},
 {"Skyrim.esm",0x022650,Place::Thieves},
 {"Skyrim.esm",0x020086,Place::BluePalace,0x016A04}
}};
// Reject null/unresolved IDs and all exterior locations; nearest matching ancestor wins.
template<class Location,class Parent,class Match>
Place GuildForLocation(bool interior,Location* location,Parent parent,Match match) {
 if(!interior) return Place::Normal;
 for(unsigned depth=0;location && depth<32;++depth,location=parent(location)) {
  const auto place=match(location);
  if(IsGuild(place)) return place;
 }
 return Place::Normal;
}
}
