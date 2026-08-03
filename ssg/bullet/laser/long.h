///
/// LaserLong - Infinite-distance beam anchored to an enemy
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "bullet/bullet_common.h"
#include "gfx/graphics.h"
#include "gfx/render/geometry.h"

struct EnemyActor;

// ── Pool capacity ────────────────────────────────────────────────
inline constexpr auto kLongLaserCapacity = 20;

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
  std::size_t enemy_id{};
  WorldCoord dx{};
  WorldCoord dy{};
  WorldCoord v{};
  WorldCoord w{};
  float angle{};
  uint8_t c{};
  LongLaserType type{LongLaserType::Long};
  WorldCoord player_x{};
  WorldCoord player_y{};
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
  float angle = 0.0F;
  float delta = 0.0F;
};

// ── LaserLong ────────────────────────────────────────────────────
struct LaserLong {
  using SpawnInfo = LongLaserSpawnInfo;
  using UpdateInfo = LongLaserUpdateInfo;

  friend struct LaserReflect; // for reflection on long lasers, must keep

  void Render() const;
  [[nodiscard]] bool IsDead() const;
  void Kill();
  void Spawn(const LongLaserSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(WorldCoord px, WorldCoord py,
                                   WorldCoord player_radius) const;
  void Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

  [[nodiscard]] WorldCoord X() const;
  [[nodiscard]] bool BelongsTo(const EnemyActor *enemy, std::size_t id) const;

private:
  void MarkDead() {
    state_ = LongState::Inactive;
    e_ = nullptr;
  }

  float x_{};
  float y_{};
  float v_{};
  float angle_{};
  uint8_t c_{};
  int count_{};

  const EnemyActor *e_{};
  float dx_{};
  float dy_{};
  float lx_{};
  float ly_{};
  float infx_{};
  float infy_{};
  float wx_{};
  float wy_{};
  float w_{};
  float wmax_{};
  std::array<VertexXy, 4> p_{};
  std::size_t enemy_id_{};

  LongLaserType subtype_{LongLaserType::Long};
  LongState state_{LongState::Inactive};

  void RecalcGeometry();
  void UpdateOpening();
  void UpdateClosing();
  void TickUpdate();
  void ApplyCommand(LongLaserUpdateInfo::Command cmd, float angle, float delta);
  void FixAngleGeometry();

  void DrawBeam() const;
  void DrawPreviewLine() const;
};
