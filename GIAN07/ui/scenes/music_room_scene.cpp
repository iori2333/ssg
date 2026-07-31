/// Music room UI scene.

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <cinttypes>
#include <format>

#include "music_room_scene.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/font_uty.h"
#include "gfx/text.h"
#include "i18n/localization.h"
#include "music/music_player.h"
#include "platform/text_backend.h"
#include "sys/input.h"
#include "ui/text_marquee.h"
#include "util/debug.h"
#include "util/math_utils.h"

// Constants
// ---------

static constexpr RGB ColorHighlight = {.r = 51, .g = 102, .b = 153};
static constexpr RGB ColorDefault = {.r = 153, .g = 204, .b = 255};
static constexpr int TitleAreaWidth = 232;
// ---------

// State
// -----

void MusicRoomScene::Text::RenderVersion(WINDOW_POINT topleft,
                                         std::string_view value) const {
  TextObj.Render(topleft, version, value, [value](TEXTRENDER_SESSION &s) {
    s.SetFont(FONT_ID::SMALL);
    s.SetColor(ColorDefault);
    s.Put({.x = 0, .y = 0}, value);
  });
}

void MusicRoomScene::Text::RenderMidDev(WINDOW_POINT topleft) const {
  const auto maybe_dev_full = MidBackend_DeviceName();
  if (!maybe_dev_full) {
    return;
  }
  const auto dev_full = maybe_dev_full.value();
  std::string_view dev = {dev_full.data(), std::min(dev_full.size(), 13UZ)};
  TextObj.Render(topleft, mid_dev, dev, [&dev](TEXTRENDER_SESSION &s) {
    s.SetFont(FONT_ID::SMALL);
    s.SetColor(ColorDefault);
    s.Put({.x = 0, .y = 0}, dev);
  });
}

