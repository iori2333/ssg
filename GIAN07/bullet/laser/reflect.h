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
  int v{};
  int w{}, l{}, l2{};
  uint8_t d{}, dw{};
  uint8_t n{}, c{};
  bool aimed{};
  BulletPattern pattern{BulletPattern::Spread};
  ReflectLaserType type{ReflectLaserType::Short};

  // Per-bullet context (set by manager before Spawn)
  int bullet_index{};
  uint8_t base_deg{};
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
inline constexpr auto kReflectMax = 1000;

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
  bool IsDead() const;
  void Kill();
  void Spawn(const ReflectSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

  [[nodiscard]] int X() const { return x_; }
  [[nodiscard]] int Y() const { return y_; }
  [[nodiscard]] bool RegisterGraze();

private:
  void MarkDead() { state_ = ReflectState::Dead; }

  int x_{};
  int y_{};
  int v_{};
  uint8_t d_{};
  uint8_t c_{};
  uint32_t count_{};
  bool grazed_{};

  int vx_{};
  int vy_{};
  int lx_{};
  int ly_{};
  int wx_{};
  int wy_{};
  VERTEX_XY p_[4]{};

  int w_{};
  int wmax_{};
  int l_{};
  int lmax_{};

  ReflectLaserType subtype_{ReflectLaserType::Short};
  ReflectState state_{ReflectState::Dead};

  void SetupGeometry();

  void UpdateGrowing();
  [[nodiscard]] UpdateResult UpdateFlying(std::span<const LaserLong *> longs);
  [[nodiscard]] UpdateResult UpdateShooting(std::span<const LaserLong *> longs);
  void UpdateReflected();
  void UpdateClearing();

  [[nodiscard]] static UpdateResult
  CheckLongLaser(const LaserReflect &self, const LaserLong &ll, int dx, int dy);

  void DrawOuter() const;
  void DrawClearing() const;
};
