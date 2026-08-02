/// Music room UI scene.

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "music_room_scene.h"

#include "audio/audio_system.h"
#include "audio/core/audio_types.h"
#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/text.h"
#include "gfx/text_ttf.h"
#include "i18n/localization.h"
#include "music/music_player.h"
#include "sys/input.h"
#include "sys/log.h"
#include "ui/text_marquee.h"
#include "util/math_utils.h"

// Constants
// ---------

static constexpr Rgb ColorHighlight = {.r = 51, .g = 102, .b = 153};
static constexpr Rgb ColorDefault = {.r = 153, .g = 204, .b = 255};
static constexpr int TitleAreaWidth = 232;
// ---------

// State
// -----

void MusicRoomScene::Text::RenderVersion(WindowPoint topleft,
                                         std::string_view value) const {
  TextRenderer().Render(topleft, version, value, [value](TextRenderSession &s) {
    s.SetFont(FontId::Small);
    s.SetColor(ColorDefault);
    s.Put({.x = 0, .y = 0}, value);
  });
}

void MusicRoomScene::Text::RenderMidDev(WindowPoint topleft,
                                        std::string_view value) const {
  if (value.empty()) {
    return;
  }
  std::string_view dev = value.substr(0, std::min(value.size(), 13UZ));
  TextRenderer().Render(topleft, mid_dev, dev, [&dev](TextRenderSession &s) {
    s.SetFont(FontId::Small);
    s.SetColor(ColorDefault);
    s.Put({.x = 0, .y = 0}, dev);
  });
}

void MusicRoomScene::Text::RenderTitle(WindowPoint topleft,
                                       std::size_t track_id,
                                       std::string_view track_title,
                                       int marquee_frame) const {
  // Some modders might assign the same title to consecutive tracks, but it's
  // not possible to change the track title without switching to a different
  // track first.
  auto num_str = std::format("#{:02}", (track_id + 1));
  std::string_view num = num_str;

  const auto cache_key = std::format("{}|{}|{}", num, track_title,
                                     marquee_frame / ui::kMarqueeStepFrames);
  TextRenderer().Render(
      topleft, title, cache_key,
      [&num, track_title, marquee_frame](TextRenderSession &s) {
        s.SetFont(FontId::Normal);
        // GDI would calculate a trailing space as 4 pixels wide, not 8.
        const auto title_left = (s.Extent(num).w + 8);

        const auto display_title = ui::MarqueeWindow(
            s, track_title, s.RectSize().w - title_left - 1, marquee_frame);
        s.Put({.x = 1, .y = 0}, num, ColorHighlight);
        s.Put({.x = (title_left + 1), .y = 0}, display_title, ColorHighlight);
        s.Put({.x = 0, .y = 0}, num, ColorDefault);
        s.Put({.x = (title_left + 0), .y = 0}, display_title, ColorDefault);
      });
}

void MusicRoomScene::Text::RenderComment(WindowPoint topleft,
                                         std::string_view comment_text) const {
  if (comment_text.empty()) {
    return;
  }
  TextRenderer().Render(
      topleft, comment, comment_text, [comment_text](TextRenderSession &s) {
        int y = 0;
        s.SetFont(FontId::Small);
        s.SetColor(ColorDefault);

        size_t pos = 0;
        while (pos < comment_text.size()) {
          const auto nl = comment_text.find('\n', pos);
          const auto line = comment_text.substr(pos, nl - pos);
          if (!line.empty() || nl != std::string_view::npos) {
            s.Put({.x = 0, .y = y}, line);
            y += 16;
          }
          if (nl == std::string_view::npos) {
            break;
          }
          pos = nl + 1;
        }
      });
}

bool MusicRoomScene::Enter() {
  TextRenderer().Clear();
  GraphicsBackendClear();
  GraphicsFlip();

  if (!graphics_.LoadMusicRoom()) {
    logging::Error(logging::Channel::Ui, "Failed to load Music Room graphics");
    return false;
  }

  GraphicsBackendSetClip(kGameResolutionRect);

  track_id_ = 0;
  previous_input_ = 0;
  device_change_wait_ = false;
  title_marquee_frame_ = 0;
  spectrum_peaks_.fill(0);
  spectrum_decay_frame_ = 0;

  audio_.SetBgmTempo(0);

  if (music_.TrackCount() == 0 || localization_.MusicComment(0).empty()) {
    logging::Error(logging::Channel::I18n,
                   "Music Room text catalog is invalid");
    return false;
  }

  text_ = Text{
      .mid_dev = TextRenderer().Register({.w = 98, .h = 13}),
      .title = TextRenderer().Register({.w = TitleAreaWidth, .h = 16}),
      .comment = TextRenderer().Register({.w = 272, .h = 192}),
      .version = TextRenderer().Register({.w = 490, .h = 13}),
  };

  return true;
}

