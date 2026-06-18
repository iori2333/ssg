///
/// WindowSys - Command window processing
///

#include "window_sys.h"

#include "game/enum_flags.h"
#include "game/input.h"
#include "game/snd.h"
#include "loader.h"
#include "menu/menu_renderer.h"
#include "platform/text_backend.h"
#include <utility>

// [MenuDef / MenuItem methods]

uint8_t MenuDef::MaxItems() const {
  uint8_t ret = NumItems;
  for (auto i = 0; std::cmp_less(i, NumItems); i++) {
    if (ItemPtr[i]->Submenu != nullptr) {
      ret = (std::max)(ret, ItemPtr[i]->Submenu->MaxItems());
    }
  }
  return ret;
}

void MenuItem::SetActive(bool active) {
  EnumFlagSet(Flags, MenuFlags::DISABLED,
              static_cast<std::underlying_type_t<enum MenuFlags>>(!active));
}

// [MenuController methods]

void MenuController::Init(PIXEL_COORD w) {
  W = w;
  Parent.SetItems(*this, true);

  // Don't forget the header.
  const auto max_items = (1 + Parent.MaxItems());

  for (auto i = 0; i < max_items; i++) {
    TRRs[i] = TextObj.Register({.w = W, .h = CWIN_ITEM_H});
  }
}

void MenuController::Open(WINDOW_POINT topleft, int select) {
  x = topleft.x;
  y = topleft.y;

  Count = 0;
  Select[0] = select;
  SelectDepth = 0;
  State = CWIN_INIT;

  OldKey = Key_Data;
  KeyCount = CWIN_KEYWAIT;

  FirstWait = true;
}

void MenuController::OpenCentered(PIXEL_COORD w, int select) {
  // Shifting it down by 9 pixels avoids the clash with the background image
  // gradient.
  const WINDOW_POINT topleft = {
      (320 - (w / 2)),
      (73 + (CWIN_MAX_H / 2) - (((Parent.NumItems + 1) * CWIN_ITEM_H) / 2))};
  Open(topleft, select);
}

// Process command window for one frame
void MenuController::Tick(INPUT_BITS key) {
  switch (State) {
  case CWIN_DEAD: // Not in use
    return;

  case CWIN_FREE: // Waiting for input
    KeyEvent(key);
    Count = 0;
    return;

  case CWIN_OPEN: // Item moving forward
    break;

  case CWIN_CLOSE: // Item moving backward
    break;

  case CWIN_NEXT: // Switching to next window
    break;

  case CWIN_BEFORE: // Switching to previous window
    break;

  case CWIN_INIT: // Initializing
    State = CWIN_FREE;
    break;
  }

  Count++;
}

// Draw command window
void MenuController::Draw() {
  int i = 0;
  WINDOW_COORD top = y;

  // Find active item
  auto *p = SearchActive();

  // Draw semi-transparent box
  const uint8_t alpha = ((GrpGeom_FB() != nullptr) ? (64 + 32) : 128);

  GrpGeom->Lock();
  GrpGeom->SetAlphaNorm(alpha);

  GrpGeom->SetColor({0, 0, 0});
  GrpGeom->DrawBoxA(x, top, (x + W), (top + CWIN_ITEM_H));
  top += CWIN_ITEM_H;

  GrpGeom->SetColor({0, 0, 2});
  for (i = 0; std::cmp_less(i, p->NumItems); i++) {
    if (std::cmp_equal(i, Select[SelectDepth])) {
      GrpGeom->SetAlphaNorm(128);
      GrpGeom->SetColor({5, 0, 0});
    }
    GrpGeom->DrawBoxA(x, top, (x + W), (top + CWIN_ITEM_H));
    top += CWIN_ITEM_H;
    if (std::cmp_equal(i, Select[SelectDepth])) {
      GrpGeom->SetAlphaNorm(alpha);
      GrpGeom->SetColor({0, 0, 2});
    }
  }
  GrpGeom->Unlock();

  // Draw text
  WINDOW_POINT topleft = {x, y};
  const auto trr = TRRs[0];
  std::string_view str = p->Title->Title;
  TextObj.Render(topleft, trr, str, [=](TEXTRENDER_SESSION &s) {
    MenuDrawLabel(s, *p->Title, true);
  });
  topleft.y += (CWIN_ITEM_H + 1); // ???

  for (i = 0; std::cmp_less(i, p->NumItems); i++) {
    const auto trr = TRRs[1 + i];
    auto *item = p->ItemPtr[i];
    std::string_view c = ((item->Flags == item->FlagsPrev) ? item->Title : "");
    TextObj.Render(topleft, trr, c, [=](TEXTRENDER_SESSION &s) {
      MenuDrawLabel(s, *item, false);
    });
    item->FlagsPrev = item->Flags;
    topleft.y += CWIN_ITEM_H;
  }
}

// Find active window
MenuDef *MenuController::SearchActive() {
  // Find currently active item
  auto *p = &Parent;
  for (auto i = 0; std::cmp_less(i, SelectDepth); i++) {
    p = p->ItemPtr[Select[i]]->Submenu;
  }
  return p;
}

// Adjust window Y for tall submenus
void MenuController::AdjustYForTallMenu(int baseline_y, int max_visible) {
  const auto *active = SearchActive();
  if (active == nullptr) {
    return;
  }
  y = baseline_y;
  if (active->NumItems > max_visible) {
    y -= ((active->NumItems - max_visible) * CWIN_ITEM_H);
  }
}

