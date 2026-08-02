/// Common text layout helpers.

#include <string_view>

#include "coords.h"
#include "text.h"

// NOLINTNEXTLINE(misc-include-cleaner) - required for TextRenderSession.
#include "platform/text_backend.h"

PixelCoord TextLayoutXCenter(TextRenderSession &session,
                             std::string_view text) {
  return (session.RectSize().w - TextRenderSession::Extent(text).w) / 2;
}
