///
/// Common font rendering interface, independent of a specific rasterizer
///
#pragma once

#include <string_view>

#include "constants.h"
#include "coords.h"
#include "graphics.h"

using TextRenderRectId = unsigned int;

class TextRenderSession;

// Horizontally centers [str] on [s]'s rectangle.
PixelCoord TextLayoutXCenter(TextRenderSession &session, std::string_view text);
