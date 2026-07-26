///
/// BulletBase — Abstract base templated on SpawnInfo and UpdateInfo
///

#pragma once

#include <cstdint>

// ── Hit-test result ────────────────────────────────────────────
enum class HitResult : uint8_t { Miss, Graze, Hit };
inline constexpr auto kBulletEvadeValue = 1;

// ── Empty placeholder for types with no update info / result ───
struct Empty {
  using UpdateResult = Empty;
};

// ── BulletBase ─────────────────────────────────────────────────
template <typename SI, typename UI = Empty> struct BulletBase {
  using SpawnInfo = SI;
  using UpdateInfo = UI;
  using UpdateResult = typename UI::UpdateResult;

  BulletBase() = default;
  BulletBase(const BulletBase &) = delete;
  BulletBase(BulletBase &&) = delete;
  BulletBase &operator=(const BulletBase &) = delete;
  BulletBase &operator=(BulletBase &&) = delete;
  virtual ~BulletBase() = default;

  virtual void Render() const = 0;
  virtual bool IsDead() const = 0;
  virtual void Kill() = 0;
  virtual void Spawn(const SI &info) = 0;
  virtual HitResult CheckHit(int player_x, int player_y,
                             int player_radius) const = 0;
  virtual UpdateResult Update(const UI &info = {}) = 0;
  virtual void RenderDebugHitbox(int mode) const = 0;

  [[nodiscard]] int X() const { return x_; }
  [[nodiscard]] int Y() const { return y_; }
  [[nodiscard]] uint8_t Dir() const { return d_; }

protected:
  int x_{};
  int y_{};
  int v_{};
  uint8_t d_{};
  uint8_t c_{};
  uint32_t count_{};
};
