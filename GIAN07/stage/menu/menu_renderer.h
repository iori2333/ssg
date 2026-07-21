///
/// MenuRenderer - Menu window rendering
///

#pragma once

#include "stage/menu/window_sys.h"

struct TEXTRENDER_SESSION;

// Draw menu label (title / item)
void MenuDrawLabel(TEXTRENDER_SESSION &s, const MenuLabel &label,
                   bool is_title);

// Draw window frame
void DrawWindowFrame(int x, int y, int w, int h);
