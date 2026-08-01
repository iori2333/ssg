/// Music room UI state and rendering.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "audio/bgm/midi/midi_sequencer.h"
#include "gfx/coords.h"
#include "platform/text_backend.h"
#include "sys/input.h"

class MusicPlayer;

namespace audio {
class AudioSystem;
}

namespace i18n {
class Localization;
}

namespace data {
class GraphicsLoader;
} // namespace data

class MusicRoomScene {
public:
  MusicRoomScene(data::GraphicsLoader &graphics, MusicPlayer &music,
                 i18n::Localization &localization, audio::AudioSystem &audio)
      : graphics_(graphics), music_(music), localization_(localization),
        audio_(audio) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] bool Update(InputBits input, InputBits system_input,
                            bool should_draw);

private:
  struct Text {
    TextRenderRectId mid_dev;
    TextRenderRectId title;
    TextRenderRectId comment;
    TextRenderRectId version;
    void RenderVersion(WindowPoint topleft, std::string_view value) const;
    void RenderMidDev(WindowPoint topleft, std::string_view value) const;
    void RenderTitle(WindowPoint topleft, std::size_t track_id,
                     std::string_view track_title,
                     uint32_t marquee_frame) const;
    void RenderComment(WindowPoint topleft,
                       std::string_view comment_text) const;
  };

  void DrawNotes();
  void DrawSpectrum(int x, int y);

  std::size_t track_id_ = 0;
  std::optional<Text> text_;
  InputBits previous_input_ = 0;
  bool device_change_wait_ = false;
  uint32_t title_marquee_frame_ = 0;
  std::array<uint16_t, 144> spectrum_peaks_{};
  uint8_t spectrum_decay_frame_ = 0;
  audio::bgm::Visualization midi_visualization_{};

  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  i18n::Localization &localization_;
  audio::AudioSystem &audio_;
};