// Spectrum analyzer drawing
void MusicRoomScene::DrawSpectrum(int x, int y) {
  std::array<int, 128 + 8 + 8> ftable{};
  std::array<int, 128> ftable2{};

  constexpr PixelLtrb src = {(16 * 16), 0, ((16 * 16) + (8 * 21)), 8}; // ,,,8*4

  spectrum_decay_frame_ = ((spectrum_decay_frame_ + 1) % 5);

  for (int i = 0; i < std::size(ftable2); i++) {
    int temp = 0;
    int temp2 = 0;
    for (const auto j : std::views::iota(0, 16)) {
      temp += midi_visualization_.play[j][i];
      temp2 += ((midi_visualization_.play[j][i] != 0U) ? 1 : 0);
    }
    if (temp2 == 0) {
      temp2 = 1;
    }
    ftable2[i] = (temp / temp2);
  }

  const auto attenuation = [](int angle) {
    return 256 - math::RoundedPolarVector(
                     static_cast<float>(angle) * math::kLegacyAngleStep, 256.0F)
                     .y;
  };
  for (int i = 0; i < std::size(ftable); i++) {
    ftable[i] =
        ((i >= 8 && i <= 128 + 7) ? ftable2[i - 8] : 0) * attenuation(0) / 256;

    ftable[i] +=
        ((i >= 9 && i <= 128 + 8) ? ftable2[i - 9] : 0) * attenuation(8) / 256;
    ftable[i] += ((i >= 10 && i <= 128 + 9) ? ftable2[i - 10] : 0) *
                 attenuation(16) / 256;
    ftable[i] += ((i >= 11 && i <= 128 + 10) ? ftable2[i - 11] : 0) *
                 attenuation(24) / 256;
    ftable[i] += ((i >= 12 && i <= 128 + 11) ? ftable2[i - 12] : 0) *
                 attenuation(32) / 256;
    ftable[i] += ((i >= 13 && i <= 128 + 12) ? ftable2[i - 13] : 0) *
                 attenuation(40) / 256;
    ftable[i] += ((i >= 14 && i <= 128 + 13) ? ftable2[i - 14] : 0) *
                 attenuation(48) / 256;
    ftable[i] += ((i >= 15 && i <= 128 + 14) ? ftable2[i - 15] : 0) *
                 attenuation(56) / 256;
    ftable[i] += ((i >= 16 && i <= 128 + 15) ? ftable2[i - 16] : 0) *
                 attenuation(63) / 256;

    ftable[i] +=
        ((i <= 128 + 6 && i >= 7) ? ftable2[i - 7] : 0) * attenuation(8) / 256;
    ftable[i] +=
        ((i <= 128 + 5 && i >= 6) ? ftable2[i - 6] : 0) * attenuation(16) / 256;
    ftable[i] +=
        ((i <= 128 + 4 && i >= 5) ? ftable2[i - 5] : 0) * attenuation(24) / 256;
    ftable[i] +=
        ((i <= 128 + 3 && i >= 4) ? ftable2[i - 4] : 0) * attenuation(32) / 256;
    ftable[i] +=
        ((i <= 128 + 2 && i >= 3) ? ftable2[i - 3] : 0) * attenuation(40) / 256;
    ftable[i] +=
        ((i <= 128 + 1 && i >= 2) ? ftable2[i - 2] : 0) * attenuation(48) / 256;
    ftable[i] +=
        ((i <= 128 + 0 && i >= 1) ? ftable2[i - 1] : 0) * attenuation(56) / 256;
    ftable[i] +=
        ((i <= 128 - 1 && i >= 0) ? ftable2[i - 0] : 0) * attenuation(63) / 256;

    ftable[i] >>= 3;

    if (spectrum_peaks_[i] < ftable[i]) {
      spectrum_peaks_[i] = ftable[i];
    } else if ((spectrum_decay_frame_ == 0U) && (spectrum_peaks_[i] != 0U)) {
      spectrum_peaks_[i]--;
    }
  }

  // GraphicsSurfaceBlit({ (SPECT_X - 7), SPECT_Y }, SurfaceId::System, src);

  for (int i = 0; i < std::size(ftable); i++) {
    constexpr Rgb c1 = {.r = 200, .g = 0, .b = 0};
    constexpr Rgb c2 = {.r = 250, .g = 250, .b = 0};
    geometry::DrawGrdLineEx((i + x), (y - (ftable[i] * 2)), c1, y, c2);
  }
}

