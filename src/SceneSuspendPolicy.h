#pragma once
#include <cstdint>
namespace Gallery::AutoOutfit {
enum class SceneState { NotInstalled, Idle, Active, Recovering, Unavailable };
// An unavailable optional integration must not disable ordinary automatic outfits.
inline bool SceneBlocks(SceneState s) {return s==SceneState::Active || s==SceneState::Recovering;}
// Independent of user pause and outfit policy: never re-enable or rearm an outfit.
struct SceneSuspend {
 bool wasActive{};
 std::uint64_t resumeAt{};
 SceneState Observe(bool installed,bool supported,bool active,std::uint64_t now,std::uint64_t recoveryMs=10000) {
  if(!installed){*this={};return SceneState::NotInstalled;}
  if(!supported){*this={};return SceneState::Unavailable;}
  if(active){wasActive=true;resumeAt=0;return SceneState::Active;}
  if(wasActive){wasActive=false;resumeAt=now+recoveryMs;}
  return now<resumeAt?SceneState::Recovering:SceneState::Idle;
 }
};
}
