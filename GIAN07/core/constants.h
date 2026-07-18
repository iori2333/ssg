///
/// Constants - GIAN07-layer additions to game/ shared types
///
#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

#include "audio/constants.h"  // SfxId, SND_X_MID, SND_X_PER_DECIBEL
#include "gfx/constants.h"     // SURFACE_ID, FONT_ID, GRP_RES, etc.

// GIAN07-only constants

constexpr auto FACE_MAX = 3; // Maximum simultaneous face loads
constexpr auto ENDING_PIC_MAX = 6;

// Addition is only defined for the types of surfaces we have multiple of.
static constexpr SURFACE_ID operator+(SURFACE_ID lhs, uint8_t rhs) {
  assert(!((lhs == SURFACE_ID::FACE) && (rhs >= FACE_MAX)));
  assert(!((lhs == SURFACE_ID::ENDING_PIC) && (rhs >= ENDING_PIC_MAX)));
  return SURFACE_ID{static_cast<uint8_t>(std::to_underlying(lhs) + rhs)};
}
