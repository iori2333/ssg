/// Music room UI state and rendering.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "audio/midi.h"
#include "gfx/coords.h"
#include "platform/text_backend.h"
#include "sys/input.h"

class MusicPlayer;

namespace i18n {
class Localization;
}

namespace data {
class GraphicsLoader;
} // namespace data

class MusicRoomScene {
public:
  MusicRoomScene(data::GraphicsLoader &graphics, MusicPlayer &music,
                 i18n::Localization &localization)
      : graphics_(graphics), music_(music), localization_(localization) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] bool Update(INPUT_BITS input, INPUT_BITS system_input,
                            bool should_draw);

private:
  struct Text {
    TEXTRENDER_RECT_ID mid_dev;
    TEXTRENDER_RECT_ID title;
    TEXTRENDER_RECT_ID comment;
    TEXTRENDER_RECT_ID version;
    void RenderVersion(WINDOW_POINT topleft, std::string_view value) const;
    void RenderMidDev(WINDOW_POINT topleft) const;
    void RenderTitle(WINDOW_POINT topleft, std::size_t track_id,
                     std::string_view track_title,
                     uint32_t marquee_frame) const;
    void RenderComment(WINDOW_POINT topleft,
                       std::string_view comment_text) const;
  };

  void DrawNotes();
  void DrawSpectrum(int x, int y);

  std::size_t track_id_ = 0;
  std::optional<Text> text_;
  INPUT_BITS previous_input_ = 0;
  bool device_change_wait_ = false;
  uint32_t title_marquee_frame_ = 0;
  std::array<uint16_t, 144> spectrum_peaks_{};
  uint8_t spectrum_decay_frame_ = 0;
  MID_VISUALIZATION midi_visualization_{};

  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  i18n::Localization &localization_;
};
