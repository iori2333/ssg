///
/// Common font rendering interface, independent of a specific rasterizer
///
#pragma once

#include <string_view>

#include "coords.h"
#include "graphics.h"

#include "gfx/constants.h"

using TEXTRENDER_RECT_ID = unsigned int;

class TEXTRENDER_SESSION;

// Horizontally centers [str] on [s]'s rectangle.
PIXEL_COORD TextLayoutXCenter(TEXTRENDER_SESSION &session,
                              std::string_view text);
