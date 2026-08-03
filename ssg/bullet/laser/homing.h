///
/// LaserHoming - Snake-like homing laser
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "bullet/bullet_common.h"
#include "gfx/core/coords.h"

namespace audio {
class AudioSystem;
}

enum class HomingType : uint8_t {
  None,
  Type1,
};

// ── Spawn parameter struct ─────────────────────────────────────
struct HomingSpawnInfo {
  WorldCoord x{}, y{};
  float angle{};
  int dw{};
  int n{};
  uint8_t c{};
  HomingType type{HomingType::None};

  // Per-bullet context (set by manager before Spawn)
  int bullet_index{};
};

// ── Pool / segment constants ────────────────────────────────────
inline constexpr auto kHomingCapacity = 162;
inline constexpr auto kHomingTrailLength = 7;
inline constexpr auto kHomingSection = 4;

enum class HomingState : uint8_t {
  Normal = 0x00,
  Dead = 0xff,
};

struct HomingUpdateInfo {
  WorldCoord player_x, player_y;
};

// ── LaserHoming ─────────────────────────────────────────────────
struct LaserHoming {
  using SpawnInfo = HomingSpawnInfo;
  using UpdateInfo = HomingUpdateInfo;

  void Render() const;
  [[nodiscard]] bool IsDead() const;
  void Kill();
  void Spawn(const HomingSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(WorldCoord px, WorldCoord py,
                                   WorldCoord player_radius) const;
  void Update(audio::AudioSystem &audio, const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

private:
  struct TrailPoint {
    float x{};
    float y{};
    float angle{};
  };

  float v_{};
  uint8_t c_{};
  int count_{};

  int current_{};
  float a_{};
  uint8_t left_{};
  std::array<TrailPoint,
             static_cast<size_t>(kHomingTrailLength *kHomingSection)>
      p_{};

  HomingType subtype_{HomingType::None};
  HomingState state_{HomingState::Normal};
};
