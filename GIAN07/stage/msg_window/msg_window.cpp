/*                                                                           */
/*   msg_window.cpp   メッセージウィンドウ処理                                */
/*                                                                           */
/*                                                                           */

#include "msg_window.h"

#include "game/ut_math.h"
#include "loader.h"
#include "menu/menu_renderer.h"
#include "platform/text_backend.h"
#include "window_sys.h" // face_data, FACE_*, SURFACE_ID, MenuController, etc.

#include <utility>

///// [グローバル変数] /////

MsgWindow MsgWin; // メッセージウィンドウ (後方互換用、UIManager が本体)

void MsgWindow::MsgBlank() {
  line = 0;
  for (auto &m : msg) {
    m = {};
  }
  text.clear();
}

void MsgWindow::Init(const WINDOW_LTRB &rc, MsgWindowFlags flags) {
  max_size = rc;
  this->flags = flags;
  text_topleft = {
      .x = (!!(flags & MsgWindowFlags::WITH_FACE) ? FACE_W : 8),
      .y = 8,
  };
  trr = TextObj.Register(rc.Size() - text_topleft);
}

void MsgWindow::Open() {
  if (state != MWIN_DEAD) {
    return;
  }

  // 状態および、最終値のセット //
  state = MWIN_OPEN;
  face_state = MFACE_NONE; // 何も表示しない
  face_id = 0;
  face_time = 0;

  // 矩形の初期値をセットする //
  const auto y_mid = ((max_size.bottom + max_size.top) / 2);
  now_size.left = max_size.left;
  now_size.right = max_size.right;
  now_size.top = y_mid - 4;
  now_size.bottom = y_mid + 4;

  Cmd(MWCMD_NORMALFONT); // ノーマルフォント

  // ウィンドウ内に表示するものの初期化 //
  MsgBlank();
}

// メッセージウィンドウをクローズする //
void MsgWindow::Close() {
  //	if(state != MWIN_FREE) return;

  face_state = MFACE_CLOSE;
  state = MWIN_CLOSE;
}

// メッセージウィンドウを強制クローズする //
void MsgWindow::ForceClose() {
  face_state = MFACE_NONE;
  state = MWIN_DEAD;
}

// メッセージウィンドウを動作させる //
void MsgWindow::Tick() {
  switch (face_state) {
  case MFACE_OPEN: // 顔を表示しようとしている
    face_time += 16;
    if (face_time == 0) {
      face_state = MFACE_WAIT;
    }
    break;

  case MFACE_NEXT:
    face_time += 16;
    if (face_time == 0) {
      face_state = MFACE_OPEN;
      face_id = next_face;
      GrpBackend_PaletteSet(face_data[face_id / FACE_NUMX].pal);
    }
    break;

  case MFACE_CLOSE: // 顔を消そうとしている
    face_time += 16;
    if (face_time == 0) {
      face_state = MFACE_NONE;
    }
    break;

  default:
    break;
  }

  switch (state) {
  case MWIN_OPEN:
    now_size.top -= 2;
    now_size.bottom += 2;

    // 完全にオープンできた場合 //
    if (now_size.top <= max_size.top) {
      now_size = max_size;
      state = MWIN_FREE;
    }
    break;

  case MWIN_CLOSE:
    now_size.top += 3;
    now_size.bottom -= 3;
    now_size.right += 6;
    now_size.left -= 6;

    // 完全にクローズできた場合 //
    if (now_size.top >= now_size.bottom) {
      state = MWIN_DEAD;
    }
    break;

  case MWIN_DEAD:
  case MWIN_FREE:
    break;
  }
}

