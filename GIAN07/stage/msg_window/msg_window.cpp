///
/// MsgWindow - Message window processing
///

#include <utility>

#include "msg_window.h"

#include "data/gfx_manager.h"
#include "data/sfx_manager.h"
#include "platform/text_backend.h"
#include "stage/menu/menu_renderer.h"
#include "stage/window_sys.h"
#include "util/ut_math.h"

// [Global variables]

MsgWindow MsgWin; // Message window (backward compat, UIManager is the main)

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

  // Set state and final values
  state = MWIN_OPEN;
  face_state = MFACE_NONE; // Display nothing
  face_id = 0;
  face_time = 0;

  // Set initial rectangle values
  const auto y_mid = ((max_size.bottom + max_size.top) / 2);
  now_size.left = max_size.left;
  now_size.right = max_size.right;
  now_size.top = y_mid - 4;
  now_size.bottom = y_mid + 4;

  Cmd(MWCMD_NORMALFONT); // Normal font

  // Initialize display contents in the window
  MsgBlank();
}

// Close the message window
void MsgWindow::Close() {
  //	if(state != MWIN_FREE) return;

  face_state = MFACE_CLOSE;
  state = MWIN_CLOSE;
}

// Force close the message window
void MsgWindow::ForceClose() {
  face_state = MFACE_NONE;
  state = MWIN_DEAD;
}

// Tick the message window
void MsgWindow::Tick() {
  switch (face_state) {
  case MFACE_OPEN: // Attempting to display face
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
    }
    break;

  case MFACE_CLOSE: // Attempting to hide face
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

    // When fully opened
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

    // When fully closed
    if (now_size.top >= now_size.bottom) {
      state = MWIN_DEAD;
    }
    break;

  case MWIN_DEAD:
  case MWIN_FREE:
    break;
  }
}

// Draw the message window (same as above)
void MsgWindow::Draw() {
  PIXEL_LTRB src;

  const auto x = now_size.left;         // Window top-left X
  const auto y = now_size.top;          // Window top-left Y
  const auto w = (now_size.right - x);  // Window width
  const auto h = (now_size.bottom - y); // Window height
  int len = 0;
  int time = 0;
  int oy = 0;

  // Do nothing if the message window is dead
  if (state == MWIN_DEAD) {
    return;
  }

  // Draw translucent part
  GrpGeom->Lock();
  GrpGeom->SetAlphaNorm((GrpGeom_FB() != nullptr) ? (64 + 32) : 110);
  GrpGeom->SetColor({0, 0, 3});
  GrpGeom->DrawBoxA((x + 4), (y + 4), (x + w - 4), (y + h - 4));
  GrpGeom->Unlock();

  // Display text only when window is [FREE]
  // -> Otherwise a Surface for text would have to be created...
  if ((state == MWIN_FREE) && trr) {
    const auto topleft = (WINDOW_POINT{x, y} + text_topleft);
    const auto trr = this->trr.value();
    const auto &text = this->text;
    TextObj.Render(topleft, trr, text, [this](TEXTRENDER_SESSION &s) {
      // Draw with the set font
      s.SetFont(font_id);
      for (auto i = 0; std::cmp_less(i, line); i++) {
        const auto m = msg[i];

        // Safety measure
        if (m.empty()) {
          continue;
        }
        const PIXEL_COORD top = (i * font_dy);
        const auto left =
            (!!(flags & MsgWindowFlags::CENTER) ? TextLayoutXCenter(s, m) : 0);

        // Draw offset by 1 pixel in gray
        s.Put({.x = (left + 1), .y = top}, m,
              RGB{.r = 128, .g = 128, .b = 128});
        // Draw in white at the correct position
        s.Put({.x = (left + 0), .y = top}, m,
              RGB{.r = 255, .g = 255, .b = 255});
      }
    });
  }

  DrawWindowFrame(x, y, w, h);

  // Draw face (only when display is requested)
  const auto sid = (SURFACE_ID::FACE + (face_id / kFaceNumX));
  switch (face_state) {
  case MFACE_WAIT:
    oy = max_size.bottom - 100;
    src = PIXEL_LTWH{((face_id % kFaceNumX) * FACE_W), 0, FACE_W, FACE_H};
    GrpSurface_Blit({(x + 2), oy}, sid, src);
    break;

  case MFACE_OPEN:
    time = face_time >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 153), (64 - time) / 2);
      src = PIXEL_LTWH{((face_id % kFaceNumX) * FACE_W), i, FACE_W, 1};
      GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case MFACE_NEXT:
    time = (255 - face_time) >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 153), (64 - time) / 2);
      src = PIXEL_LTWH{((face_id % kFaceNumX) * FACE_W), i, FACE_W, 1};
      GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case MFACE_CLOSE:
    time = face_time >> 1;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 4), time);
      src = PIXEL_LTWH{((face_id % kFaceNumX) * FACE_W), i, FACE_W, 1};
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
    // When already exceeding max display lines
    for (i = 1; i < max_line - 1; i++) {
      msg[i] = msg[i + 1];
    }
    msg[Line - 1] = s;
  } else {
    // Set pointer and update line count
    msg[Line] = s;
    line = Line + 1;
  }

  text.clear();
  for (decltype(line) i = 0; i < line; i++) {
    text += msg[i];
    text += '\n';
  }
}

// Set the face
void MsgWindow::Face(uint8_t faceID) {
  if (state == MWIN_DEAD) {
    return; // Cannot display
  }
  if (faceID / kFaceNumX >= FACE_MAX) {
    return; // Impossible number
  }

  assert(text_topleft.x == FACE_W);

  if (face_state == MFACE_NONE) {
    face_state = MFACE_OPEN;
    face_id = faceID;
  } else {
    face_state = MFACE_NEXT;
    next_face = faceID;
  }

  face_time = 0;
}

// Send command
void MsgWindow::Cmd(uint8_t cmd) {
  int temp = 0;
  int Ysize = 0;

  switch (cmd) {
  case MWCMD_LARGEFONT: // Use large font
    Ysize += 8;
    [[fallthrough]];
  case MWCMD_NORMALFONT: // Use normal font
    Ysize += 2;
    [[fallthrough]];
  case MWCMD_SMALLFONT: // Use small font
    Ysize += 14;
    temp = max_size.bottom - max_size.top - 16;
    max_line = temp / Ysize; // Max displayable lines
    font_dy = ((temp % Ysize) / (temp / Ysize)) + Ysize + 1; // Y increment
    font_id = Cast::down_enum<FONT_ID>(cmd);                 // Font to use
    [[fallthrough]];

  case MWCMD_NEWPAGE: // New page
    // Invalidate strings, go to first line
    MsgBlank();
    break;

  default: // Bug if we get here...
    break;
  }
}

// Send help string
void MsgWindow::Help(MenuController *ws) {
  // Search active window and clear message area
  const auto *p = ws->SearchActive();
  MsgBlank();

  // Assign a single row of strings
  Msg(p->ItemPtr[ws->CurrentSelection()]->Help);
}
