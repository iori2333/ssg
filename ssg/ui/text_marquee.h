/// Shared UTF-8 text marquee helpers.

#pragma once

#include <string>
#include <string_view>

#include "gfx/text/text.h"

namespace ui {

inline constexpr int kMarqueeStepFrames = 10;

std::string MarqueeWindow(TextRenderSession &session, std::string_view text,
                          int available_width, int frame);

} // namespace ui
