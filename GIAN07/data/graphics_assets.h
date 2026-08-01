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

inline SurfaceId FaceSurface(uint8_t index) {
  assert(index < kFaceSurfaceCount);
  return static_cast<SurfaceId>(std::to_underlying(SurfaceId::Face) + index);
}

inline SurfaceId EndingPictureSurface(uint8_t index) {
  assert(index < kEndingPictureCount);
  return static_cast<SurfaceId>(std::to_underlying(SurfaceId::EndingPicture) +
                                index);
}

} // namespace data::graphics_assets
