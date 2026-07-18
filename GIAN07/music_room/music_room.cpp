///
/// Music - Music room
///

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <cinttypes>
#include <format>

#include "music_room.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "core/gian.h"
#include "data/gfx_manager.h"
#include "data/music_manager.h"
#include "effect/effect.h"
#include "gfx/font_uty.h"
#include "gameflow/game_main.h"
#include "gameflow/gameflow_manager.h"
#include "gfx/text.h"
#include "platform/text_backend.h"
#include "sys/input.h"
#include "util/debug.h"
#include "util/ut_math.h"

// Constants
// ---------

static constexpr RGB ColorHighlight = {.r = 51, .g = 102, .b = 153};
static constexpr RGB ColorDefault = {.r = 153, .g = 204, .b = 255};
// ---------

// State
// -----

struct MUSICROOM_TEXT {
  TEXTRENDER_RECT_ID mid_dev;
  TEXTRENDER_RECT_ID title;
  TEXTRENDER_RECT_ID comment;
  TEXTRENDER_RECT_ID version;
  BYTE_BUFFER_OWNED comment_buf = nullptr;

  void RenderVersion(WINDOW_POINT topleft) const;
  void RenderMidDev(WINDOW_POINT topleft) const;
  void RenderTitle(WINDOW_POINT topleft) const;
  void RenderComment(WINDOW_POINT topleft) const;
};

size_t MidiPlayID = 0;
std::optional<MUSICROOM_TEXT> MusicRoomText;
// -----

void MUSICROOM_TEXT::RenderVersion(WINDOW_POINT topleft) const {
  static constexpr std::string_view VERSION =
      "秋霜玉    Version 1.005     ★デモ対応版＃★";
  TextObj.Render(topleft, version, VERSION, [](TEXTRENDER_SESSION &s) {
    s.SetFont(FONT_ID::SMALL);
    s.SetColor(ColorDefault);
    s.Put({.x = 0, .y = 0}, VERSION);
  });
}

