///
/// Inline math and random utilities
///
#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>
#include <random>

namespace math {

struct RandomState {
  uint32_t seed;
  uint64_t draw_count;
};

namespace detail {

class Rng {
public:
  explicit Rng(uint32_t seed = 0) noexcept : engine_(seed), seed_(seed) {}

  void Seed(uint32_t seed) {
    engine_.seed(seed);
    seed_ = seed;
    draw_count_ = 0;
  }

  [[nodiscard]] RandomState CaptureState() const {
    return {.seed = seed_, .draw_count = draw_count_};
  }

  void RestoreState(RandomState state) {
    engine_.seed(state.seed);
    engine_.discard(state.draw_count);
    seed_ = state.seed;
    draw_count_ = state.draw_count;
  }

  [[nodiscard]] int Next() {
    draw_count_++;
    return static_cast<int>(engine_() >> 1);
  }

private:
  std::mt19937 engine_;
  uint32_t seed_ = 0;
  uint64_t draw_count_ = 0;
};

inline Rng &GlobalRng() {
  static Rng rng;
  return rng;
}

} // namespace detail

struct Vector2F {
  float x;
  float y;
};

struct Vector2I {
  int x;
  int y;
};

inline constexpr float kFullAngle = std::numbers::pi_v<float> * 2.0F;
inline constexpr float kLegacyAngleStep = kFullAngle / 256.0F;

[[nodiscard]] inline float AngleFromLegacy(uint8_t angle) {
  return static_cast<float>(angle) * kLegacyAngleStep;
}

[[nodiscard]] inline float NormalizeAngle(float angle) {
  angle = std::fmod(angle, kFullAngle);
  return angle < 0.0F ? angle + kFullAngle : angle;
}

[[nodiscard]] inline uint8_t AngleToLegacy(float angle) {
  const auto scaled =
      static_cast<int>(std::lround(NormalizeAngle(angle) / kLegacyAngleStep));
  return static_cast<uint8_t>(scaled & 0xff);
}

[[nodiscard]] inline float ShortestAngleDelta(float target, float current) {
  return std::remainder(target - current, kFullAngle);
}

[[nodiscard]] inline float AngleTo(float x, float y) {
  return std::atan2(y, x);
}

[[nodiscard]] inline Vector2F PolarVector(float angle, float length) {
  return {.x = std::cos(angle) * length, .y = std::sin(angle) * length};
}

[[nodiscard]] inline Vector2F RotateVector(float angle, float x, float y) {
  const auto angle_cos = std::cos(angle);
  const auto angle_sin = std::sin(angle);
  return {.x = angle_cos * x - angle_sin * y,
          .y = angle_sin * x + angle_cos * y};
}

[[nodiscard]] inline Vector2I RoundedPolarVector(float angle, float length) {
  const auto vector = PolarVector(angle, length);
  return {.x = static_cast<int>(std::lround(vector.x)),
          .y = static_cast<int>(std::lround(vector.y))};
}

[[nodiscard]] inline Vector2I RoundedRotateVector(float angle, float x,
                                                  float y) {
  const auto vector = RotateVector(angle, x, y);
  return {.x = static_cast<int>(std::lround(vector.x)),
          .y = static_cast<int>(std::lround(vector.y))};
}

// Random numbers
inline void SeedRandom(uint32_t seed) { detail::GlobalRng().Seed(seed); }

[[nodiscard]] inline RandomState CaptureRandomState() {
  return detail::GlobalRng().CaptureState();
}

inline void RestoreRandomState(RandomState state) {
  detail::GlobalRng().RestoreState(state);
}

[[nodiscard]] inline int RandomInt() { return detail::GlobalRng().Next(); }

[[nodiscard]] inline float RandomAngle() {
  constexpr float kRandomRange = 2147483648.0F;
  return static_cast<float>(RandomInt()) * (kFullAngle / kRandomRange);
}

} // namespace math
