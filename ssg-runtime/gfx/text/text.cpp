/// Common text layout helpers.

#include <string_view>

#include "text.h"
#include "text_renderer.h"

#include "gfx/core/coords.h"

int TextLayoutXCenter(TextRenderSession &session, std::string_view text) {
  return (session.RectSize().x - session.Extent(text).x) / 2;
}
