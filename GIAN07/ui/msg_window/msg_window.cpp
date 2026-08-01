///
/// MsgWindow - Message window processing
///

#include <utility>

#include "msg_window.h"

#include "data/graphics_assets.h"
#include "platform/text_backend.h"
#include "util/math_utils.h"

namespace {

constexpr auto kFaceColumns = 6;

inline constexpr auto kMsgWindowDead = 0x00;
inline constexpr auto kMsgWindowOpen = 0x01;
inline constexpr auto kMsgWindowClose = 0x02;
inline constexpr auto kMsgWindowFree = 0x03;

inline constexpr auto kMsgFaceNone = 0x00;
inline constexpr auto kMsgFaceOpen = 0x01;
inline constexpr auto kMsgFaceClose = 0x02;
inline constexpr auto kMsgFaceNext = 0x03;
inline constexpr auto kMsgFaceWait = 0x04;

constexpr PixelCoord kFaceWidth = 96;
constexpr PixelCoord kFaceHeight = 96;

void DrawWindowFrame(int x, int y, int w, int h) {
  w >>= 1;
  h >>= 1;

  GraphicsSurfaceBlit({x, y}, SurfaceId::System, {0, 0, w, h});
  GraphicsSurfaceBlit({x + w, y}, SurfaceId::System, {384 - w, 0, 384, h});
  GraphicsSurfaceBlit({x, y + h}, SurfaceId::System, {0, 80 - h, w, 80});
  GraphicsSurfaceBlit({x + w, y + h}, SurfaceId::System,
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

void MsgWindow::Init(const WindowLtrb &rc, MsgWindowFlags flags) {
  max_size = rc;
  this->flags = flags;
  text_topleft = {
      .x = (!!(flags & MsgWindowFlags::WithFace) ? kFaceWidth : 8),
      .y = 8,
  };
  trr = TextRenderer().Register(rc.Size() - text_topleft);
}

void MsgWindow::Open() {
  if (state != kMsgWindowDead) {
    return;
  }

  // Set state and final values
  state = kMsgWindowOpen;
  face_state = kMsgFaceNone; // Display nothing
  face_id = 0;
  face_time = 0;

  // Set initial rectangle values
  const auto y_mid = ((max_size.bottom + max_size.top) / 2);
  now_size.left = max_size.left;
  now_size.right = max_size.right;
  now_size.top = y_mid - 4;
  now_size.bottom = y_mid + 4;

  SetFont(FontId::Normal);

  // Initialize display contents in the window
  MsgBlank();
}

// Close the message window
void MsgWindow::Close() {
  //	if(state != kMsgWindowFree) return;

  face_state = kMsgFaceClose;
  state = kMsgWindowClose;
}

// Force close the message window
void MsgWindow::ForceClose() {
  face_state = kMsgFaceNone;
  state = kMsgWindowDead;
}

// Tick the message window
void MsgWindow::Tick() {
  switch (face_state) {
  case kMsgFaceOpen: // Attempting to display face
    face_time += 16;
    if (face_time == 0) {
      face_state = kMsgFaceWait;
    }
    break;

  case kMsgFaceNext:
    face_time += 16;
    if (face_time == 0) {
      face_state = kMsgFaceOpen;
      face_id = next_face;
    }
    break;

  case kMsgFaceClose: // Attempting to hide face
    face_time += 16;
    if (face_time == 0) {
      face_state = kMsgFaceNone;
    }
    break;

  default:
    break;
  }

  switch (state) {
  case kMsgWindowOpen:
    now_size.top -= 2;
    now_size.bottom += 2;

    // When fully opened
    if (now_size.top <= max_size.top) {
      now_size = max_size;
      state = kMsgWindowFree;
    }
    break;

  case kMsgWindowClose:
    now_size.top += 3;
    now_size.bottom -= 3;
    now_size.right += 6;
    now_size.left -= 6;

    // When fully closed
    if (now_size.top >= now_size.bottom) {
      state = kMsgWindowDead;
    }
    break;

  case kMsgWindowDead:
  case kMsgWindowFree:
    break;
  }
}

// Draw the message window (same as above)
void MsgWindow::Draw() {
  PixelLtrb src;

  const auto x = now_size.left;         // Window top-left X
  const auto y = now_size.top;          // Window top-left Y
  const auto w = (now_size.right - x);  // Window width
  const auto h = (now_size.bottom - y); // Window height
  int len = 0;
  int time = 0;
  int oy = 0;

  // Do nothing if the message window is dead
  if (state == kMsgWindowDead) {
    return;
  }

  // Draw translucent part
  Geometry().SetAlphaNorm(110);
  Geometry().SetColor({0, 0, 3});
  Geometry().DrawBoxA((x + 4), (y + 4), (x + w - 4), (y + h - 4));

  // Display text only when window is [FREE]
  // -> Otherwise a Surface for text would have to be created...
  if ((state == kMsgWindowFree) && trr) {
    const auto topleft = (WindowPoint{x, y} + text_topleft);
    const auto trr = this->trr.value();
    const auto &text = this->text;
    TextRenderer().Render(topleft, trr, text, [this](TextRenderSession &s) {
      // Draw with the set font
      s.SetFont(font_id);
      for (auto i = 0; std::cmp_less(i, line); i++) {
        const auto m = msg[i];

        // Safety measure
        if (m.empty()) {
          continue;
        }
        const PixelCoord top = (i * font_dy);
        const auto left =
            (!!(flags & MsgWindowFlags::Center) ? TextLayoutXCenter(s, m) : 0);

        // Draw offset by 1 pixel in gray
        s.Put({.x = (left + 1), .y = top}, m,
              Rgb{.r = 128, .g = 128, .b = 128});
        // Draw in white at the correct position
        s.Put({.x = (left + 0), .y = top}, m,
              Rgb{.r = 255, .g = 255, .b = 255});
      }
    });
  }

  DrawWindowFrame(x, y, w, h);

  // Draw face (only when display is requested)
  const auto sid = data::graphics_assets::FaceSurface(face_id / kFaceColumns);
  switch (face_state) {
  case kMsgFaceWait:
    oy = max_size.bottom - 100;
    src = PixelLtwh{((face_id % kFaceColumns) * kFaceWidth), 0, kFaceWidth,
                    kFaceHeight};
    GraphicsSurfaceBlit({(x + 2), oy}, sid, src);
    break;

  case kMsgFaceOpen:
    time = face_time >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < kFaceHeight; i++) {
      len = math::RoundedPolarVector(static_cast<float>(time + (i * 153)) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>(64 - time) / 2.0f)
                .x;
      src =
          PixelLtwh{((face_id % kFaceColumns) * kFaceWidth), i, kFaceWidth, 1};
      GraphicsSurfaceBlit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case kMsgFaceNext:
    time = (255 - face_time) >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < kFaceHeight; i++) {
      len = math::RoundedPolarVector(static_cast<float>(time + (i * 153)) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>(64 - time) / 2.0f)
                .x;
      src =
          PixelLtwh{((face_id % kFaceColumns) * kFaceWidth), i, kFaceWidth, 1};
      GraphicsSurfaceBlit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case kMsgFaceClose:
    time = face_time >> 1;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < kFaceHeight; i++) {
      len = math::RoundedPolarVector(static_cast<float>(time + (i * 4)) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>(time))
                .x;
      src =
          PixelLtwh{((face_id % kFaceColumns) * kFaceWidth), i, kFaceWidth, 1};
      if ((i & 1) != 0) {
        GraphicsSurfaceBlit({(x - len + 2), (oy + i)}, sid, src);
      } else {
        GraphicsSurfaceBlit({(x + len + 2), (oy + i)}, sid, src);
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
  if (state == kMsgWindowDead) {
    return; // Cannot display
  }
  if (face_id / kFaceColumns >= data::graphics_assets::kFaceSurfaceCount) {
    return; // Impossible number
  }

  assert(text_topleft.x == kFaceWidth);

  if (face_state == kMsgFaceNone) {
    face_state = kMsgFaceOpen;
    this->face_id = face_id;
  } else {
    face_state = kMsgFaceNext;
    next_face = face_id;
  }

  face_time = 0;
}

void MsgWindow::SetFont(FontId font) {
  int font_height = 0;
  switch (font) {
  case FontId::Small:
    font_height = 14;
    break;
  case FontId::Normal:
    font_height = 16;
    break;
  case FontId::Large:
    font_height = 24;
    break;
  case FontId::Tiny:
    font_height = 10;
    break;
  case FontId::Count:
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
