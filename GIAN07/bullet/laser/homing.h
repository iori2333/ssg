///
/// LaserHoming - Snake-like homing laser
///

#pragma once

#include <cstdint>

#include "bullet/bullet_common.h"

enum class HomingType : uint8_t {
  None,
  Type1,
};

// ── Spawn parameter struct ─────────────────────────────────────
struct HomingSpawnInfo {
  int x{}, y{};
  uint8_t d{};
  uint8_t dw{};
  uint8_t n{};
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
  int player_x, player_y;
};

// ── LaserHoming ─────────────────────────────────────────────────
struct LaserHoming {
  using SpawnInfo = HomingSpawnInfo;
  using UpdateInfo = HomingUpdateInfo;

  void Render() const;
  [[nodiscard]] bool IsDead() const;
  void Kill();
  void Spawn(const HomingSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const;
  void Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

private:
  struct TrailPoint {
    int x{};
    int y{};
    uint8_t d{};
  };

  int v_{};
  uint8_t c_{};
  uint32_t count_{};

  int current_{};
  int a_{};
  uint8_t left_{};
  TrailPoint p_[kHomingTrailLength * kHomingSection]{};

  HomingType subtype_{HomingType::None};
  HomingState state_{HomingState::Normal};
};