// Display pressed notes
void MusicRoomScene::DrawNotes() {
  // 0123456789ab (Mod c)
  // o#o#oo#o#o#o
  // o o oo o o o

  constexpr std::array<PixelLtrb, 12> src = {
      PixelLtrb{0, 464, 3, 474}, // white
      PixelLtrb{0, 456, 3, 461}, // black

      PixelLtrb{4, 464, 7, 474}, // white
      PixelLtrb{0, 456, 3, 461}, // black

      PixelLtrb{8, 464, 11, 474},  // white
      PixelLtrb{12, 464, 15, 474}, // white

      PixelLtrb{0, 456, 3, 461},   // black
      PixelLtrb{16, 464, 19, 474}, // white

      PixelLtrb{0, 456, 3, 461},   // black
      PixelLtrb{20, 464, 23, 474}, // white

      PixelLtrb{0, 456, 3, 461},   // black
      PixelLtrb{24, 464, 27, 474}, // white
  };

  constexpr std::array<PixelLtrb, 12> src2 = {
      PixelLtrb{0, (464 - 24), 3, (474 - 24)}, // white
      PixelLtrb{0, (456 - 24), 3, (461 - 24)}, // black

      PixelLtrb{4, (464 - 24), 7, (474 - 24)}, // white
      PixelLtrb{0, (456 - 24), 3, (461 - 24)}, // black

      PixelLtrb{8, (464 - 24), 11, (474 - 24)},  // white
      PixelLtrb{12, (464 - 24), 15, (474 - 24)}, // white

      PixelLtrb{0, (456 - 24), 3, (461 - 24)},   // black
      PixelLtrb{16, (464 - 24), 19, (474 - 24)}, // white

      PixelLtrb{0, (456 - 24), 3, (461 - 24)},   // black
      PixelLtrb{20, (464 - 24), 23, (474 - 24)}, // white

      PixelLtrb{0, (456 - 24), 3, (461 - 24)},   // black
      PixelLtrb{24, (464 - 24), 27, (474 - 24)}, // white
  };

  constexpr std::array<PixelCoord, 12> destX = {
      0, // white
      2, // black

      4, // white
      6, // black

      8,  // white
      12, // white

      14, // black
      16, // white

      18, // black
      20, // white

      22, // black
      24, // white
  };

  PixelLtrb rc;

  for (const auto Track : std::views::iota(0, 16)) {
    const auto top = (22 + (Track * 24));
    const auto pan = (static_cast<int8_t>(midi_visualization_.pan[Track]) - 64);
    DrawFontMid(50, top, midi_visualization_.volume[Track]);
    DrawFontMid(125, top, midi_visualization_.expression[Track]);
    DrawFontMid(181, top, pan);

    int LevelSum = 0;
    int num = 0;
    for (const auto NoteNo : std::views::iota(0, 128)) {
      if (midi_visualization_.note_highlights[Track][NoteNo] != 0U) {
        const auto x = (40 + destX[NoteNo % 12] + ((NoteNo / 12) * 28));
        const auto y = (9 + (Track * 24));
        GraphicsSurfaceBlit({x, y}, SurfaceId::Music, src[NoteNo % 12]);
      }

      if (midi_visualization_.notes[Track][NoteNo] != 0U) {
        const auto x = (40 + destX[NoteNo % 12] + ((NoteNo / 12) * 28));
        const auto y = (9 + (Track * 24));
        GraphicsSurfaceBlit({x, y}, SurfaceId::Music, src[NoteNo % 12]);
      }
      if (midi_visualization_.levels[Track][NoteNo] != 0U) {
        LevelSum += midi_visualization_.levels[Track][NoteNo];
        num++;
      }
    }

    if (num != 0) {
      rc = PixelLtwh{80, 456, (std::min)((LevelSum / num), 96), 5};
      GraphicsSurfaceBlit({240, (22 + (Track * 24))}, SurfaceId::Music, rc);
    }
  }
}

