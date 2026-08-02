/// Common text layout helpers.

#include <string_view>

#include "text.h"

#include "gfx/coords.h"
// NOLINTNEXTLINE(misc-include-cleaner) - required for TextRenderSession.
#include "platform/text_backend.h"

PixelCoord TextLayoutXCenter(TextRenderSession &session,
                             std::string_view text) {
  return (session.RectSize().w - TextRenderSession::Extent(text).w) / 2;
}
