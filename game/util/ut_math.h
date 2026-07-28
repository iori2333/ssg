///
/// Integer math utilities
///
#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>
#include <random>

struct RandomState {
  uint32_t seed;
  uint64_t draw_count;
};

namespace ut_math_detail {

constexpr double PI = std::numbers::pi;
constexpr double DEG256_TO_RAD = (2.0 * PI) / 256.0;
constexpr double RAD_TO_DEG256 = 256.0 / (2.0 * PI);
constexpr double deg256_to_rad(uint8_t deg) {
  return static_cast<double>(deg) * DEG256_TO_RAD;
}

// Modern random number generator.
// Produces 15-bit unsigned integers for compatibility with the legacy API.
class Rng {
public:
  explicit Rng(uint32_t seed = 0) noexcept : engine_(seed), seed_(seed) {}

  void seed(uint32_t s) {
    engine_.seed(s);
    seed_ = s;
    draw_count_ = 0;
  }

  [[nodiscard]] RandomState state() const { return {seed_, draw_count_}; }

  void restore(RandomState state) {
    engine_.seed(state.seed);
    engine_.discard(state.draw_count);
    seed_ = state.seed;
    draw_count_ = state.draw_count;
  }

  uint16_t next() {
    // Use the high 15 bits of the 32-bit Mersenne Twister output.
    draw_count_++;
    return static_cast<uint16_t>((engine_() >> 16) & 0x7FFF);
  }

  // Uniform integer in [0, max].
  uint16_t next(uint16_t max) {
    if (max == 0) {
      return 0;
    }
    return static_cast<uint16_t>(next() % (static_cast<uint32_t>(max) + 1));
  }

private:
  std::mt19937 engine_;
  uint32_t seed_ = 0;
  uint64_t draw_count_ = 0;
};

} // namespace ut_math_detail

// Table lookup macros replaced with inline functions.
// These still return values scaled by 256 to keep existing callers working.
inline int sinm(uint8_t deg) {
  return static_cast<int>(
      std::round(std::sin(ut_math_detail::deg256_to_rad(deg)) * 256.0));
}

inline int cosm(uint8_t deg) {
  return static_cast<int>(
      std::round(std::cos(ut_math_detail::deg256_to_rad(deg)) * 256.0));
}

// Trig functions 2
int sinl(uint8_t deg, int length); // (SIN(deg) * length) / 256)
int cosl(uint8_t deg, int length); // (COS(deg) * length) / 256)

// ((length * 256) / ((SIN(deg) > 0) ? SIN(deg) : 256))
int sinDiv(uint8_t deg, int length);

// ((length * 256) / ((COS(deg) > 0) ? COS(deg) : 256))
int cosDiv(uint8_t deg, int length);

uint8_t atan8(int x, int y); // 32-bit version

// Square root (integer version)
// Calculates √[s], rounded to the nearest integer.
int32_t isqrt(int32_t s);

// Random numbers
void rnd_seed_set(uint32_t val);
[[nodiscard]] RandomState rnd_state();
void rnd_state_restore(RandomState state);
uint16_t rnd();
