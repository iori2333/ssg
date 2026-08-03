///
/// MsgWindow - Message window processing
///

#include <cstddef>
#include <cstdint>
#include <utility>

#include "msg_window.h"

#include "data/graphics_assets.h"
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/graphics.h"
#include "gfx/render/geometry.h"
#include "gfx/text/text.h"
#include "gfx/text/text_renderer.h"
#include "sys/log.h"
#include "util/math_utils.h"

namespace {

constexpr std::size_t kFaceColumns = 6;

constexpr int kFaceWidth = 96;
constexpr int kFaceHeight = 96;

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

void MsgWindow::Init(const Rect &rc, MsgWindowFlags flags) {
  max_size = rc;
  this->flags = flags;
  text_topleft = {
      .x = (!!(flags & MsgWindowFlags::WithFace) ? kFaceWidth : 8),
      .y = 8,
  };
  trr = TextRenderer().Register(rc.Size() - text_topleft);
}

void MsgWindow::Open() {
  if (state != State::Dead) {
    return;
  }

  // Set state and final values
  state = State::Open;
  face_state = FaceState::None; // Display nothing
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

  face_state = FaceState::Close;
  state = State::Close;
}

// Force close the message window
void MsgWindow::ForceClose() {
  face_state = FaceState::None;
  state = State::Dead;
}

// Tick the message window
void MsgWindow::Tick() {
  const auto advance_face = [this] {
    face_time = (face_time + kFaceFrameStep) % kFaceFrameCount;
    return face_time == 0;
  };

  switch (face_state) {
  case FaceState::Open: // Attempting to display face
    if (advance_face()) {
      face_state = FaceState::Wait;
    }
    break;

  case FaceState::Next:
    if (advance_face()) {
      face_state = FaceState::Open;
      face_id = next_face;
    }
    break;

  case FaceState::Close: // Attempting to hide face
    if (advance_face()) {
      face_state = FaceState::None;
    }
    break;

  default:
    break;
  }

  switch (state) {
  case State::Open:
    now_size.top -= 2;
    now_size.bottom += 2;

    // When fully opened
    if (now_size.top <= max_size.top) {
      now_size = max_size;
      state = State::Free;
    }
    break;

  case State::Close:
    now_size.top += 3;
    now_size.bottom -= 3;
    now_size.right += 6;
    now_size.left -= 6;

    // When fully closed
    if (now_size.top >= now_size.bottom) {
      state = State::Dead;
    }
    break;

  case State::Dead:
  case State::Free:
  default:
    break;
  }
}

// Draw the message window (same as above)
void MsgWindow::Draw() {
  Rect src;

  const auto x = now_size.left;         // Window top-left X
  const auto y = now_size.top;          // Window top-left Y
  const auto w = (now_size.right - x);  // Window width
  const auto h = (now_size.bottom - y); // Window height
  int len = 0;
  int time = 0;
  int oy = 0;

  // Do nothing if the message window is dead
  if (state == State::Dead) {
    return;
  }

  // Draw translucent part
  geometry::SetAlphaNorm(110);
  geometry::SetColor({0, 0, 3});
  geometry::DrawBoxA((x + 4), (y + 4), (x + w - 4), (y + h - 4));

  // Display text only when window is [FREE]
  // -> Otherwise a Surface for text would have to be created...
  if ((state == State::Free) && trr) {
    const auto topleft = (PixelPoint{x, y} + text_topleft);
    const auto trr = this->trr.value();
    const auto &text = this->text;
    TextRenderer().Render(topleft, trr, text, [this](TextRenderSession &s) {
      // Draw with the set font
      s.SetFont(font_id);
      for (std::size_t i = 0; i < line; i++) {
        const auto m = msg[i];

        // Safety measure
        if (m.empty()) {
          continue;
        }
        const int top = (i * font_dy);
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
  case FaceState::Wait:
    oy = max_size.bottom - 100;
    src =
        Rect::FromLtwh(static_cast<int>((face_id % kFaceColumns) * kFaceWidth),
                       0, kFaceWidth, kFaceHeight);
    GraphicsSurfaceBlit({(x + 2), oy}, sid, src);
    break;

  case FaceState::Open:
    time = face_time >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < kFaceHeight; i++) {
      len = math::RoundedPolarVector(static_cast<float>(time + (i * 153)) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>(64 - time) / 2.0F)
                .x;
      src = Rect::FromLtwh(
          static_cast<int>((face_id % kFaceColumns) * kFaceWidth), i,
          kFaceWidth, 1);
      GraphicsSurfaceBlit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case FaceState::Next:
    time = (255 - face_time) >> 2;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < kFaceHeight; i++) {
      len = math::RoundedPolarVector(static_cast<float>(time + (i * 153)) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>(64 - time) / 2.0F)
                .x;
      src = Rect::FromLtwh(
          static_cast<int>((face_id % kFaceColumns) * kFaceWidth), i,
          kFaceWidth, 1);
      GraphicsSurfaceBlit({(x + len + 2), (oy + i)}, sid, src);
    }
    break;

  case FaceState::Close:
    time = face_time >> 1;
    oy = max_size.bottom - 100;
    for (auto i = 0; i < kFaceHeight; i++) {
      len = math::RoundedPolarVector(static_cast<float>(time + (i * 4)) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>(time))
                .x;
      src = Rect::FromLtwh(
          static_cast<int>((face_id % kFaceColumns) * kFaceWidth), i,
          kFaceWidth, 1);
      if ((i & 1) != 0) {
        GraphicsSurfaceBlit({(x - len + 2), (oy + i)}, sid, src);
      } else {
        GraphicsSurfaceBlit({(x + len + 2), (oy + i)}, sid, src);
      }
    }
    break;
  default:
    break;
  }
}

void MsgWindow::AppendMessage(std::string_view message) {
  const auto Line = line;

  if (std::cmp_equal(Line, max_line)) {
    // When already exceeding max display lines
    if (max_line > 1) {
      for (std::size_t i = 1; i < max_line - 1; i++) {
        msg[i] = msg[i + 1];
      }
    }
    if (Line > 0) {
      msg[Line - 1] = message;
    }
  } else {
    // Set pointer and update line count
    msg[Line] = message;
    line = Line + 1;
  }

  text.clear();
  for (std::size_t i = 0; i < line; i++) {
    text += msg[i];
    text += '\n';
  }
}

// Set the face
void MsgWindow::SetFace(std::size_t face_id) {
  if (state == State::Dead) {
    return; // Cannot display
  }
  if (face_id / kFaceColumns >= data::graphics_assets::kFaceSurfaceCount) {
    return; // Impossible number
  }

  if (text_topleft.x != kFaceWidth) {
    logging::Critical(logging::Channel::Ui,
                      "Cannot set a face on a message window without face "
                      "layout");
    return;
  }

  if (face_state == FaceState::None) {
    face_state = FaceState::Open;
    this->face_id = face_id;
  } else {
    face_state = FaceState::Next;
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