bool MusicRoomScene::Update(InputBits input, InputBits system_input,
                            bool should_draw) {
  if (!text_) {
    logging::Critical(logging::Channel::Ui,
                      "Cannot update the Music Room before initialization");
    return true;
  }
  auto &text = text_.value();

  auto &audio = audio_;
  const auto snapshot = audio.BgmSnapshot();
  const bool midi_playing = (snapshot.mode == audio::BgmMode::Midi &&
                             snapshot.state == audio::PlaybackState::Playing);

  if (input != previous_input_) {
    if (InputIsCancel(input)) {
      device_change_wait_ = false;
      text_ = std::nullopt;
      return true;
    }
    if ((input == KeyRight) || (input == KeyLeft)) {
      if (input == KeyRight) {
        track_id_ += 2;
      }
      audio.StopBgm();
      const auto track_count = music_.TrackCount();
      track_id_ = ((track_id_ + track_count - 1) % track_count);
      music_.Play(track_id_);
      title_marquee_frame_ = 0;
    }
    previous_input_ = input;
  }

  switch (input) {
  case KeyUp:
    audio.SetBgmTempo(snapshot.tempo + 1);
    break;
  case KeyDown:
    audio.SetBgmTempo(snapshot.tempo - 1);
    break;
  case KeyShift:
    audio.SetBgmTempo(0);
    break;
  default:
    break;
  }

  if ((system_input & SystemKeyBgmFade) != 0) {
    audio.FadeOutBgm(120);
  }

  if (midi_playing && ((system_input & SystemKeyBgmDevice) != 0)) {
    if (!device_change_wait_) {
      audio.ChangeMidiDevice(1);
      device_change_wait_ = true;
    }
  } else {
    // Re-enable if not pressed
    device_change_wait_ = false;
  }

  if (should_draw) {
    midi_visualization_ = audio.MidiVisualization();
    GraphicsBackendClear();

    auto BlitBG = [](const PixelLtwh &rect) {
      GraphicsSurfaceBlit({rect.left, rect.top}, SurfaceId::Music, rect);
    };

    auto BlitLegend = [](const PixelLtwh &rect) {
      const PixelLtrb src = (rect + PixelPoint{.x = 0, .y = 392});
      GraphicsSurfaceBlit({(8 + rect.left), (410 + rect.top)}, SurfaceId::Music,
                          src);
    };

    BlitBG({0, 0, 504, 392});     // From keyboard to spectrum analyzer
    BlitBG({504, 0, 136, 59});    // Down to PASSED TIME
    BlitBG({504, 108, 136, 284}); // Everything below MIDI DEVICE

    BlitLegend({0, 0, 176, 40});    // Left side
    BlitLegend({176, 11, 464, 29}); // Right side without device key

    if (midi_playing) {
      BlitBG({504, 83, 136, 25});    // MIDI DEVICE
      BlitLegend({176, 0, 176, 11}); // Device change key

      // GrpDrawSpect(0,480);
      DrawSpectrum(352, 128);
      DrawNotes();
    }

    const auto millis = snapshot.play_time.count();
    const auto m = ((millis / 1000) / 60);
    const auto s = ((millis / 1000) % 60);
    DrawFont7B(560, 44, std::format("{:02} : {:02}", m, s).c_str());
    // TextOut(hdc,561,40+2,buf,strlen(buf));

    if (midi_visualization_.loaded) {
      BlitBG({504, 59, 136, 24}); // MIDI TIMER
      DrawFont7B(
          560, 68,
          std::format("{:07}", midi_visualization_.play_time.pulse_interpolated)
              .c_str());
      // TextOut(hdc,561,64+2,buf,strlen(buf));
    }

    DrawFont7B(560, 116, std::format("{:3}", snapshot.tempo).c_str());
    // TextOut(hdc,561,112+2,buf,strlen(buf));
    // SetTextColor(hdc,Rgb(255*5/5,255*2/5,255*1/5));

    if (midi_playing) {
      text.RenderMidDev({(540 + 2), (96 - 3)},
                        audio.MidiCurrentDeviceName().value_or(""));
    }
    text.RenderTitle({400, (144 + 2)}, track_id_,
                     localization_.MusicTitle(track_id_), title_marquee_frame_);
    text.RenderComment({(400 - 40), (144 + 30)},
                       localization_.MusicComment(track_id_));
    text.RenderVersion(
        {(200 - 50), 460},
        localization_.Text(i18n::TextIdFromKey("ui.music_room.version")));

    GraphicsFlip();
  }
  title_marquee_frame_++;
  return false;
}
