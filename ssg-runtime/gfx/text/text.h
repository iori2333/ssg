///
/// Common font rendering interface, independent of a specific rasterizer
///
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include "gfx/core/constants.h"
#include "gfx/core/coords.h"

struct TextRenderRectId {
  size_t index = std::numeric_limits<size_t>::max();
  uint64_t generation = 0;

  constexpr bool operator==(const TextRenderRectId &) const = default;
};

class TextRenderSession;

// Horizontally centers [str] on [s]'s rectangle.
int TextLayoutXCenter(TextRenderSession &session, std::string_view text);