void MusicRoomScene::Text::RenderTitle(WINDOW_POINT topleft,
                                       std::size_t track_id,
                                       std::string_view track_title,
                                       uint32_t marquee_frame) const {
  // Some modders might assign the same title to consecutive tracks, but it's
  // not possible to change the track title without switching to a different
  // track first.
  auto num_str = std::format("#{:02}", (track_id + 1));
  std::string_view num = num_str;

  const auto cache_key = std::format("{}|{}|{}", num, track_title,
                                     marquee_frame / ui::kMarqueeStepFrames);
  TextObj.Render(
      topleft, title, cache_key,
      [&num, track_title, marquee_frame](TEXTRENDER_SESSION &s) {
        s.SetFont(FONT_ID::NORMAL);
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

void MusicRoomScene::Text::RenderComment(WINDOW_POINT topleft,
                                         std::string_view comment_text) const {
  if (comment_text.empty()) {
    return;
  }
  TextObj.Render(topleft, comment, comment_text,
                 [comment_text](TEXTRENDER_SESSION &s) {
                   int y = 0;
                   s.SetFont(FONT_ID::SMALL);
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
  TextObj.Clear();
  GrpBackend_Clear();
  Grp_Flip();

  if (!graphics_.LoadMusicRoom()) {
    DebugOut("ゲームデータが破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);

  track_id_ = 0;
  previous_input_ = 0;
  device_change_wait_ = false;
  title_marquee_frame_ = 0;
  spectrum_peaks_.fill(0);
  spectrum_decay_frame_ = 0;

  BGM_SetTempo(0);

  // Still necessary because the note arrays aren't actually processed
  // outside of the Music Room.
  Mid_TableInit();

  // BGM_Stop();
  if (music_.TrackCount() == 0 || localization_.MusicComment(0).empty()) {
    DebugOut("Music Room text catalog is invalid");
    return false;
  }

  text_ = Text{
      .mid_dev = TextObj.Register({.w = 98, .h = 13}),
      .title = TextObj.Register({.w = TitleAreaWidth, .h = 16}),
      .comment = TextObj.Register({.w = 272, .h = 192}),
      .version = TextObj.Register({.w = 490, .h = 13}),
  };

  return true;
}

// Spectrum analyzer drawing
void MusicRoomScene::DrawSpectrum(int x, int y) {
  uint16_t ftable[128 + 8 + 8];
  uint16_t ftable2[128];

  constexpr PIXEL_LTRB src = {(16 * 16), 0, ((16 * 16) + (8 * 21)),
                              8}; // ,,,8*4

  spectrum_decay_frame_ = ((spectrum_decay_frame_ + 1) % 5);

  for (int i = 0; i < std::size(ftable2); i++) {
    int temp = 0;
    int temp2 = 0;
    for (const auto j : std::views::iota(0, 16)) {
      temp += Mid_PlayTable[j][i];
      temp2 += ((Mid_PlayTable[j][i] != 0U) ? 1 : 0);
      if (Mid_PlayTable[j][i] != 0) {
        Mid_PlayTable[j][i] -= ((Mid_PlayTable[j][i] >> 3) + 1); // 4
      }
      // if(Mid_PlayTable[j][i])
      // Mid_PlayTable[j][i]-=(Mid_PlayTable[j][i]>>3)+1;
    }
    if (temp2 == 0) {
      temp2 = 1;
    }
    ftable2[i] = (temp / temp2);
  }

  const auto attenuation = [](int angle) {
    return 256 - math::RoundedPolarVector(
                     static_cast<float>(angle) * math::kLegacyAngleStep, 256.0f)
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

  // GrpSurface_Blit({ (SPECT_X - 7), SPECT_Y }, SURFACE_ID::SYSTEM, src);

  GrpGeom->Lock();

  if (auto *gp = GrpGeom_Poly()) {
    for (int i = 0; i < std::size(ftable); i++) {
      // WORD c2 = 0;	//5
      constexpr RGB c1 = {.r = 200, .g = 0, .b = 0};
      constexpr RGB c2 = {.r = 250, .g = 250, .b = 0};
      gp->DrawGrdLineEx((i + x), (y - (ftable[i] * 2)), c1, y, c2);
    }
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({4, 2, 1});
    for (int i = 0; i < std::size(ftable); i++) {
      // WORD c2 = 0;	//5
      if (ftable[i] != 0U) {
        gf->DrawLine((i + x), (y - (ftable[i] * 2)), (i + x), y);
      }
    }
  }

  GrpGeom->Unlock();
}

// Display pressed notes
void MusicRoomScene::DrawNotes() {
  // 0123456789ab (Mod c)
  // o#o#oo#o#o#o
  // o o oo o o o

  constexpr const PIXEL_LTRB src[12] = {
      {0, 464, 3, 474}, // white
      {0, 456, 3, 461}, // black

      {4, 464, 7, 474}, // white
      {0, 456, 3, 461}, // black

      {8, 464, 11, 474},  // white
      {12, 464, 15, 474}, // white

      {0, 456, 3, 461},   // black
      {16, 464, 19, 474}, // white

      {0, 456, 3, 461},   // black
      {20, 464, 23, 474}, // white

      {0, 456, 3, 461},   // black
      {24, 464, 27, 474}, // white
  };

  constexpr const PIXEL_LTRB src2[12] = {
      {0, (464 - 24), 3, (474 - 24)}, // white
      {0, (456 - 24), 3, (461 - 24)}, // black

      {4, (464 - 24), 7, (474 - 24)}, // white
      {0, (456 - 24), 3, (461 - 24)}, // black

      {8, (464 - 24), 11, (474 - 24)},  // white
      {12, (464 - 24), 15, (474 - 24)}, // white

      {0, (456 - 24), 3, (461 - 24)},   // black
      {16, (464 - 24), 19, (474 - 24)}, // white

      {0, (456 - 24), 3, (461 - 24)},   // black
      {20, (464 - 24), 23, (474 - 24)}, // white

      {0, (456 - 24), 3, (461 - 24)},   // black
      {24, (464 - 24), 27, (474 - 24)}, // white
  };

  constexpr const PIXEL_COORD destX[12] = {
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

  PIXEL_LTRB rc;

  for (const auto Track : std::views::iota(0, 16)) {
    const auto top = (22 + (Track * 24));
    const auto pan = (Cast::sign<int8_t>(Mid_PanpodTable[Track]) - 64);
    GrpPutMidNum(50, top, Mid_VolumeTable[Track]);
    GrpPutMidNum(125, top, Mid_ExpressionTable[Track]);
    GrpPutMidNum(181, top, pan);

    int LevelSum = 0;
    int num = 0;
    for (const auto NoteNo : std::views::iota(0, 128)) {
      if (Mid_NoteWTable[Track][NoteNo] != 0U) {
        const auto x = (40 + destX[NoteNo % 12] + ((NoteNo / 12) * 28));
        const auto y = (9 + (Track * 24));
        GrpSurface_Blit({x, y}, SURFACE_ID::MUSIC, src[NoteNo % 12]);
        Mid_NoteWTable[Track][NoteNo]--;
      }

      if (Mid_NoteTable[Track][NoteNo] != 0U) {
        const auto x = (40 + destX[NoteNo % 12] + ((NoteNo / 12) * 28));
        const auto y = (9 + (Track * 24));
        GrpSurface_Blit({x, y}, SURFACE_ID::MUSIC, src[NoteNo % 12]);
      }
      if (Mid_PlayTable2[Track][NoteNo] != 0U) {
        LevelSum += (Mid_PlayTable2[Track][NoteNo]);
        // if(Mid_PlayTable2[Track][NoteNo]>128)
        // Mid_PlayTable2[Track][NoteNo]=128;
        Mid_PlayTable2[Track][NoteNo] -=
            (std::max)((Mid_PlayTable2[Track][NoteNo] / 50), 1);
        num++;
      }
    }

    if (num != 0) {
      rc = PIXEL_LTWH{80, 456, (std::min)((LevelSum / num), 96), 5};
      GrpSurface_Blit({240, (22 + (Track * 24))}, SURFACE_ID::MUSIC, rc);
    }
  }
}

bool MusicRoomScene::Update(INPUT_BITS input, INPUT_BITS system_input,
                            bool should_draw) {
  if (!text_) {
    assert(!"Music Room not initialized?");
    std::unreachable();
  }
  auto &text = text_.value();

  const auto playing = BGM_Playing();

  if (input != previous_input_) {
    if (Input_IsCancel(input)) {
      device_change_wait_ = false;
      text_ = std::nullopt;
      return true;
    }
    if ((input == KEY_RIGHT) || (input == KEY_LEFT)) {
      if (input == KEY_RIGHT) {
        track_id_ += 2;
      }
      BGM_Stop();
      const auto track_count = music_.TrackCount();
      track_id_ = ((track_id_ + track_count - 1) % track_count);
      music_.Play(track_id_);
      title_marquee_frame_ = 0;
    }
    previous_input_ = input;
  }

  switch (input) {
  case KEY_UP:
    BGM_SetTempo(BGM_GetTempo() + 1);
    break;
  case KEY_DOWN:
    BGM_SetTempo(BGM_GetTempo() - 1);
    break;
  case KEY_SHIFT:
    BGM_SetTempo(0);
    break;
  }

  if ((system_input & SYSKEY_BGM_FADE) != 0) {
    BGM_FadeOut(120);
  }

  BGM_UpdateMIDITables();

  if ((playing == BGM_PLAYING::MIDI) &&
      ((system_input & SYSKEY_BGM_DEVICE) != 0)) {
    if (!device_change_wait_) {
      BGM_ChangeMIDIDevice(1);
      device_change_wait_ = true;
    }
  } else {
    // Re-enable if not pressed
    device_change_wait_ = false;
  }

  if (should_draw) {
    GrpBackend_Clear();

    auto BlitBG = [](const PIXEL_LTWH &rect) {
      GrpSurface_Blit({rect.left, rect.top}, SURFACE_ID::MUSIC, rect);
    };

    auto BlitLegend = [](const PIXEL_LTWH &rect) {
      const PIXEL_LTRB src = (rect + PIXEL_POINT{.x = 0, .y = 392});
      GrpSurface_Blit({(8 + rect.left), (410 + rect.top)}, SURFACE_ID::MUSIC,
                      src);
    };

    BlitBG({0, 0, 504, 392});     // From keyboard to spectrum analyzer
    BlitBG({504, 0, 136, 59});    // Down to PASSED TIME
    BlitBG({504, 108, 136, 284}); // Everything below MIDI DEVICE

    BlitLegend({0, 0, 176, 40});    // Left side
    BlitLegend({176, 11, 464, 29}); // Right side without device key

    if (playing == BGM_PLAYING::MIDI) {
      BlitBG({504, 83, 136, 25});    // MIDI DEVICE
      BlitLegend({176, 0, 176, 11}); // Device change key

      // GrpDrawSpect(0,480);
      DrawSpectrum(352, 128);
      DrawNotes();
    }

    const auto millis = BGM_PlayTime().count();
    const auto m = ((millis / 1000) / 60);
    const auto s = ((millis / 1000) % 60);
    GrpPut7B(560, 44, std::format("{:02} : {:02}", m, s).c_str());
    // TextOut(hdc,561,40+2,buf,strlen(buf));

    if (Mid_Loaded()) {
      BlitBG({504, 59, 136, 24}); // MIDI TIMER
      GrpPut7B(560, 68,
               std::format("{:07}", Mid_PlayTime.pulse_interpolated).c_str());
      // TextOut(hdc,561,64+2,buf,strlen(buf));
    }

    GrpPut7B(560, 116, std::format("{:3}", BGM_GetTempo()).c_str());
    // TextOut(hdc,561,112+2,buf,strlen(buf));
    // SetTextColor(hdc,RGB(255*5/5,255*2/5,255*1/5));

    if (playing == BGM_PLAYING::MIDI) {
      text.RenderMidDev({(540 + 2), (96 - 3)});
    }
    text.RenderTitle({400, (144 + 2)}, track_id_,
                     localization_.MusicTitle(track_id_), title_marquee_frame_);
    text.RenderComment({(400 - 40), (144 + 30)},
                       localization_.MusicComment(track_id_));
    text.RenderVersion(
        {(200 - 50), 460},
        localization_.Text(i18n::TextIdFromKey("ui.music_room.version")));

    Grp_Flip();
  }
  title_marquee_frame_++;
  return false;
}
