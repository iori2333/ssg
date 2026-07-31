/// Common text layout helpers.

#include "text.h"

#include "platform/text_backend.h"

PIXEL_COORD TextLayoutXCenter(TEXTRENDER_SESSION &session,
                              std::string_view text) {
  return (session.RectSize().w - session.Extent(text).w) / 2;
}
