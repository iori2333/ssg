///
/// Integer-only math functions (modernized floating-point implementation)
///

#include <cmath>
#include <type_traits>

#include "ut_math.h"

static ut_math_detail::Rng g_rng; // Global random number instance

int sinl(uint8_t deg, int length) {
  double rad = ut_math_detail::deg256_to_rad(deg);
  return static_cast<int>(std::round(std::sin(rad) * length));
}

int cosl(uint8_t deg, int length) {
  double rad = ut_math_detail::deg256_to_rad(deg);
  return static_cast<int>(std::round(std::cos(rad) * length));
}

// length*256/(COS(deg)>0 ? COS(deg) : 256) //
int cosDiv(uint8_t deg, int length) {
  double c = std::cos(ut_math_detail::deg256_to_rad(deg));
  if (c <= 1e-12) {
    return length;
  }
  return static_cast<int>(std::round(static_cast<double>(length) / c));
}

uint8_t atan8(int x, int y) {
  if (x == 0 && y == 0) {
    return 0;
  }

  double rad = std::atan2(static_cast<double>(y), static_cast<double>(x));
  int deg256 =
      static_cast<int>(std::round(rad * ut_math_detail::RAD_TO_DEG256));

  // Normalize to [0, 255].
  deg256 = ((deg256 % 256) + 256) % 256;
  return static_cast<uint8_t>(deg256);
}

void rnd_seed_set(uint32_t val) { g_rng.seed(val); }

int32_t isqrt(int32_t s) {
  // Near-constant-time integer square root algorithm, adapted from
  //
  // 	https://en.wikipedia.org/w/index.php?title=Methods_of_computing_square_roots&oldid=1170166684#Binary_numeral_system_(base_2)
  //
  // The linear ASM algorithm used by the original game takes ~15 hours on an
  // Intel Core i5 8400T to run over the entire domain from 0 to (2³¹ - 1).
  // In contrast, this one takes just 50 seconds to cover the same domain and
  // return the same results. Interestingly, it's not *that* much slower than
  // the simple floating-point version
  //
  // 	 round(sqrt(s))
  //
  // which compiles down to the SSE `SQRTSD` instruction, and takes ~48
  // seconds for the entire domain on the same hardware.
  // (We need to use the `double` variant to ensure that we can fit every
  // signed 32-bit integer.)
  if (s <= 0) {
    return 0;
  }

  auto error = s;
  decltype(s) root = 0;

  // Start at the highest power of 4 ≤[s]
  std::make_unsigned_t<decltype(s)> d = (1 << ((sizeof(s) * 8) - 2));
  while (d > s) {
    d >>= 2;
  }

  while (d != 0) {
    if (error >= (root + d)) {
      error -= (root + d);
      root = ((root >> 1) + d);
    } else {
      root >>= 1;
    }
    d >>= 2;
  }

  // [error] is now equal to ([s] - [root]²), and can help us to determine
  // whether we need to round up to the next integer root.
  // The difference between two consecutive integer squares (𝓃 and 𝓃+1) is
  //
  // 	((𝓃+1)² - 𝓃²) = (2𝓃 + 1)
  //
  // Since we only need half of the difference to arrive at the arithmetic
  // mean, we get (𝓃 + 0.5). And since we use integers, we can round this up
  // to a ≥(𝓃+1) comparison, which can be further simplified to >𝓃.
  if (error > root) {
    return (root + 1);
  }
  return root;
}

uint16_t rnd() { return g_rng.next(); }
