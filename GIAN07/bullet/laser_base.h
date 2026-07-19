///
/// LaserBase — Abstract base templated on SpawnInfo
///

#pragma once

#include <cstdint>

// ── Hit-test result ────────────────────────────────────────────
enum class HitResult : uint8_t { Miss, Graze, Hit };
inline constexpr auto kLaserEvadeValue = 1;

// ── LaserBase ───────────────────────────────────────────────────
template <typename SI> struct LaserBase {
  LaserBase(const LaserBase &) = delete;
  LaserBase(LaserBase &&) = delete;
  LaserBase &operator=(const LaserBase &) = delete;
  LaserBase &operator=(LaserBase &&) = delete;
  virtual ~LaserBase() = default;

  virtual void Render() const = 0;
  virtual bool IsDead() const = 0;
  virtual void Kill() = 0;
  virtual void Spawn(const SI &info) = 0;
  virtual HitResult CheckHit(int player_x, int player_y) const = 0;

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
