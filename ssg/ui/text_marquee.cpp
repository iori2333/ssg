/// Shared UTF-8 text marquee helpers.

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

#include "text_marquee.h"

#include "gfx/text/text_renderer.h"

namespace ui {
namespace {

void RemoveLastUtf8CodePoint(std::string &text) {
  if (text.empty()) {
    return;
  }
  auto start = text.size() - 1;
  while (start > 0 &&
         (static_cast<unsigned char>(text[start]) & 0xC0U) == 0x80U) {
    start--;
  }
  text.resize(start);
}

void RemoveFirstUtf8CodePoint(std::string &text) {
  if (text.empty()) {
    return;
  }
  auto end = std::size_t{1};
  while (end < text.size() &&
         (static_cast<unsigned char>(text[end]) & 0xC0U) == 0x80U) {
    end++;
  }
  text.erase(0, end);
}

} // namespace

std::string MarqueeWindow(TextRenderSession &session, std::string_view text,
                          int available_width, int frame) {
  if (available_width <= 0) {
    return {};
  }

  std::string suffix(text);
  if (session.Extent(suffix).x <= available_width) {
    return suffix;
  }

  std::string final_suffix = suffix;
  int shift_count = 0;
  while (!final_suffix.empty() &&
         session.Extent(final_suffix).x > available_width) {
    RemoveFirstUtf8CodePoint(final_suffix);
    shift_count++;
  }

  constexpr int kStartDelayFrames = 45;
  constexpr int kEndDelayFrames = 45;
  const auto scroll_frames = shift_count * kMarqueeStepFrames;
  const auto cycle_frames = kStartDelayFrames + scroll_frames + kEndDelayFrames;
  const auto phase = frame % cycle_frames;

  int shifts = 0;
  if (phase >= kStartDelayFrames) {
    shifts = std::min(shift_count,
                      1 + (phase - kStartDelayFrames) / kMarqueeStepFrames);
  }
  while (shifts-- != 0) {
    RemoveFirstUtf8CodePoint(suffix);
  }

  while (!suffix.empty() && session.Extent(suffix).x > available_width) {
    RemoveLastUtf8CodePoint(suffix);
  }
  return suffix;
}

} // namespace ui
