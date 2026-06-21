///
/// MenuRenderer - Menu window drawing
///

#include "menu_renderer.h"

#include "game/coords.h"
#include "game/text.h"
#include "game/graphics_backend.h"
#include "platform/windows/text_gdi.h"

// Draw menu label (title / item)
void MenuDrawLabel(TEXTRENDER_SESSION &s, const MenuLabel &label,
                   bool is_title) {
  struct COLOR_PAIR {
    RGB shadow;
    RGB text;
  };

  static constexpr COLOR_PAIR COL[2][2] = {
      COLOR_PAIR{.shadow = {.r = 128, .g = 128, .b = 128},
                 .text = {.r = 255, .g = 255, .b = 255}}, // Active, regular
      COLOR_PAIR{.shadow = {.r = 128, .g = 128, .b = 128},
                 .text = {.r = 255, .g = 255, .b = 70}}, // Active, HL
      COLOR_PAIR{.shadow = {.r = 96, .g = 96, .b = 96},
                 .text = {.r = 192, .g = 192, .b = 192}}, // Disabled, regular
      COLOR_PAIR{.shadow = {.r = 96, .g = 96, .b = 96},
                 .text = {.r = 192, .g = 192, .b = 70}}, // Disabled, HL
  };

  const auto disabled = !!(label.Flags & MenuFlags::DISABLED);
  const auto highlight = !!(label.Flags & MenuFlags::HIGHLIGHT);
  const auto &col = COL[disabled][highlight];
  s.SetFont(CWIN_FONT);

  // Adding CWIN_ITEM_LEFT to centered text would throw it off-center,
  // obviously. Also, non-centered titles that don't start with spaces
  // shouldn't be dedented relative to the menu items.
  const auto starts_with_space =
      ((label.Title != nullptr) && (*label.Title == ' '));
  const auto left =
      (!!(label.Flags & MenuFlags::CENTER)
           ? TextLayoutXCenter(s, label.Title)
           : ((is_title && starts_with_space) ? 0 : CWIN_ITEM_LEFT));
  s.Put({.x = (left + 1), .y = 0}, label.Title, col.shadow);
  s.Put({.x = (left + 0), .y = 0}, label.Title, col.text);
}

// Draw window frame
void DrawWindowFrame(int x, int y, int w, int h) {
  PIXEL_LTRB src;

  w = w >> 1;
  h = h >> 1;

  // Top-left
  src = {0, 0, w, h};
  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);

  // Top-right
  src = {(384 - w), 0, 384, h};
  GrpSurface_Blit({(x + w), y}, SURFACE_ID::SYSTEM, src);

  // Bottom-left
  src = {0, (80 - h), w, 80};
  GrpSurface_Blit({x, (y + h)}, SURFACE_ID::SYSTEM, src);

  // Bottom-right
  src = {(384 - w), (80 - h), 384, 80};
  GrpSurface_Blit({(x + w), (y + h)}, SURFACE_ID::SYSTEM, src);
}