void MUSICROOM_TEXT::RenderMidDev(WINDOW_POINT topleft) const {
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

void MUSICROOM_TEXT::RenderTitle(WINDOW_POINT topleft) const {
  // Some modders might assign the same title to consecutive tracks, but it's
  // not possible to change the track title without switching to a different
  // track first.
  auto num_str = std::format("#{:02}", (MidiPlayID + 1));
  std::string_view num = num_str;

  TextObj.Render(topleft, title, num, [&num](TEXTRENDER_SESSION &s) {
    const auto &title = BGM_Title();

    // GDI would calculate a trailing space as 4 pixels wide, not 8.
    const auto title_left = (s.Extent(num).w + 8);

    s.SetFont(FONT_ID::NORMAL);
    s.Put({.x = 1, .y = 0}, num, ColorHighlight);
    s.Put({.x = (title_left + 1), .y = 0}, title, ColorHighlight);
    s.Put({.x = 0, .y = 0}, num, ColorDefault);
    s.Put({.x = (title_left + 0), .y = 0}, title, ColorDefault);
  });
}

void MUSICROOM_TEXT::RenderComment(WINDOW_POINT topleft) const {
  if (!comment_buf || comment_buf.size() == 0) {
    return;
  }

  const std::string_view comment_str = {
      reinterpret_cast<const char *>(comment_buf.get()), comment_buf.size()};

  TextObj.Render(topleft, comment, comment_str,
                 [&comment_str](TEXTRENDER_SESSION &s) {
                   int y = 0;
                   s.SetFont(FONT_ID::SMALL);
                   s.SetColor(ColorDefault);

                   size_t pos = 0;
                   while (pos < comment_str.size()) {
                     const auto nl = comment_str.find('\n', pos);
                     const auto line = comment_str.substr(pos, nl - pos);
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

bool MusicRoomInit() {
  TextObj.Clear();
  GrpBackend_Clear();
  Grp_Flip();

  if (!gfx.LoadStage(kGfxMusicRoom)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);

  MidiPlayID = 0;

  BGM_SetTempo(0);

  // Still necessary because the note arrays aren't actually processed
  // outside of the Music Room.
  Mid_TableInit();

  // BGM_Stop();
  auto comment_buf = music.LoadRoomComment(0);
  if (!comment_buf) {
    DebugOut("MUSIC.PAK がはかいされています");
    GameExit();
    return false;
  }

  MusicRoomText = MUSICROOM_TEXT{
      .mid_dev = TextObj.Register({.w = 98, .h = 13}),
      .title = TextObj.Register({.w = 240, .h = 16}),
      .comment = TextObj.Register({.w = 272, .h = 192}),
      .version = TextObj.Register({.w = 490, .h = 13}),

      .comment_buf = std::move(comment_buf),
  };

  // if(!LoadMusic(0)) {
  //         DebugOut("MUSIC.DAT has been corrupted");
  //         GameExit();
  //         return false;
  // }
  // BGM_Play();

  GameFlow.game_main = MusicRoomProc;
  GameFlow.current_state = GameState::MusicRoom;

  return true;
}

// Spectrum analyzer drawing
void GrpDrawSpect(int x, int y) {
  uint16_t ftable[128 + 8 + 8];
  uint16_t ftable2[128];

  static uint16_t ftable3[128 + 8 + 8];
  static uint8_t ftable3flag;

  constexpr PIXEL_LTRB src = {(16 * 16), 0, ((16 * 16) + (8 * 21)),
                              8}; // ,,,8*4

  ftable3flag = ((ftable3flag + 1) % 5);

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

  for (int i = 0; i < std::size(ftable); i++) {
    ftable[i] =
        ((i >= 8 && i <= 128 + 7) ? ftable2[i - 8] : 0) * (256 - sinm(0)) / 256;

    ftable[i] +=
        ((i >= 9 && i <= 128 + 8) ? ftable2[i - 9] : 0) * (256 - sinm(8)) / 256;
    ftable[i] += ((i >= 10 && i <= 128 + 9) ? ftable2[i - 10] : 0) *
                 (256 - sinm(16)) / 256;
    ftable[i] += ((i >= 11 && i <= 128 + 10) ? ftable2[i - 11] : 0) *
                 (256 - sinm(24)) / 256;
    ftable[i] += ((i >= 12 && i <= 128 + 11) ? ftable2[i - 12] : 0) *
                 (256 - sinm(32)) / 256;
    ftable[i] += ((i >= 13 && i <= 128 + 12) ? ftable2[i - 13] : 0) *
                 (256 - sinm(40)) / 256;
    ftable[i] += ((i >= 14 && i <= 128 + 13) ? ftable2[i - 14] : 0) *
                 (256 - sinm(48)) / 256;
    ftable[i] += ((i >= 15 && i <= 128 + 14) ? ftable2[i - 15] : 0) *
                 (256 - sinm(56)) / 256;
    ftable[i] += ((i >= 16 && i <= 128 + 15) ? ftable2[i - 16] : 0) *
                 (256 - sinm(63)) / 256;

    ftable[i] +=
        ((i <= 128 + 6 && i >= 7) ? ftable2[i - 7] : 0) * (256 - sinm(8)) / 256;
    ftable[i] += ((i <= 128 + 5 && i >= 6) ? ftable2[i - 6] : 0) *
                 (256 - sinm(16)) / 256;
    ftable[i] += ((i <= 128 + 4 && i >= 5) ? ftable2[i - 5] : 0) *
                 (256 - sinm(24)) / 256;
    ftable[i] += ((i <= 128 + 3 && i >= 4) ? ftable2[i - 4] : 0) *
                 (256 - sinm(32)) / 256;
    ftable[i] += ((i <= 128 + 2 && i >= 3) ? ftable2[i - 3] : 0) *
                 (256 - sinm(40)) / 256;
    ftable[i] += ((i <= 128 + 1 && i >= 2) ? ftable2[i - 2] : 0) *
                 (256 - sinm(48)) / 256;
    ftable[i] += ((i <= 128 + 0 && i >= 1) ? ftable2[i - 1] : 0) *
                 (256 - sinm(56)) / 256;
    ftable[i] += ((i <= 128 - 1 && i >= 0) ? ftable2[i - 0] : 0) *
                 (256 - sinm(63)) / 256;

    ftable[i] >>= 3;

    if (ftable3[i] < ftable[i]) {
      ftable3[i] = ftable[i];
    } else if ((ftable3flag == 0U) && (ftable3[i] != 0U)) {
      ftable3[i]--;
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
void GrpDrawNote() {
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

void MusicRoomProc(bool & /*unused*/) {
  if (!MusicRoomText) {
    assert(!"Music Room not initialized?");
    std::unreachable();
  }
  auto &text = MusicRoomText.value();

  static decltype(Key_Data) Old_Key;
  static bool DevChgWait;

  const auto playing = BGM_Playing();

  if (Key_Data != Old_Key) {
    if (Input_IsCancel(Key_Data)) {
      DevChgWait = false;
      MusicRoomText = std::nullopt;
      GameExit();
      return;
    }
    if ((Key_Data == KEY_RIGHT) || (Key_Data == KEY_LEFT)) {
      if (Key_Data == KEY_RIGHT) {
        MidiPlayID += 2;
      }
      BGM_Stop();
      MidiPlayID = ((MidiPlayID + music.kTrackCount - 1) % music.kTrackCount);
      BGM_Switch(MidiPlayID);
      text.comment_buf = music.LoadRoomComment(MidiPlayID);
    }
    Old_Key = Key_Data;
  }

  switch (Key_Data) {
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

  if ((SystemKey_Data & SYSKEY_BGM_FADE) != 0) {
    BGM_FadeOut(120);
  }

  BGM_UpdateMIDITables();

  if ((playing == BGM_PLAYING::MIDI) &&
      ((SystemKey_Data & SYSKEY_BGM_DEVICE) != 0)) {
    if (!DevChgWait) {
      BGM_ChangeMIDIDevice(1);
      DevChgWait = true;
    }
  } else {
    // Re-enable if not pressed
    DevChgWait = false;
  }

  if (GameFlow.IsDraw()) {
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
      GrpDrawSpect(352, 128);
      GrpDrawNote();
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
    text.RenderTitle({400, (144 + 2)});
    text.RenderComment({(400 - 40), (144 + 30)});
    text.RenderVersion({(200 - 50), 460});

    Grp_Flip();
  }
}
