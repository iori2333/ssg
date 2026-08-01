///
/// LaserReflect - Short / reflective laser
///

#pragma once

#include <cstdint>
#include <span>

#include "bullet/bullet_common.h"
#include "gfx/graphics_backend.h"

struct LaserLong;

enum class ReflectLaserType : uint8_t {
  Short = 0x00,
  Reflect = 0x01,
};

// ── Spawn parameter struct ────────────────────────────────────
struct ReflectSpawnInfo {
  bool no_scaling = false;
  int x{}, y{};
  float v{};
  int w{}, l{}, l2{};
  float angle{};
  uint8_t dw{};
  uint8_t n{}, c{};
  bool aimed{};
  BulletPattern pattern{BulletPattern::Spread};
  ReflectLaserType type{ReflectLaserType::Short};

  // Per-bullet context (set by manager before Spawn)
  int bullet_index{};
  float base_angle{};
};

// ── State machine ─────────────────────────────────────────────
enum class ReflectState : uint8_t {
  Growing,
  Flying,
  Shooting,
  Reflected,
  Clearing,
  Dead,
};

// ── Pool capacity ──────────────────────────────────────────────
inline constexpr auto kReflectCapacity = 1000;

// ── Update info ────────────────────────────────────────────────
struct ReflectUpdateInfo {
  std::span<const LaserLong *> longs;

  struct UpdateResult {
    bool spawn_requested = false;
    ReflectSpawnInfo spawn_info;
  };
};

// ── LaserReflect ───────────────────────────────────────────────
struct LaserReflect {
  using SpawnInfo = ReflectSpawnInfo;
  using UpdateInfo = ReflectUpdateInfo;
  using UpdateResult = UpdateInfo::UpdateResult;

  void Render() const;
  [[nodiscard]] bool IsDead() const;
  void Kill();
  void Spawn(const ReflectSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

  [[nodiscard]] int X() const;
  [[nodiscard]] int Y() const;
  [[nodiscard]] bool RegisterGraze();

private:
  void MarkDead() { state_ = ReflectState::Dead; }

  float x_{};
  float y_{};
  float v_{};
  float angle_{};
  uint8_t c_{};
  uint32_t count_{};
  bool grazed_{};

  float vx_{};
  float vy_{};
  float lx_{};
  float ly_{};
  float wx_{};
  float wy_{};
  VertexXy p_[4]{};

  float w_{};
  float wmax_{};
  float l_{};
  float lmax_{};

  ReflectLaserType subtype_{ReflectLaserType::Short};
  ReflectState state_{ReflectState::Dead};

  void SetupGeometry();

  void UpdateGrowing();
  [[nodiscard]] UpdateResult UpdateFlying(std::span<const LaserLong *> longs);
  [[nodiscard]] UpdateResult UpdateShooting(std::span<const LaserLong *> longs);
  void UpdateReflected();
  void UpdateClearing();

  [[nodiscard]] static UpdateResult CheckLongLaser(const LaserReflect &self,
                                                   const LaserLong &ll,
                                                   float dx, float dy);

  void DrawOuter() const;
  void DrawClearing() const;
};
