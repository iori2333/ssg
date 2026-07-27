///
/// MsgWindow - Message window processing
///

#include <utility>

#include "msg_window.h"

#include "data/graphics_assets.h"
#include "platform/text_backend.h"
#include "util/ut_math.h"

namespace {

constexpr auto kFaceColumns = 6;

inline constexpr auto MWIN_DEAD = 0x00;
inline constexpr auto MWIN_OPEN = 0x01;
inline constexpr auto MWIN_CLOSE = 0x02;
inline constexpr auto MWIN_FREE = 0x03;

inline constexpr auto MFACE_NONE = 0x00;
inline constexpr auto MFACE_OPEN = 0x01;
inline constexpr auto MFACE_CLOSE = 0x02;
inline constexpr auto MFACE_NEXT = 0x03;
inline constexpr auto MFACE_WAIT = 0x04;

constexpr PIXEL_COORD FACE_W = 96;
constexpr PIXEL_COORD FACE_H = 96;

void DrawWindowFrame(int x, int y, int w, int h) {
  w >>= 1;
  h >>= 1;

  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, {0, 0, w, h});
  GrpSurface_Blit({x + w, y}, SURFACE_ID::SYSTEM, {384 - w, 0, 384, h});
  GrpSurface_Blit({x, y + h}, SURFACE_ID::SYSTEM, {0, 80 - h, w, 80});
  GrpSurface_Blit({x + w, y + h}, SURFACE_ID::SYSTEM,
                  {384 - w, 80 - h, 384, 80});
}

} // namespace

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

  SetFont(FONT_ID::NORMAL);

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
  const auto sid = data::graphics_assets::FaceSurface(face_id / kFaceColumns);
  switch (face_state) {
  case MFACE_WAIT:
    oy = max_size.bottom - 100;
    src = PIXEL_LTWH{((face_id % kFaceColumns) * FACE_W), 0, FACE_W, FACE_H};
    GrpSurface_Blit({(x + 2), oy}, sid, src);
    break;

  case MFACE_OPEN:
    time = face_time >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 153), (64 - time) / 2);
      src = PIXEL_LTWH{((face_id % kFaceColumns) * FACE_W), i, FACE_W, 1};
      GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case MFACE_NEXT:
    time = (255 - face_time) >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 153), (64 - time) / 2);
      src = PIXEL_LTWH{((face_id % kFaceColumns) * FACE_W), i, FACE_W, 1};
      GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case MFACE_CLOSE:
    time = face_time >> 1;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < FACE_H; i++) {
      len = cosl(time + (i * 4), time);
      src = PIXEL_LTWH{((face_id % kFaceColumns) * FACE_W), i, FACE_W, 1};
      if ((i & 1) != 0) {
        GrpSurface_Blit({(x - len + 2), (oy + i)}, sid, src);
      } else {
        GrpSurface_Blit({(x + len + 2), (oy + i)}, sid, src);
      }
    }
    break;
  }
}

void MsgWindow::AppendMessage(std::string_view message) {
  int Line = 0;
  int i = 0;

  Line = line;

  if (std::cmp_equal(Line, max_line)) {
    // When already exceeding max display lines
    for (i = 1; i < max_line - 1; i++) {
      msg[i] = msg[i + 1];
    }
    msg[Line - 1] = message;
  } else {
    // Set pointer and update line count
    msg[Line] = message;
    line = Line + 1;
  }

  text.clear();
  for (decltype(line) i = 0; i < line; i++) {
    text += msg[i];
    text += '\n';
  }
}

// Set the face
void MsgWindow::SetFace(uint8_t face_id) {
  if (state == MWIN_DEAD) {
    return; // Cannot display
  }
  if (face_id / kFaceColumns >= data::graphics_assets::kFaceSurfaceCount) {
    return; // Impossible number
  }

  assert(text_topleft.x == FACE_W);

  if (face_state == MFACE_NONE) {
    face_state = MFACE_OPEN;
    this->face_id = face_id;
  } else {
    face_state = MFACE_NEXT;
    next_face = face_id;
  }

  face_time = 0;
}

void MsgWindow::SetFont(FONT_ID font) {
  int font_height = 0;
  switch (font) {
  case FONT_ID::SMALL:
    font_height = 14;
    break;
  case FONT_ID::NORMAL:
    font_height = 16;
    break;
  case FONT_ID::LARGE:
    font_height = 24;
    break;
  case FONT_ID::TINY:
    font_height = 10;
    break;
  case FONT_ID::COUNT:
    return;
  }

  const int text_height = max_size.bottom - max_size.top - 16;
  max_line = text_height / font_height;
  font_dy = ((text_height % font_height) / max_line) + font_height + 1;
  font_id = font;
  NewPage();
}

void MsgWindow::NewPage() { MsgBlank(); }

void MsgWindow::ShowHelp(std::string_view help) {
  MsgBlank();
  if (!help.empty()) {
    AppendMessage(help);
  }
}
