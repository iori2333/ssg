/// Common text layout helpers.

#include <string_view>

#include "coords.h"
#include "text.h"
#include "text_ttf.h"

PixelCoord TextLayoutXCenter(TextRenderSession &session,
                             std::string_view text) {
  return (session.RectSize().w - session.Extent(text).w) / 2;
}
