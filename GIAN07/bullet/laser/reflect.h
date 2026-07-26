///
/// LaserReflect - Short / reflective laser
///

#pragma once

#include <optional>
#include <span>

#include "bullet/bullet_base.h"
#include "gfx/graphics_backend.h"

struct LaserLong;

// ── Backward-compat ECL command struct (used by EnemyActor) ──────
struct LaserCommand {
  int x{}, y{};
  int v{};
  int w{}, l{}, l2{};
  uint8_t d{}, dw{};
  uint8_t n{}, c{};
  char a{};
  uint8_t cmd{}, type{}, notr{};
};

// ── Spawn parameter struct ────────────────────────────────────
struct ReflectSpawnInfo {
  bool no_scaling = false;
  int x{}, y{};
  int v{};
  int w{}, l{}, l2{};
  uint8_t d{}, dw{};
  uint8_t n{}, c{};
  char a{};
  uint8_t cmd{}, cmd_type{};

  // Per-bullet context (set by manager before Spawn)
  int bullet_index{};
  uint8_t base_deg{};
};

// ── Laser type discriminator ───────────────────────────────────
enum class ReflectLaserType : uint8_t {
  Short = 0x00,
  Reflect = 0x01,
};

// ── State machine ─────────────────────────────────────────────
enum class ReflectState : uint8_t {
  Idle = 0x00,
  Growing = 0x01,
  Flying = 0x02,
  Shooting = 0x03,
  Reflected = 0x04,
  NoMove = 0x05,
  Clearing = 0x06,
  Dead = 0x07,
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
struct LaserReflect : BulletBase<ReflectSpawnInfo, ReflectUpdateInfo> {
  using SpawnInfo = ReflectSpawnInfo;
  using UpdateInfo = ReflectUpdateInfo;

  friend struct BulletManager;

  void Render() const override;
  bool IsDead() const override;
  void Kill() override;
  void Spawn(const ReflectSpawnInfo &info) override;
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const override;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {}) override;
  void RenderDebugHitbox(int mode) const override;

private:
  void MarkDead() { state_ = ReflectState::Dead; }

  int vx_{};
  int vy_{};
  int lx_{};
  int ly_{};
  int wx_{};
  int wy_{};
  VERTEX_XY p_[4]{};

  char a_{};
  int w_{};
  int wmax_{};
  int l_{};
  int lmax_{};
  int ltemp_{};

  ReflectLaserType subtype_{ReflectLaserType::Short};
  ReflectState state_{ReflectState::Idle};

  void SetupGeometry();

  void UpdateGrowing();
  [[nodiscard]] UpdateResult UpdateFlying(std::span<const LaserLong *> longs);
  [[nodiscard]] UpdateResult UpdateShooting(std::span<const LaserLong *> longs);
  void UpdateReflected();
  void UpdateNoMove();
  void UpdateClearing();

  [[nodiscard]] static UpdateResult
  CheckLongLaser(const LaserReflect &self, const LaserLong &ll, int dx, int dy);

  void DrawOuter() const;
  void DrawClearing() const;
};
