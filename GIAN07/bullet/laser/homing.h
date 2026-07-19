///
/// LaserHoming - Snake-like homing laser
///

#pragma once

#include "../laser_base.h"

#include "core/point.h"

// ── Spawn parameter struct ─────────────────────────────────────
struct HomingSpawnInfo {
  int x{}, y{};
  uint8_t d{};
  uint8_t dw{};
  uint8_t n{};
  uint8_t c{};
  uint8_t type{};

  // Per-bullet context (set by manager before Spawn)
  int bullet_index{};
};

// ── Pool / segment constants ────────────────────────────────────
inline constexpr auto kHomingMax = 162;
inline constexpr auto kHomingLen = 7;
inline constexpr auto kHomingSection = 4;

// ── Type / state enums ──────────────────────────────────────────
enum class HomingType : uint8_t {
  None = 0,
  Type1 = 1,
};

enum class HomingState : uint8_t {
  Normal = 0x00,
  Dead = 0xff,
};

// ── LaserHoming ─────────────────────────────────────────────────
struct LaserHoming : LaserBase<HomingSpawnInfo> {
  using SpawnInfo = HomingSpawnInfo;

  void Render() const override;
  bool IsDead() const override;
  void Kill() override;
  void Spawn(const HomingSpawnInfo &info) override;
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y) const override;

  void Update();

  [[nodiscard]] HomingState State() const { return state_; }
  [[nodiscard]] int Current() const { return current_; }
  [[nodiscard]] const DegPoint *Segments() const { return p_; }

  void MarkDead() { state_ = HomingState::Dead; }

private:
  int current_{};
  int a_{};
  uint8_t left_{};
  DegPoint p_[kHomingLen * kHomingSection]{};

  HomingType subtype_{HomingType::None};
  HomingState state_{HomingState::Normal};
};
