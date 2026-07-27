///
/// GraphicsAssets - surface slots reserved for multi-surface assets
///
#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

#include "gfx/constants.h"

namespace data::graphics_assets {

inline constexpr uint8_t kFaceSurfaceCount = 3;
inline constexpr uint8_t kEndingPictureCount = 6;

inline SURFACE_ID FaceSurface(uint8_t index) {
  assert(index < kFaceSurfaceCount);
  return static_cast<SURFACE_ID>(std::to_underlying(SURFACE_ID::FACE) + index);
}

inline SURFACE_ID EndingPictureSurface(uint8_t index) {
  assert(index < kEndingPictureCount);
  return static_cast<SURFACE_ID>(std::to_underlying(SURFACE_ID::ENDING_PIC) +
                                 index);
}

} // namespace data::graphics_assets