// メッセージウィンドウを描画する(上に同じ) //
void MsgWindow::Draw() {
  PIXEL_LTRB src;

  const auto x = now_size.left;         // ウィンドウ左上Ｘ
  const auto y = now_size.top;          // ウィンドウ左上Ｙ
  const auto w = (now_size.right - x);  // ウィンドウ幅
  const auto h = (now_size.bottom - y); // ウィンドウ高さ
  int len = 0;
  int time = 0;
  int oy = 0;

  // メッセージウィンドウが死んでいたら何もしない //
  if (state == MWIN_DEAD) {
    return;
  }

  // 半透明部の描画 //
  GrpGeom->Lock();
  GrpGeom->SetAlphaNorm((GrpGeom_FB() != nullptr) ? (64 + 32) : 110);
  GrpGeom->SetColor({0, 0, 3});
  GrpGeom->DrawBoxA((x + 4), (y + 4), (x + w - 4), (y + h - 4));
  GrpGeom->Unlock();

  // 文字列を表示するのはウィンドウが[FREE]である場合だけ        //
  // -> こうしないと文字列用 Surface を作成することになるので... //
  if ((state == MWIN_FREE) && trr) {
    const auto topleft = (WINDOW_POINT{x, y} + text_topleft);
    const auto trr = this->trr.value();
    const auto &text = this->text;
    TextObj.Render(topleft, trr, text, [this](TEXTRENDER_SESSION &s) {
      // セットされたフォントで描画
      s.SetFont(font_id);
      for (auto i = 0; std::cmp_less(i, line); i++) {
        const auto m = msg[i];

        // 一応安全対策
        if (m.empty()) {
          continue;
        }
        const PIXEL_COORD top = (i * font_dy);
        const auto left =
            (!!(flags & MsgWindowFlags::CENTER) ? TextLayoutXCenter(s, m) : 0);

        // 灰色で１どっとずらして描画
        s.Put({.x = (left + 1), .y = top}, m,
              RGB{.r = 128, .g = 128, .b = 128});
        // 白で表示すべき位置に表示
        s.Put({.x = (left + 0), .y = top}, m,
              RGB{.r = 255, .g = 255, .b = 255});
      }
    });
  }

  DrawWindowFrame(x, y, w, h);

  // お顔をかきましょう(表示を要請されている場合にだけ) //
  const auto sid = (SURFACE_ID::FACE + (face_id / FACE_NUMX));
  switch (face_state) {
  case MFACE_WAIT:
    oy = max_size.bottom - 100;
    src = PIXEL_LTWH{((face_id % FACE_NUMX) * FACE_W), 0, FACE_W, FACE_H};
    GrpSurface_Blit({(x + 2), oy}, sid, src);
    break;

  case MFACE_OPEN:
    time = face_time >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 153), (64 - time) / 2);
      src = PIXEL_LTWH{((face_id % FACE_NUMX) * FACE_W), i, FACE_W, 1};
      GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case MFACE_NEXT:
    time = (255 - face_time) >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 153), (64 - time) / 2);
      src = PIXEL_LTWH{((face_id % FACE_NUMX) * FACE_W), i, FACE_W, 1};
      GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case MFACE_CLOSE:
    time = face_time >> 1;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 4), time);
      src = PIXEL_LTWH{((face_id % FACE_NUMX) * FACE_W), i, FACE_W, 1};
      if ((i & 1) != 0) {
        GrpSurface_Blit({(x - len + 2), (oy + i)}, sid, src);
      } else {
        GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
      }
    }
    break;
  }
}

void MsgWindow::Msg(std::string_view s) {
  int Line = 0;
  int i = 0;

  Line = line;

  if (std::cmp_equal(Line, max_line)) {
    // すでに表示最大行数を超えていた場合 //
    for (i = 1; i < max_line - 1; i++) {
      msg[i] = msg[i + 1];
    }
    msg[Line - 1] = s;
  } else {
    // ポインタセット＆行数更新 //
    msg[Line] = s;
    line = Line + 1;
  }

  text.clear();
  for (decltype(line) i = 0; i < line; i++) {
    text += msg[i];
    text += '\n';
  }
}

// 顔をセットする //
void MsgWindow::Face(uint8_t faceID) {
  if (state == MWIN_DEAD) {
    return; // 表示不可
  }
  if (faceID / FACE_NUMX >= FACE_MAX) {
    return; // あり得ない数字
  }

  assert(text_topleft.x == FACE_W);

  if (face_state == MFACE_NONE) {
    face_state = MFACE_OPEN;
    face_id = faceID;
    GrpBackend_PaletteSet(face_data[faceID / FACE_NUMX].pal);
  } else {
    face_state = MFACE_NEXT;
    next_face = faceID;
  }

  face_time = 0;
}

// コマンドを送る //
void MsgWindow::Cmd(uint8_t cmd) {
  int temp = 0;
  int Ysize = 0;

  switch (cmd) {
  case MWCMD_LARGEFONT: // ラージフォントを使用する
    Ysize += 8;
    [[fallthrough]];
  case MWCMD_NORMALFONT: // ノーマルフォントを使用する
    Ysize += 2;
    [[fallthrough]];
  case MWCMD_SMALLFONT: // スモールフォントを使用する
    Ysize += 14;
    temp = max_size.bottom - max_size.top - 16;
    max_line = temp / Ysize;                                 // 表示可能最大行数
    font_dy = ((temp % Ysize) / (temp / Ysize)) + Ysize + 1; // Ｙ増量
    font_id = Cast::down_enum<FONT_ID>(cmd);                 // 使用フォント
    [[fallthrough]];

  case MWCMD_NEWPAGE: // 改ページする
    // 文字列無効化, 最初の行へ
    MsgBlank();
    break;

  default: // ここに来たらバグね...
    break;
  }
}

// ヘルプ文字列を送る //
void MsgWindow::Help(MenuController *ws) {
  // アクティブなウィンドウを検索し、メッセージ領域をクリアする //
  const auto *p = ws->SearchActive();
  MsgBlank();

  // 一列だけ文字列を割り当てる //
  Msg(p->ItemPtr[ws->CurrentSelection()]->Help);
}
