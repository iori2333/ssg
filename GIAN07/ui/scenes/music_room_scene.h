/// Music room UI state and rendering.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "gfx/coords.h"
#include "platform/text_backend.h"
#include "sys/input.h"

class MusicPlayer;
struct ConfigData;

namespace data {
class GameData;
class GraphicsLoader;
} // namespace data

class MusicRoomScene {
public:
  MusicRoomScene(data::GameData &data, data::GraphicsLoader &graphics,
                 MusicPlayer &music, ConfigData &config)
      : data_(data), graphics_(graphics), music_(music), config_(config) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] bool Update(INPUT_BITS input, INPUT_BITS system_input,
                            bool should_draw);

private:
  struct Text {
    TEXTRENDER_RECT_ID mid_dev;
    TEXTRENDER_RECT_ID title;
    TEXTRENDER_RECT_ID comment;
    TEXTRENDER_RECT_ID version;
    std::string_view comment_text;

    void RenderVersion(WINDOW_POINT topleft) const;
    void RenderMidDev(WINDOW_POINT topleft) const;
    void RenderTitle(WINDOW_POINT topleft, std::size_t track_id,
                     std::string_view track_title) const;
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

  data::GameData &data_;
  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  ConfigData &config_;
};