// Process keyboard input
void MenuController::KeyEvent(INPUT_BITS key) {
  if (FirstWait) {
    if (key != 0U) {
      return;
    }
    FirstWait = false;
  }

  // Fix for poor responsiveness
  if (key == 0 && (KeyCount != 0U)) {
    OldKey = 0;
    KeyCount = 0;
    return;
  }

  // Find active window
  const auto *p = SearchActive();
  auto Depth = SelectDepth;

  // In case the item just disabled itself...
  while (!!(p->ItemPtr[Select[Depth]]->Flags & MenuFlags::DISABLED)) {
    Select[Depth] = ((Select[Depth] + 1) % p->NumItems);
  }

  // Set active item
  auto *p2 = p->ItemPtr[Select[Depth]];

  // Prevent excessive keyboard repeat
  if (KeyCount != 0U) {
    KeyCount--;
    if (KeyCount == 0) {
      OldKey = 0;
    }
    return;
  }
  if (!!(p2->Flags & MenuFlags::FAST_REPEAT) &&
      (Input_OptionKeyDelta(OldKey) != 0)) {
    KeyCount = FastRepeatWait;
    FastRepeatWait = (std::max)((FastRepeatWait - 2), 0);
    if (KeyCount == 0) {
      OldKey = 0;
    }
    return;
  }
  if ((OldKey == KEY_UP) || (OldKey == KEY_DOWN) || (OldKey == KEY_LEFT) ||
      (OldKey == KEY_RIGHT)) {
    KeyCount = CWIN_KEYWAIT;
    return;
  }
  if (Input_IsOK(OldKey) || Input_IsCancel(OldKey)) {
    // Never allow repeat for these keys
    if (key == OldKey) {
      return;
    }
  } else {
    KeyCount = 0;
  }

  OldKey = key;

  const auto next_active = [](const MenuDef &menu, auto cur,
                              int_fast8_t direction) {
    do {
      cur = ((cur + menu.NumItems + direction) % menu.NumItems);
    } while (!!(menu.ItemPtr[cur]->Flags & MenuFlags::DISABLED));
    return cur;
  };

  const auto move_to_previous_level = [this] {
    if (SelectDepth == 0) {
      return;
    }
    SelectDepth--;
    SearchActive()->SetItems(*this, false);
  };

  // Handle keyboard input (KEY_UP/KEY_DOWN)
  if ((key == KEY_UP) || (key == KEY_DOWN)) {
    // One item up / One item down
    const auto delta = ((key == KEY_UP) ? -1 : +1);
    Select[Depth] = next_active(*p, Select[Depth], delta);
    Snd_SEPlay(SOUND_ID_SELECT);
  } else if (Input_IsCancel(key)) {
    Snd_SEPlay(SOUND_ID_CANCEL);
  } else if (Input_OptionKeyDelta(key) != 0) {
    Snd_SEPlay(SOUND_ID_SELECT);
  } else if (key == 0) {
    FastRepeatWait = CWIN_KEYWAIT;
  }

  if (p2->OptionFn || p2->CallBackFn) {
    // Callback processing
    const auto ret = ([this, p, p2, key] {
      if (p2->OptionFn) {
        if (Input_IsCancel(key)) {
          return false;
        }
        if (const auto delta = Input_OptionKeyDelta(key)) {
          p2->OptionFn(*this, delta);
        }
        p->SetItems(*this, true);
        return true;
      }

      // The item text may need to change while the cursor is on a
      // non-option item...
      p->SetItems(*this, false);

      return p2->CallBackFn(*this, key);
    })();
    if (!ret) {
      if (Depth == 0) {
        if (!Input_IsCancel(key)) {
          // Change to (CWIN_CLOSE) later
          State = CWIN_DEAD;
          OldKey = 0; // This is quite important
        }
      } else {
        move_to_previous_level();
      }
    }
  } else {
    // Hotkeys might be pressed while the cursor is on an item with a menu,
    // where we wouldn't fall into the branch above...
    p->SetItems(*this, false);

    // Default keyboard operation
    if (Input_IsOK(key)) {
      // Confirm / Select
      if ((p2->Submenu != nullptr) && (p2->Submenu->NumItems != 0)) {
        p2->Submenu->Title = p2;
        p2->Submenu->SetItems(*this, false);

        // Jump to the first active item to avoid potentially drawing
        // the selection cursor on top of a disabled item on the next
        // frame.
        Select[Depth + 1] = next_active(*p2->Submenu, -1, +1);
        SelectDepth++;
      }
    } else if (Input_IsCancel(key)) {
      // Cancel
      move_to_previous_level();
    }
  }
}

// [Utility functions]

// Default handler for [Exit] command
bool CWinExitFn(MenuController & /*ctrl*/, INPUT_BITS key) {
  return !(Input_IsOK(key) || Input_IsCancel(key));
}

PIXEL_SIZE CWinTextExtent(std::string_view str) {
  return TextObj.TextExtent(FONT_ID::SMALL, str);
}

PIXEL_SIZE CWinItemExtent(std::string_view str) {
  auto ret = CWinTextExtent(str);
  ret.w += CWIN_ITEM_LEFT;
  ret.h = CWIN_ITEM_H;
  return ret;
}
