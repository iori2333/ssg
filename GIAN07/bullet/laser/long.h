///
/// LaserLong - Infinite-distance beam anchored to an enemy
///

#pragma once

#include <span>

#include "../bullet_base.h"

#include "enemy/enemy.h"
#include "gfx/graphics_backend.h"

// ── Pool capacity ────────────────────────────────────────────────
inline constexpr auto kLongLaserMax = 20;

// ── Laser type discriminator ────────────────────────────────────
enum class LongLaserType : uint8_t {
  Long = 0x00,
  LongY = 0x01,
  SetDeg = 0x02,
  LongZ = 0x03,
};

// ── Spawn parameter struct ──────────────────────────────────────
struct LongLaserSpawnInfo {
  const EnemyData *enemy{};
  uint8_t enemy_id{};
  int dx{};
  int dy{};
  int v{};
  int w{};
  uint8_t d{};
  uint8_t c{};
  LongLaserType type{LongLaserType::Long};
  int player_x{};
  int player_y{};
};

// ── State machine ────────────────────────────────────────────────
enum class LongState : uint8_t {
  Inactive = 0x00,
  Active = 0x01,
  Opening = 0x02,
  Closing = 0x04,
  ClosingToLine = 0x08,
  Line = 0x10,
};

// ── Update info (per-frame tick or external control command) ─────
struct LongLaserUpdateInfo {
  enum class Command : uint8_t {
    Tick,
    Open,
    Close,
    CloseToLine,
    ForceClose,
    SetAngle,
    AdjustAngle,
    SetEnemyGone,
  };
  Command command = Command::Tick;
  uint8_t angle = 0;
  int8_t delta = 0;

  struct UpdateResult {};
};

// ── LaserLong ────────────────────────────────────────────────────
struct LaserLong : BulletBase<LongLaserSpawnInfo, LongLaserUpdateInfo> {
  using SpawnInfo = LongLaserSpawnInfo;
  using UpdateInfo = LongLaserUpdateInfo;

  friend struct BulletManager;
  friend struct LaserReflect; // for reflection on long lasers, must keep

  void Render() const override;
  bool IsDead() const override;
  void Kill() override;
  void Spawn(const LongLaserSpawnInfo &info) override;
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y) const override;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {}) override;
  void RenderDebugHitbox(int mode) const override;

private:
  [[nodiscard]] bool BelongsTo(const EnemyData *e, uint8_t id) const;

  void MarkDead() {
    state_ = LongState::Inactive;
    e_ = nullptr;
  }

  const EnemyData *e_{};
  int dx_{};
  int dy_{};
  int lx_{};
  int ly_{};
  int infx_{};
  int infy_{};
  int wx_{};
  int wy_{};
  int w_{};
  int wmax_{};
  VERTEX_XY p_[4]{};
  uint8_t enemy_id_{};

  LongLaserType subtype_{LongLaserType::Long};
  LongState state_{LongState::Inactive};

  void RecalcGeometry();
  void UpdateOpening();
  void UpdateClosing();
  void TickUpdate();
  void ApplyCommand(LongLaserUpdateInfo::Command cmd, uint8_t angle, int8_t delta);
  void FixAngleGeometry();

  void DrawBeam() const;
  void DrawPreviewLine() const;
};
