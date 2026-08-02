///
/// Public effect commands shared with gameplay systems.
///

#pragma once

#include <cstdint>

enum class CircleEffectKind : uint8_t {
  None = 0x00,
  Star = 0x01,
  Converging = 0x02,
  Diverging = 0x03,
};

enum class FragmentKind : uint8_t {
  Graze,
  Smoke,
  ExpandingCircle,
  SmallStar,
  LargeStar,
  Hit,
  RisingStar,
  Heart,
};

enum class ScreenTransition : uint8_t {
  CircleFadeIn,
  CircleFadeOut,
  WhiteIn,
  WhiteOut,
};
