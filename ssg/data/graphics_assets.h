///
/// GraphicsAssets - surface slots reserved for multi-surface assets
///
#pragma once

#include <cstddef>
#include <utility>

#include "gfx/core/constants.h"

namespace data::graphics_assets {

inline constexpr std::size_t kFaceSurfaceCount = 3;
inline constexpr std::size_t kEndingPictureCount = 6;

inline SurfaceId FaceSurface(std::size_t index) {
  return static_cast<SurfaceId>(std::to_underlying(SurfaceId::Face) + index);
}

inline SurfaceId EndingPictureSurface(std::size_t index) {
  return static_cast<SurfaceId>(std::to_underlying(SurfaceId::EndingPicture) +
                                index);
}

} // namespace data::graphics_assets
