///
/// LaserLong - Infinite-distance beam anchored to an enemy
///

#pragma once

#include <cstdint>

#include "bullet/bullet_common.h"
#include "gfx/graphics_backend.h"

struct EnemyActor;

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
  const EnemyActor *enemy{};
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
  Inactive,
  Active,
  Opening,
  Closing,
  ClosingToLine,
  Line,
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
  };
  Command command = Command::Tick;
  uint8_t angle = 0;
  int8_t delta = 0;
};

// ── LaserLong ────────────────────────────────────────────────────
struct LaserLong {
  using SpawnInfo = LongLaserSpawnInfo;
  using UpdateInfo = LongLaserUpdateInfo;

  friend struct LaserReflect; // for reflection on long lasers, must keep

  void Render() const;
  bool IsDead() const;
  void Kill();
  void Spawn(const LongLaserSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const;
  void Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

  [[nodiscard]] int X() const { return x_; }
  [[nodiscard]] bool BelongsTo(const EnemyActor *enemy, uint8_t id) const;

private:
  void MarkDead() {
    state_ = LongState::Inactive;
    e_ = nullptr;
  }

  int x_{};
  int y_{};
  int v_{};
  uint8_t d_{};
  uint8_t c_{};
  uint32_t count_{};

  const EnemyActor *e_{};
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
  void ApplyCommand(LongLaserUpdateInfo::Command cmd, uint8_t angle,
                    int8_t delta);
  void FixAngleGeometry();

  void DrawBeam() const;
  void DrawPreviewLine() const;
};
