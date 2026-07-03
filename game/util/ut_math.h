///
/// Integer math utilities
///
#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>
#include <random>

namespace ut_math_detail {

constexpr double PI = std::numbers::pi;
constexpr double DEG256_TO_RAD = (2.0 * PI) / 256.0;
constexpr double RAD_TO_DEG256 = 256.0 / (2.0 * PI);
constexpr double deg256_to_rad(uint8_t deg) {
  return static_cast<double>(deg) * DEG256_TO_RAD;
}

// Inverse of deg256_to_rad, with the same [0,255] wrap that `atan8` uses.
// Needed so sprite-frame bucketing (e.g. `((deg+8) & 0xf0)`) is preserved once
// a live entity's angle is stored as `double` radians instead of uint8_t.
inline uint8_t rad_to_deg256(double rad) {
  int deg256 = static_cast<int>(std::round(rad * RAD_TO_DEG256));
  deg256 = ((deg256 % 256) + 256) % 256;
  return static_cast<uint8_t>(deg256);
}

// Modern random number generator.
// Produces 15-bit unsigned integers for compatibility with the legacy API.
class Rng {
public:
  explicit Rng(uint32_t seed = 0) noexcept : engine_(seed) {}

  void seed(uint32_t s) { engine_.seed(s); }

  uint16_t next() {
    // Use the high 15 bits of the 32-bit Mersenne Twister output.
    return static_cast<uint16_t>((engine_() >> 16) & 0x7FFF);
  }

  // Uniform integer in [0, max].
  uint16_t next(uint16_t max) {
    if (max == 0) {
      return 0;
    }
    std::uniform_int_distribution<uint16_t> dist(0, max);
    return dist(engine_);
  }

private:
  std::mt19937 engine_;
};

} // namespace ut_math_detail

// Table lookup macros replaced with inline functions.
// These still return values scaled by 256 to keep existing callers working.
inline int sinm(uint8_t deg) {
  return static_cast<int>(
      std::round(std::sin(ut_math_detail::deg256_to_rad(deg)) * 256.0));
}

// Trig functions 2
int sinl(uint8_t deg, int length); // (SIN(deg) * length) / 256)
int cosl(uint8_t deg, int length); // (COS(deg) * length) / 256)

// ((length * 256) / ((COS(deg) > 0) ? COS(deg) : 256))
int cosDiv(uint8_t deg, int length);

uint8_t atan8(int x, int y); // 32-bit version

// ============================================================
// Radian-based trig (float-angle migration target)
// ============================================================
// Direct float replacements for the deg256 functions above, for use once a
// live entity's angle is stored as `double` radians. Project a length onto an
// axis given an angle in radians.
inline int cos_len(double rad, int length) {
  return static_cast<int>(std::round(std::cos(rad) * length));
}
inline int sin_len(double rad, int length) {
  return static_cast<int>(std::round(std::sin(rad) * length));
}

// Full-precision atan2 returning radians (returns 0 for the zero vector,
// matching atan8). Prefer this over deg256_to_rad(atan8(...)) where the snap
// to 256 buckets would lose precision (e.g. continuous homing steering).
inline double atan2_rad(double y, double x) {
  if (x == 0.0 && y == 0.0) {
    return 0.0;
  }
  return std::atan2(y, x);
}

// Wrap an angle (radians) to [-pi, pi]: the shortest-turn normalization that
// replaces the legacy `if (<-128) += 256; if (>128) -= 256` deg256 idiom.
inline double wrap_pi(double a) {
  constexpr double two_pi = 2.0 * std::numbers::pi;
  a = std::fmod(a, two_pi); // a in (-2*pi, 2*pi)
  if (a <= -ut_math_detail::PI) {
    a += two_pi;
  }
  if (a > ut_math_detail::PI) {
    a -= two_pi;
  }
  return a;
}

// Square root (integer version)
// Calculates √[s], rounded to the nearest integer.
int32_t isqrt(int32_t s);

// Random numbers
void rnd_seed_set(uint32_t val);
uint16_t rnd();
