///
/// LaserLong - Infinite-distance beam anchored to an enemy
///

#pragma once

#include <span>

#include "../laser_base.h"

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

// ── LaserLong ────────────────────────────────────────────────────
struct LaserLong : LaserBase<LongLaserSpawnInfo> {
  using SpawnInfo = LongLaserSpawnInfo;

  void Render() const override;
  bool IsDead() const override;
  void Kill() override;
  void Spawn(const LongLaserSpawnInfo &info) override;
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y) const override;

  void Update();

  void Open();
  void Close();
  void CloseToLine();
  void ForceClose();
  void SetAngle(uint8_t angle);
  void AdjustAngle(int8_t delta);
  void SetEnemyGone();
  [[nodiscard]] bool BelongsTo(const EnemyData *e, uint8_t id) const;

  void RecalcGeometry();

  [[nodiscard]] LongState State() const { return state_; }
  [[nodiscard]] int W() const { return w_; }
  [[nodiscard]] std::span<const VERTEX_XY, 4> P() const { return p_; }
  [[nodiscard]] int WX() const { return wx_; }
  [[nodiscard]] int WY() const { return wy_; }
  [[nodiscard]] int LX() const { return lx_; }
  [[nodiscard]] int LY() const { return ly_; }
  [[nodiscard]] int InfX() const { return infx_; }
  [[nodiscard]] int InfY() const { return infy_; }
  [[nodiscard]] bool IsReflectable() const {
    return state_ == LongState::Active;
  }

  void MarkDead() {
    state_ = LongState::Inactive;
    e_ = nullptr;
  }

private:
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

  void UpdateOpening();
  void UpdateClosing();
  void DrawBeam() const;
  void DrawPreviewLine() const;
};
