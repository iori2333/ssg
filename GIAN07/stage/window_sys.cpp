/*                                                                           */
/*   WINDOWSYS.cpp   コマンドウィンドウ処理                                  */
/*                                                                           */
/*                                                                           */

#include "window_sys.h"

#include "font_uty.h"
#include "game/enum_flags.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "loader.h"
#include "msg_window/msg_window.h"
#include "platform/text_backend.h"
#include <utility>

///// [非公開関数] /////

static void CWinDrawLabel(TEXTRENDER_SESSION &s, const MenuLabel &label,
                          bool is_title);

///// [MenuDef / MenuItem メソッド] /////

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

///// [MenuController メソッド] /////

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

  OldKey = Key_Data; // TODO: 引数化は Phase 4 で対応
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

// コマンドウィンドウを１フレーム動作させる //
void MenuController::Tick(INPUT_BITS key) {
  switch (State) {
  case CWIN_DEAD: // 使用されていない
    return;

  case CWIN_FREE: // 入力待ち状態
    KeyEvent(key);
    Count = 0;
    return;

  case CWIN_OPEN: // 項目移動中(進む)
    break;

  case CWIN_CLOSE: // 項目移動中(戻る)
    break;

  case CWIN_NEXT: // 次のウィンドウに移行中
    break;

  case CWIN_BEFORE: // 前のウィンドウに移行中
    break;

  case CWIN_INIT: // 初期化処理中
    State = CWIN_FREE;
    break;
  }

  Count++;
}

// コマンドウィンドウの描画 //
void MenuController::Draw() {
  int i = 0;
  WINDOW_COORD top = y;

  // アクティブな項目を検索する //
  auto *p = SearchActive();

  // 半透明ＢＯＸの描画 //
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

  // 文字列の描画 //
  WINDOW_POINT topleft = {x, y};
  const auto trr = TRRs[0];
  const Narrow::string_view str = p->Title->Title;
  TextObj.Render(topleft, trr, str, [=](TEXTRENDER_SESSION &s) {
    CWinDrawLabel(s, *p->Title, true);
  });
  topleft.y += (CWIN_ITEM_H + 1); // ???

  for (i = 0; std::cmp_less(i, p->NumItems); i++) {
    const auto trr = TRRs[1 + i];
    auto *item = p->ItemPtr[i];
    const Narrow::string_view c =
        ((item->Flags == item->FlagsPrev) ? item->Title : "");
    TextObj.Render(topleft, trr, c, [=](TEXTRENDER_SESSION &s) {
      CWinDrawLabel(s, *item, false);
    });
    item->FlagsPrev = item->Flags;
    topleft.y += CWIN_ITEM_H;
  }
}

// アクティブなウィンドウを探す //
MenuDef *MenuController::SearchActive() {
  // 現在アクティブな項目を探す //
  auto *p = &Parent;
  for (auto i = 0; std::cmp_less(i, SelectDepth); i++) {
    p = p->ItemPtr[Select[i]]->Submenu;
  }
  return p;
}

// 長いサブメニュー用にウィンドウ Y 座標を調整する //
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

// キーボード入力を処理する //
void MenuController::KeyEvent(INPUT_BITS key) {
  if (FirstWait) {
    if (key != 0U) {
      return;
    }
    FirstWait = false;
  }

  // 操作性が悪かった点を修正 //
  if (key == 0 && (KeyCount != 0U)) {
    OldKey = 0;
    KeyCount = 0;
    return;
  }

  // アクティブなウィンドウを検索する //
  const auto *p = SearchActive();
  auto Depth = SelectDepth;

  // In case the item just disabled itself...
  while (!!(p->ItemPtr[Select[Depth]]->Flags & MenuFlags::DISABLED)) {
    Select[Depth] = ((Select[Depth] + 1) % p->NumItems);
  }

  // アクティブな項目をセットする //
  auto *p2 = p->ItemPtr[Select[Depth]];

  // キーボードの過剰なリピート防止 //
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
  if ((OldKey == KEY_UP) || (OldKey == KEY_DOWN) ||
      (OldKey == KEY_LEFT) || (OldKey == KEY_RIGHT)) {
    KeyCount = CWIN_KEYWAIT;
    return;
  }
  if (Input_IsOK(OldKey) || Input_IsCancel(OldKey)) {
    // いかなる場合もリピートを許可しないキー //
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

  // 一部のキーボード入力を処理する(KEY_UP/KEY_DOWN) //
  if ((key == KEY_UP) || (key == KEY_DOWN)) {
    // 一つ上の項目へ / 一つ下の項目へ
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
    // コールバック動作時の処理 //
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
          // 後で (CWIN_CLOSE) に変更すること//
          State = CWIN_DEAD;
          OldKey = 0; // ここは結構重要
        }
      } else {
        move_to_previous_level();
      }
    }
  } else {
    // Hotkeys might be pressed while the cursor is on an item with a menu,
    // where we wouldn't fall into the branch above...
    p->SetItems(*this, false);

    // デフォルトのキーボード動作 //
    if (Input_IsOK(key)) {
      // 決定・選択
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
      // キャンセル
      move_to_previous_level();
    }
  }
}

///// [CWin* 転調ファサード] /////

void CWinMove(MenuController *ws) { ws->Tick(Key_Data); }
void CWinDraw(MenuController *ws) { ws->Draw(); }
MenuDef *CWinSearchActive(MenuController *ws) { return ws->SearchActive(); }

// コマンド [Exit] のデフォルト処理関数 //
bool CWinExitFn(MenuController & /*ctrl*/, INPUT_BITS key) {
  return !(Input_IsOK(key) || Input_IsCancel(key));
}

PIXEL_SIZE CWinTextExtent(Narrow::string_view str) {
  return TextObj.TextExtent(FONT_ID::SMALL, str);
}

PIXEL_SIZE CWinItemExtent(Narrow::string_view str) {
  auto ret = CWinTextExtent(str);
  ret.w += CWIN_ITEM_LEFT;
  ret.h = CWIN_ITEM_H;
  return ret;
}

///// [メッセージウィンドウ転調] /////

void MWinInit(const WINDOW_LTRB &rc, MsgWindowFlags flags) {
  MsgWin.Init(rc, flags);
}
void MWinOpen() { MsgWin.Open(); }
void MWinClose() { MsgWin.Close(); }
void MWinForceClose() { MsgWin.ForceClose(); }
void MWinMove() { MsgWin.Tick(); }
void MWinDraw() { MsgWin.Draw(); }
void MWinMsg(Narrow::string_view str) { MsgWin.Msg(str); }
void MWinFace(uint8_t faceID) { MsgWin.Face(faceID); }
void MWinCmd(uint8_t cmd) { MsgWin.Cmd(cmd); }
void MWinHelp(MenuController *ws) { MsgWin.Help(ws); }

///// [非公開関数実装] /////

static void CWinDrawLabel(TEXTRENDER_SESSION &s, const MenuLabel &label,
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
      ((label.Title.ptr != nullptr) && (*label.Title.ptr == ' '));
  const auto left =
      (!!(label.Flags & MenuFlags::CENTER)
           ? TextLayoutXCenter(s, label.Title)
           : ((is_title && starts_with_space) ? 0 : CWIN_ITEM_LEFT));
  s.Put({.x = (left + 1), .y = 0}, label.Title, col.shadow);
  s.Put({.x = (left + 0), .y = 0}, label.Title, col.text);
}
