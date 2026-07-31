/// Shared UTF-8 text marquee helpers.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "gfx/text.h"

namespace ui {

inline constexpr uint32_t kMarqueeStepFrames = 10;

std::string MarqueeWindow(TEXTRENDER_SESSION &session, std::string_view text,
                          int available_width, uint32_t frame);

} // namespace ui
