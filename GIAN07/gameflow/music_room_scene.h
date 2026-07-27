/// Music room page state and rendering.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "gfx/coords.h"
#include "platform/text_backend.h"
#include "sys/input.h"

class MusicRoomScene {
public:
  [[nodiscard]] bool Enter();
  void Update(bool &);

private:
  struct Text {
    TEXTRENDER_RECT_ID mid_dev;
    TEXTRENDER_RECT_ID title;
    TEXTRENDER_RECT_ID comment;
    TEXTRENDER_RECT_ID version;
    std::string_view comment_text;

    void RenderVersion(WINDOW_POINT topleft) const;
    void RenderMidDev(WINDOW_POINT topleft) const;
    void RenderTitle(WINDOW_POINT topleft, std::size_t track_id) const;
    void RenderComment(WINDOW_POINT topleft) const;
  };

  void DrawNotes();
  void DrawSpectrum(int x, int y);

  std::size_t track_id_ = 0;
  std::optional<Text> text_;
  INPUT_BITS previous_input_ = 0;
  bool device_change_wait_ = false;
  std::array<uint16_t, 144> spectrum_peaks_{};
  uint8_t spectrum_decay_frame_ = 0;
};
