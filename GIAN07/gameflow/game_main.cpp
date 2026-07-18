///
/// GameMain - Window system switching and other processing
///
#include <algorithm>
#include <chrono>
#include <format>

#include "audio/snd_backend.h"
#include "demo_manager.h"
#include "demo_play.h"
#include "game_main.h"
#include "gameflow_manager.h"
#include "gameflow/rank_manager.h"
#include "item/item_manager.h"
#include "score.h"
#include "score_manager.h"

#include "audio/bgm.h"
#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "bullet/laser_manager.h"
#include "bullet/bullet_debug.h"
#include "core/config.h"
#include "core/gian.h"
#include "data/gfx_manager.h"
#include "data/stage_manager.h"
#include "core/level.h"
#include "effect/bomb_efc.h"
#include "effect/effect_manager.h"
#include "enemy/boss_manager.h"
#include "enemy/enemy_manager.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "effect/lens.h"
#include "player/player.h"
#include "platform/text_backend.h"
#include "music_room/music_room.h"
#include "stage/scroll_manager.h"
#include "stage/ui_manager.h"
#include "stage/window_sys.h"
#include "sys/input.h"
#include "util/debug.h"
#include "util/time.h"
#include "util/ut_math.h"

constexpr WINDOW_POINT MAIN_WINDOW_TOPLEFT = {400, 250};

namespace Version {
struct LINE {
  TEXTRENDER_RECT_ID trr;
  WINDOW_COORD left;
};

static LINE Line;
constexpr auto BUILD_LABEL = "BUILD";

void Init() {
  const auto build_w = TextObj.TextExtent(FONT_ID::TINY, VERSION_TAG).w;
  Line.trr = TextObj.Register({.w = 136, .h = 10});
  Line.left = (GRP_RES.w - build_w);
}

void Render(PIXEL_COORD top) {
  // Matches the font rendered by GrpPutScore().
  const auto gradient_func = [](PIXEL_COORD y) -> uint8_t {
    return ((y <= 3) ? 254 : (y <= 6) ? 220 : 180);
  };

  constexpr auto BUILD_LABEL_EXTENT = GrpExtent5(BUILD_LABEL);
  const WINDOW_POINT build_label_topleft = {
      {.x = (Line.left - BUILD_LABEL_EXTENT.w),

       // MS Gothic 10 is actually 7 pixels high and starts at a Y coordinate
       // of 2.
       .y = (top + 2 + 7 - BUILD_LABEL_EXTENT.h)}};

  GrpPut55(build_label_topleft, BUILD_LABEL);
  TextObj.Render({Line.left, top}, Line.trr, VERSION_TAG,
                 [gradient_func](auto &s) {
                   std::array<std::string_view, 1> strs = {VERSION_TAG};
                   DrawGrdFont(s, {strs}, FONT_ID::TINY, false, gradient_func);
                 });
}
}; // namespace Version

// GameFlow.demo_timer, draw_count, weapon_key_wait, GameFlow.game_over_timer,
// current_name, current_rank, current_dif, Games.is_demoplay,
// input_locked moved to GameFlowManager in gameflow_manager.cpp

// Converted functions -> inline wrappers provided in gameflow_manager.h
// (TitleProc, WeaponSelectProc, GameOverProc0, NameRegistProc, ScoreNameProc,
// ScoreDraw)

void GameProc(bool & /*unused*/);
void GameOverProc(bool & /*unused*/); // Game over
void PauseProc(bool & /*unused*/);
void DemoProc(bool & /*unused*/); // Demo play

void ReplayProcAll(bool & /*unused*/);
void GameOverSaveProc(bool & /*unused*/);

// West Project initialization section
void SProjectProc(bool & /*unused*/); // West Project display operation section

void GameSTD_Init(); // Initialization functions required when starting the game
bool DemoInit();     // Initialize demo play

void GameDraw();
void GameMove();

// game_main initial values set in gameflow_manager.cpp

GameLevel CurrentLevel() {
  return ((Games.game_stage == kGfxExStage) ? GameLevel::EXTRA
                                                     : Games.game_level);
}

// input_locked moved to GameFlowManager in gameflow_manager.cpp

// Prepare score name display
bool ScoreNameInit() {
  GameFlow.current_dif = std::to_underlying(CurrentLevel());

  GameFlow.current_rank = Scores.SetScoreString(
      nullptr, static_cast<GameLevel>(GameFlow.current_dif));
  if (GameFlow.current_rank == 0) {
    return GameExit();
  }

  UI.MsgForceClose();
  GrpBackend_Clear();
  Grp_Flip();

  if (!gfx.LoadStage(kGfxNameRegist)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);

  GameFlow.input_locked = (Key_Data != 0U);
  GameFlow.game_main = [](bool &q) { GameFlow.ScoreNameProc(q); };
  GameFlow.current_state = GameState::ScoreName;

  return true;
}

// Display score name
void GameFlowManager::ScoreNameProc(bool & /*unused*/) {
  static const char *ExString[5] = {"Easy", "Normal", "Hard", "Lunatic",
                                    "Extra"};

  switch (Key_Data) {
  case KEY_RETURN:
  case KEY_TAMA:
  case KEY_BOMB:
  case KEY_ESC:
    if (input_locked) {
      break;
    }
    Snd_SEPlay(SfxId::Cancel);
    GameExit(false);
    return;

  case KEY_UP:
  case KEY_LEFT:
    if (Scores.score_strings[4].bMoveEnable) {
      break;
    }
    Snd_SEPlay(SfxId::Select);
    current_dif = (current_dif + 4) % 5;
    current_rank =
        Scores.SetScoreString(nullptr, static_cast<GameLevel>(current_dif));
    break;

  case KEY_DOWN:
  case KEY_RIGHT:
    if (Scores.score_strings[4].bMoveEnable) {
      break;
    }
    Snd_SEPlay(SfxId::Select);
    current_dif = (current_dif + 1) % 5;
    current_rank =
        Scores.SetScoreString(nullptr, static_cast<GameLevel>(current_dif));
    break;

  case 0:
    input_locked = false;
    break;
  }

  GrpBackend_Clear();
  ScoreDraw();
  GrpPut16(320, 450, ExString[current_dif]);
  Grp_Flip();
}

// Draw score
void GameFlowManager::ScoreDraw() {
  int i = 0;
  int gx = 0;
  int gy = 0;
  int v = 0;
  PIXEL_LTRB src;

  for (i = 0; i < 5; i++) {
    v = (Scores.score_strings[i].x - ((50 + (i * 24)) << 6)) / 12;
    if (v > 64 * 2) {
      // v = max(v,20*64);
      Scores.score_strings[i].x -= v;
    } else {
      Scores.score_strings[i].bMoveEnable = false;
    }

    src = PIXEL_LTWH{0, (64 + (32 * i)), 400, 32};
    const WINDOW_POINT topleft = {
        (Scores.score_strings[i].x >> 6),
        (Scores.score_strings[i].y >> 6),
    };
    GrpSurface_Blit(topleft, SURFACE_ID::NAMEREG, src);
    // GrpSurface_Blit(
    // 	{ (50 + (i * 24)), (100 + (i * 48) }, SURFACE_ID::NAMEREG, src
    // );

    gx = (Scores.score_strings[i].x >> 6) + 88;
    gy = (Scores.score_strings[i].y >> 6) + 4;
    GrpPut16c2(gx, gy, Scores.score_strings[i].Name);

    gx = (Scores.score_strings[i].x >> 6) + 232 - 16;
    gy = (Scores.score_strings[i].y >> 6) + 4;
    GrpPut16c2(gx, gy, Scores.score_strings[i].Score.c_str());

    gx = (Scores.score_strings[i].x >> 6) + 120;
    gy = (Scores.score_strings[i].y >> 6) + 25;
    GrpPutScore(gx, gy, Scores.score_strings[i].Evade.c_str());

    // I know there's no time, but...
    gx = (Scores.score_strings[i].x >> 6) + 224;
    gy = (Scores.score_strings[i].y >> 6) + 25;
    if (Scores.score_strings[i].Stage[0] == '7') {
      src = {288, 88, (288 + 16), (88 + 8)};
      GrpSurface_Blit({gx, (gy - 1)}, SURFACE_ID::SYSTEM, src);
    } else {
      {
        GrpPutScore(gx, gy, Scores.score_strings[i].Stage.c_str());
      }
    }

    gx = (Scores.score_strings[i].x >> 6) + 224 + 80;
    gy = (Scores.score_strings[i].y >> 6) + 25;
    src = PIXEL_LTWH{0, (400 + (Scores.score_strings[i].Weapon * 8)), 48, 8};
    GrpSurface_Blit({gx, (gy - 1)}, SURFACE_ID::NAMEREG, src);
  }
}

static constexpr auto NR_EXCHAR_BACK = 0;
static constexpr auto NR_EXCHAR_END = -1;
static constexpr auto NR_EXCHAR_ERROR = -2;

// Get selected character from coordinates
char GameFlowManager::GetAddr2Char(int x, int y) {
  // Uppercase
  if (y == 0) {
    return ('A' + (x % 26));
  }
  // Lowercase
  if (y == 1) {
    return ('a' + (x % 26));
  }
  // Other symbols

  switch (x) {
  case 0:
    return '0';
  case 1:
    return '1';
  case 2:
    return '2';
  case 3:
    return '3';
  case 4:
    return '4';
  case 5:
    return '5';
  case 6:
    return '6';
  case 7:
    return '7';
  case 8:
    return '8';
  case 9:
    return '9';
  case 10:
    return '!';
  case 11:
    return '?';
  case 12:
    return '#';
  case 13:
    return '\\';
  case 14:
    return '<';
  case 15:
    return '>';
  case 16:
    return '=';
  case 17:
    return ',';
  case 18:
    return '+';
  case 19:
    return '-';
  case 20:
    return ' '; // SPACE
  // case 21:
  case 22:
    return NR_EXCHAR_BACK;
  // case 23:
  case 24:
    return NR_EXCHAR_END;
  // case 25:
  default:
    return NR_EXCHAR_ERROR;
  }
}

// Name input
void GameFlowManager::NameRegistProc(bool & /*unused*/) {
  // <- Fix DemoInit()
  PIXEL_LTRB src = {0, 0, 400, 64};
  int gx = 0;
  int gy = 0;
  int len = 0;
  static int x;
  static int y;
  static int8_t key_time;
  static uint8_t count;
  static uint8_t time;
  // char	buf[100],
  char c = 0;

  constexpr int8_t END_WAIT = -1;

  if (key_time == 0) {
    key_time = 8; // 16;

    switch (Key_Data) {
    case KEY_UP:
      y = (y + 2) % 3;
      Snd_SEPlay(SfxId::Select);
      break;

    case KEY_DOWN:
      y = (y + 1) % 3;
      Snd_SEPlay(SfxId::Select);
      break;

    case KEY_LEFT:
      if (y == 2 && x > 20) {
        x = (x - 2) % 26;
      } else {
        x = (x + 25) % 26;
      }
      Snd_SEPlay(SfxId::Select);
      break;

    case KEY_RIGHT:
      if (y == 2 && x >= 20) {
        x = (x + 2) % 26;
      } else {
        x = (x + 1) % 26;
      }
      Snd_SEPlay(SfxId::Select);
      break;

    case KEY_BOMB:
      Snd_SEPlay(SfxId::Cancel);
      goto BACK_NR_PROC;

    case KEY_TAMA:
    case KEY_RETURN:
      if (input_locked) {
        break;
      }
      Snd_SEPlay(SfxId::Select);

      // If at the last character
      if (strlen(Scores.score_strings[current_rank - 1].Name) ==
          NR_NAME_LEN - 1) {
        switch (c = GetAddr2Char(x, y)) {
        case NR_EXCHAR_END:
        case NR_EXCHAR_ERROR:
          goto EXIT_NR_PROC;

        case NR_EXCHAR_BACK:
          goto BACK_NR_PROC;

        default:
          x = 24;
          y = 2;
          break;
        }

        break;
      }

      // Otherwise
      switch (c = GetAddr2Char(x, y)) {
      case NR_EXCHAR_END:
      case NR_EXCHAR_ERROR:
        goto EXIT_NR_PROC;

      case NR_EXCHAR_BACK:
        goto BACK_NR_PROC;

      default:
        len = strlen(Scores.score_strings[current_rank - 1].Name);
        Scores.score_strings[current_rank - 1].Name[len] = c;
        Scores.score_strings[current_rank - 1].Name[len + 1] = '\0';
        break;
      }
      break;

    // Go back one character
    BACK_NR_PROC:
      len = strlen(Scores.score_strings[current_rank - 1].Name);
      if (len != 0) {
        Scores.score_strings[current_rank - 1].Name[len - 1] = '\0';
      }
      break;

    // Name registration end processing
    EXIT_NR_PROC:
      if (strlen(Scores.score_strings[current_rank - 1].Name) == 0) {
        std::format_to_n(Scores.score_strings[current_rank - 1].Name,
                         NR_NAME_LEN, "Vivit!");
      }

      Scores.score_strings[current_rank - 1].Name[NR_NAME_LEN - 1] = '\0';

      std::format_to_n(current_name.Name, NR_NAME_LEN, "{}",
                       Scores.score_strings[current_rank - 1].Name);
      Scores.SaveScoreData(&current_name, CurrentLevel());

      key_time = END_WAIT;
      break;

    case 0:
      input_locked = false;
      break;
    }

    if (x > 20 && y == 2) {
      x &= (~1);
    }
  } else if (key_time != END_WAIT) {
    key_time--;
  }

  if (key_time == -1) {
    if (Key_Data == 0) {
      x = y = key_time = 0;
      GameExit();
      return;
    }
  } else {
    if (Key_Data == 0) {
      key_time = 0;
    }
    count = ((count + 1) % 24);
    time++;
  }

  GrpBackend_Clear();

  GrpGeom->Lock();

  GrpGeom->SetColor({2, 0, 0});
  gx = Scores.score_strings[current_rank - 1].x >> 6;
  gy = Scores.score_strings[current_rank - 1].y >> 6;
  GrpGeom->DrawBox(gx, gy, (gx + 400), (gy + 32));

  if (time % 64 > 32) {
    GrpGeom->SetColor({4, 0, 0});
    len = std::min(strlen(Scores.score_strings[current_rank - 1].Name),
                   NR_NAME_LEN - 2);
    gx += ((len * 16) + 88);
    gy += 4;
    GrpGeom->DrawBox(gx, gy, (gx + 14), (gy + 16));
  }

  GrpGeom->Unlock();

  constexpr auto sid = SURFACE_ID::NAMEREG;
  GrpSurface_Blit({120, 0}, sid, src);

  ScoreDraw();

  // Name input string group
  src = {0, 432, 416, 480};
  GrpSurface_Blit({112, 420}, sid, src);

  // Cursor
  if ((x >= 20) && (y == 2)) {
    src = PIXEL_LTWH{432, (432 + ((count >> 3) << 4)), 32, 16};
  } else {
    src = PIXEL_LTWH{416, (432 + ((count >> 3) << 4)), 16, 16};
  }
  GrpSurface_Blit({(112 + (x << 4)), (420 + (y << 4))}, sid, src);

  // sprintf(buf,"(%2d,%2d)", x, y);
  // GrpPut16(0,0,buf);

  //	GrpPut16(400,100,temps);
  //	for(i=0; i<5; i++){
  //		GrpPut16(100, 100+i*32, Scores.score_strings[i].Score);
  //		if(current_rank == i+1) GrpPut16(85, 100+i*32, "!!");
  //	}
  Grp_Flip();
}

// Initialize name input
bool GameFlowManager::NameRegistInit(bool bNeedChgMusic) {
  for (auto &it : current_name.Name) {
    it = '\0';
  }
  current_name.Score = Players.Score();
  current_name.Evade = Players.GrazeSum();
  current_name.Weapon = Players.Weapon();
  if (Games.game_stage == kGfxExStage) {
    current_name.Stage = 1;
  } else {
    current_name.Stage = Games.game_stage;
  }

  // For debugging...
  Snd_SEStop(8); // Stop warning sound
  Snd_SEStopAll();

  // If not a high score, transition to title
  current_rank = Scores.SetScoreString(&current_name, CurrentLevel());
  if (current_rank == 0) {
    return GameExit();
  }

  UI.MsgForceClose();
  GrpBackend_Clear();
  Grp_Flip();

  if (!gfx.LoadStage(kGfxNameRegist)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);

  input_locked = (Key_Data != 0U);
  game_main = [](bool &q) { GameFlow.NameRegistProc(q); };
  current_state = GameState::NameRegist;

  if (bNeedChgMusic) {
    BGM_Switch(19);
  }

  return true;
}

// Initialization functions required when starting the game
void GameSTD_Init() {
  Scroller.key_wait_count = 0;
  UI.MsgForceClose();
  // GrpBackend_Clear();
  // Grp_Flip();

  Bosses.Init();

  // Players.Initialize();
  Players.SetMaidShotIndices();
  Enemies.InitIndices();
  Bullets.SetIndices(400 + 200); // 400 for small bullets
  Lasers.SetIndices();
  Lasers.SetupLong();
  Lasers.InitHoming();
  Effects.InitStringEffects();
  Effects.InitCircleEffects();
  Effects.InitLockOn();
  Items.SetIndices();
  Effects.InitFragments();
  Effects.InitScreenEffect();
  Effects.SetScreenEffect(SCNEFC_CFADEIN);

  Effects.InitBombEffects();

  Effects.InitWarningText();
  Effects.InitWarningEffect();
  // WarningEffectSet();

  BGM_SetTempo(0);

  // draw_count = 0;
}

bool GameFlowManager::WeaponSelectInit(bool ExStg) {
  GrpBackend_Clear();
  Grp_Flip();

  Games.game_level = (ExStg ? GameLevel::HARD : ConfigDat.game_level);

  GameSTD_Init();
  Ranking.Reset();

  Players.Initialize();

  GrpBackend_SetClip(GRP_RES_RECT);

  weapon_key_wait = 1;
  Players.BeginWeaponPreview();
  game_main = [](bool &q) { GameFlow.WeaponSelectProc(q); };
  current_state = GameState::WeaponSelect;
  if (ExStg) {
    Games.game_stage = kGfxExStage;
  }

  return true;
}

bool GameInit(std::function<void(bool &)> next_proc) {
  TextObj.Clear();
  if (GameFlow.current_state != GameState::Demo) {
    BGM_FadeOut(240);
    Effects.InitMusicTitle();
  }
  if (GameFlow.current_state == GameState::Game ||
      GameFlow.current_state == GameState::ReplayAll) {
    // Set window display position
    // Replays don't show dialog, so this is the only place where we need
    // to do this.
    const auto flags = MsgWindowFlags::WITH_FACE;
    if (ConfigDat.window_upper) {
      UI.Msg().Init({128, 16, (640 - 128), 96}, flags);
    } else if (!ConfigDat.msg_disable) {
      UI.Msg().Init({128, 400, (640 - 128), 480}, flags);
    }

    if (GameFlow.current_state == GameState::Game) {
      UI.InitExit();
      UI.InitContinue();
    }
  }

  // Wire up UI callbacks into the gameflow layer
  UI.on_game_exit = [] { GameExit(); };
  UI.on_game_exit_no_save = [] { GameFlow.NameRegistInit(true); };
  UI.on_game_restart = [] { GameRestart(); };
  UI.on_game_continue = [] { GameContinue(); };

  GrpBackend_SetClip(PLAYFIELD_CLIP);
  GameFlow.game_main = std::move(next_proc);
  return true;
}

// Transition to next stage
bool GameNextStage() {
  Games.game_stage++;

  // Transition to ending
  Games.game_stage =
      std::min<int>(Games.game_stage, STAGE_MAX); // To be changed later

  GameSTD_Init();
  Players.PrepareNextStage();

  if (!gfx.LoadStage(Games.game_stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  if (!stage_mgr.LoadStageData(Games.game_stage)) {
    DebugOut("MAP.PAK が破壊されています");
    return false;
  }

  return true;
}

// Initialize for multi-stage replay
bool GameReplayInitAll(const char *fn) {
  Players.Initialize();

  if (!Demos.LoadReplayAll(fn)) {
    return false;
  }

  Games.game_stage = Demos.multi_play_info.Stages[0];

  Ranking.Reset();

  GrpBackend_Clear();
  Grp_Flip();
  GameSTD_Init();

  if (!gfx.LoadStage(Games.game_stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    Demos.Cleanup();
    Demos.load_all_enable = false;
    return false;
  }
  if (!stage_mgr.LoadStageData(Games.game_stage)) {
    DebugOut("MAP.PAK が破壊されています");
    Demos.Cleanup();
    Demos.load_all_enable = false;
    return false;
  }

  if (Games.game_stage == kGfxExStage) {
    Players.SetCredits(0);
  }

  GameFlow.current_state = GameState::ReplayAll;
  return GameInit([](bool &q) { ReplayProcAll(q); });
}

// Multi-stage replay playback
void ReplayProcAll(bool & /*unused*/) {
  static uint8_t ExTimer = 0;

  ExTimer = (ExTimer + 1) % 128;

  const int speed = ((SystemKey_Data & SYSKEY_SKIP) != 0) ? 6 : 1;

  for (int i = 0; i < speed; i++) {
    if (Key_Data != KEY_ESC) {
      Key_Data = Demos.Move();
    }

    if ((Key_Data & KEY_ESC) != 0) {
      Demos.Cleanup();
      Demos.load_all_enable = false;
      GameExit();
      return;
    }

    GameMove();

    if (GameFlow.current_state != GameState::ReplayAll) {
      Demos.Cleanup();
      Demos.load_all_enable = false;
      GameExit();
      return;
    }

    if (!Demos.load_enable) {
      Demos.Cleanup();
      Demos.load_all_enable = false;
      GameExit();
      return;
    }
  }

  if (GameFlow.IsDraw()) {
    GameDraw();

    constexpr PIXEL_LTWH rc = {312, 80, 32, 8};
    GrpSurface_Blit({128, 470}, SURFACE_ID::SYSTEM, rc);
    if (ExTimer < 64 + 32) {
      GrpGeom->Lock();
      GrpGeom->SetAlphaNorm(128);
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->DrawBoxA((128 + 45 - 3), (470 + 4 - 1), (128 + 45 + 72),
                        (470 + 4 + 5));
      GrpGeom->Unlock();
      constexpr PIXEL_LTWH rc = {312, 88, 72, 8};
      GrpSurface_Blit({(128 + 45), (470 + 4)}, SURFACE_ID::SYSTEM, rc);
    }
    Grp_Flip();
  }
}

// Initialize demo play
bool DemoInit() {
  GrpBackend_Clear();
  Grp_Flip();

  GameSTD_Init();

  Players.Initialize();

  rnd_seed_set(Time_SteadyTicksMS());
  Games.game_stage = (rnd() % STAGE_MAX) + 1;

  if (!Demos.LoadDemo(Games.game_stage)) {
    // DebugOut("Demo play data does not exist");
    return false;
  }

  Ranking.Reset();

  if (!gfx.LoadStage(Games.game_stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  if (!stage_mgr.LoadStageData(Games.game_stage)) {
    DebugOut("MAP.PAK が破壊されています");
    return false;
  }

  GameFlow.current_state = GameState::Demo;
  return GameInit([](bool &q) { DemoProc(q); });
}

// West Project display operation section
std::optional<LensInfo> Lens;

void SProjectProc(bool & /*unused*/) {
  static uint16_t timer = 0;

  constexpr PIXEL_SIZE logo_size = {.w = 320, .h = 42};
  constexpr WINDOW_LTRB logo = WINDOW_LTWH{
      (320 - (logo_size.w / 2)), (240 + 40), logo_size.w, logo_size.h};

  const auto fade = [logo](uint8_t black_alpha, uint8_t palette_tone) {
    if (auto *gp = GrpGeom_Poly()) {
      gp->Lock();
      gp->SetAlphaNorm(black_alpha);
      gp->SetColor({0, 0, 0});
      gp->DrawBoxA(logo.left, logo.top, logo.right, logo.bottom);
      gp->Unlock();
    }
  };

  constexpr PIXEL_LTRB rc = {0, 0, logo_size.w, logo_size.h};
  int x = 0;
  int y = 0;

  timer = timer + 1;

  if (timer >= 256) {
    timer = 0;

    Lens = std::nullopt;
    GameExit();
    return;
  }

  if (GameFlow.IsDraw()) {
    GrpBackend_Clear(/* 255 */);

    GrpSurface_Blit({logo.left, logo.top}, SURFACE_ID::SPROJECT, rc);

    if (timer < 64) {
      fade(((255 - timer) * 4), (timer * 4));
    } else if (timer > 192) {
      fade((timer * 4), ((255 - timer) * 4));
    } else if (Lens) {
      const uint8_t d = (timer - 64);
      x = 320 + sinl(d - 64, 240);
      y = 295 + sinl(d * 2, 20);
      Lens.value().Draw({x, y});
    }

    Grp_Flip();
  }
}

// Initialize West Project display
bool SProjectInit() {
  GrpBackend_PixelAccessStart();

  if (!gfx.LoadStage(kGfxSProject)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  // If lens already exists, destroy it
  Lens = GrpCreateLensBall(70, 36);
  if (!Lens) {
    return false;
  }

  GameFlow.game_main = SProjectProc;
  GameFlow.current_state = GameState::SProject;

  return true;
}

// Resume game (from ESC exit)
void GameRestart() {
  BGM_Resume();
  SndBackend_ResumeAll();
  GameFlow.game_main = GameProc;
  GameFlow.current_state = GameState::Game;
}

// Exit game
bool GameExit(bool bNeedChgMusic) {
  SndBackend_ResumeAll();
  GrpBackend_PixelAccessEnd();
  TextObj.Clear();
  GrpBackend_Clear();
  Grp_Flip();

  if (!gfx.LoadStage(kGfxTitle)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  GrpBackend_SetClip(GRP_RES_RECT);

  Lasers.SetupLong(); // Stop sound
  Snd_SEStop(8);      // Stop warning sound

  const auto flags = MsgWindowFlags::CENTER;
  UI.MsgForceClose();
  UI.Msg().Init({(128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480}, flags);
  UI.Msg().Open();
  // MWinFace(0);

  GameFlow.demo_timer = 0;

  Games.game_stage = 0;

  if (GameFlow.current_state != GameState::Demo) {
    if (bNeedChgMusic) {
      BGM_Switch(0);
    }
  }

  // Must come after the BGM switch to correctly initialize the sound
  // configuration menu.
  UI.InitMain();
  UI.Main().Open(MAIN_WINDOW_TOPLEFT, 0);
  // MainWindow.Open({ 150, 200 }, 0);
  // MainWindow.Open({ 250, 150 }, 0);

  Version::Init();

  GameFlow.game_main = [](bool &q) { GameFlow.TitleProc(q); };
  GameFlow.current_state = GameState::Title;

  return true;
}

// Game over preprocessing
void GameOverInit() {
  Effects.SpawnGameOverEffect();

  GameFlow.game_over_timer = 120;

  GameFlow.game_main = [](bool &q) { GameFlow.GameOverProc0(q); };
  GameFlow.current_state = GameState::GameOver0;
}

// When continuing
void GameContinue() {
  BGM_Resume();
  SndBackend_ResumeAll();
  Players.ResetForContinue();

  GameFlow.game_main = GameProc;
  GameFlow.current_state = GameState::Game;

  // If we don't reach here, it's a bug...
  if (Players.Credits() != 0U) {
    // If credits remain (go to continue Y/N processing)
    Players.UseCredit();
  }
}

void GameProc(bool & /*unused*/) {
  // Record current input (always-on multi-stage or legacy single-stage)
  Demos.Record(Key_Data);

  if ((Key_Data & KEY_ESC) != 0) {
    // Show exit dialog
    UI.Exit().Open({250, 150}, 1);
    BGM_Pause();
    SndBackend_PauseAll();
    GameFlow.game_main = PauseProc;
    GameFlow.current_state = GameState::Pause;
    return;
  }
  //	static BYTE count;
  //	if(count) count--;
  //	if((Key_Data & KEY_TAMA) && count==0){
  //		CEffectSet(Players.X(),Players.Y(),CEFC_CIRCLE2);//STAR);
  //		count = 30;
  //	}
  //	if((Key_Data & KEY_BOMB) && count==0){
  //		CEffectSet(Players.X(),Players.Y(),CEFC_CIRCLE1);//STAR);
  //		count = 30;
  //	}
  GameMove();
  if (GameFlow.current_state != GameState::Game) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    if (Demos.save_all_enable) {
      constexpr PIXEL_LTRB rc = PIXEL_LTWH{288, 80, 24, 8};
      GrpSurface_Blit({128, 470}, SURFACE_ID::SYSTEM, rc);
    }
    Grp_Flip();
  }
}

// For game over appearance
void GameFlowManager::GameOverProc0(bool & /*unused*/) {
  switch (game_over_timer) {
  default:
    game_over_timer--;
    Effects.MoveFragments();
    Effects.MoveStringEffects();
    break;

  case 0:
    // Wait for press
    if (Key_Data != 0) {
      game_over_timer--;
    }
    break;

  case -1:
    // Wait for release
    if (Key_Data != 0) {
      break;
    }

    // Multi-stage recording: show Save Replay dialog
    if (Demos.HasRecordedStages()) {
      UI.GameOverSave().Open({250, 200}, 0);
      game_main = GameOverSaveProc;
      current_state = GameState::GameOverSave;
      return;
    }

    if (Players.Credits() == 0) {
      NameRegistInit(true);
      // GameExit();
      return; // Temporary
    }

    UI.Continue().Open({250, 200}, 0);
    game_main = GameOverProc;
    current_state = GameState::GameOver;
    return;
  }

  if (IsDraw()) {
    GameDraw();
    Grp_Flip();
  }
}

// Save Replay dialog for Game Over
void GameOverSaveProc(bool & /*unused*/) {
  UI.GameOverSave().Tick(Key_Data);
  if (GameFlow.current_state != GameState::GameOverSave) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    UI.GameOverSave().Draw();
    Grp_Flip();
  }
}

// Game over
void GameOverProc(bool & /*unused*/) {
  UI.Continue().Tick(Key_Data);
  if (GameFlow.current_state != GameState::GameOver) {
    Effects.InitStringEffects();
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    UI.Continue().Draw();
    //	if(DemoplaySaveEnable){
    //		constexpr PIXEL_LTRB rc = PIXEL_LTWH{ 288, 80, 24, 8 };
    //		GrpSurface_Blit({ 128, 470 }, SURFACE_ID::SYSTEM, rc);
    //	}
    Grp_Flip();
  }
}

// Demo play
void DemoProc(bool & /*unused*/) {
  static uint8_t ExTimer = 0;

  ExTimer = (ExTimer + 1) % 128;

  if (Key_Data != 0U) {
    Key_Data = KEY_ESC;
  } else {
    Key_Data = Demos.Move();
  }

  Games.is_demoplay = true;

  // Exit immediately if ESC is pressed
  if ((Key_Data & KEY_ESC) != 0) {
    Demos.Cleanup();
    Games.is_demoplay = false;
    GameExit();
    return;
  }

  GameMove();

  if (GameFlow.current_state != GameState::Demo) {
    Demos.Cleanup(); // Cleanup
    Games.is_demoplay = false;
    GameExit(); // Force exit (game over countermeasure)
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    if (ExTimer < 64) {
      GrpPut16(200, 200, "D E M O   P L A Y");
    }
    Grp_Flip();
  }
}

// Weapon selection
void GameFlowManager::WeaponSelectProc(bool & /*unused*/) {
  PIXEL_LTRB rc;
  int i = 0;
  int x = 0;
  int y = 0;

  static char deg = 0;
  static char spd = 0;
  static int count = 0;

  constexpr PIXEL_LTRB src[4] = {
      PIXEL_LTWH{0, 344, 56, 48},
      PIXEL_LTWH{0, 392, 56, 48},
      PIXEL_LTWH{56, 344, 56, 48},
      PIXEL_LTWH{56, 392, 56, 48},
  };

  deg += spd;
  if (deg >= 85 || deg <= -85) {
    // if(deg>=64 || deg<=-64){
    // if(spd<0) Players.Weapon() = (Players.Weapon()+3)%4;
    // else      Players.Weapon() = (Players.Weapon()+1)%4;
    if (spd < 0) {
      Players.RotateWeapon(-1);
    } else {
      Players.RotateWeapon(1);
    }
    spd = 0;
    deg = 0;
    Snd_SEPlay(SfxId::Buzz);
  }

  if (weapon_key_wait != 0U) {
    if (Key_Data == 0U) {
      weapon_key_wait = 0;
    } else {
      Key_Data = 0;
    }
  }

  int forceStage = 0;
  if ((Key_Data & KEY_STAGE1) != 0) {
    forceStage = 1;
  } else if ((Key_Data & KEY_STAGE2) != 0) {
    forceStage = 2;
  } else if ((Key_Data & KEY_STAGE3) != 0) {
    forceStage = 3;
  } else if ((Key_Data & KEY_STAGE4) != 0) {
    forceStage = 4;
  } else if ((Key_Data & KEY_STAGE5) != 0) {
    forceStage = 5;
  } else if ((Key_Data & KEY_STAGE6) != 0) {
    forceStage = 6;
  }
  Key_Data &= ~(KEY_STAGE1 | KEY_STAGE2 | KEY_STAGE3 | KEY_STAGE4 | KEY_STAGE5 |
                KEY_STAGE6);

  const auto shift_held = Key_Data & KEY_SHIFT;
  Key_Data &= ~KEY_SHIFT;

  switch (Key_Data) {
  case KEY_RIGHT:
    if (spd < 0) {
      Players.RotateWeapon(-1);
      deg += 85;
    }
    spd = 3;
    break;

  case KEY_LEFT:
    if (spd > 0) {
      Players.RotateWeapon(1);
      deg -= 85;
    }
    spd = -3;
    break;

  case KEY_TAMA:
  case KEY_RETURN:
    if (spd != 0) {
      break;
    }
    if (Games.game_stage == kGfxExStage) {
      if (((1 << Players.Weapon()) & ConfigDat.extra_stg_flags) == 0) {
        break;
      }
    }

    Players.CommitWeaponSelection();
    Players.SetMaidShotIndices();
    count = 0;

    Snd_SEPlay(SfxId::Select);
    if (Games.game_stage != kGfxExStage) {
      if (forceStage != 0) {
        Games.game_stage = forceStage;
        if (Games.game_stage == 2) {
          Players.SetPower(160);
        }
        if (Games.game_stage >= 3) {
          Players.SetPower(255);
        }
      } else if (ConfigDat.stage_select != 0U) {
        Games.game_stage = ConfigDat.stage_select;
        if (Games.game_stage == 2) {
          Players.SetPower(160);
        }
        if (Games.game_stage >= 3) {
          Players.SetPower(255);
        }
      } else {
        Games.game_stage = 1;
      }
    } else {
      Players.SetCredits(0);
      Players.SetLives(2);
      Players.SetPower(255);
    }

    Demos.Init();

    if (!gfx.LoadStage(Games.game_stage)) {
      DebugOut("IMAGES.PAK が破壊されています");
      return;
    }
    if (!stage_mgr.LoadStageData(Games.game_stage)) {
      DebugOut("MAP.PAK が破壊されています");
      return;
    }

    GameFlow.current_state = GameState::Game;
    GameInit([](bool &q) { GameProc(q); });
    return;

  case KEY_ESC:
  case KEY_BOMB:
    if (spd != 0) {
      break;
    }
    Snd_SEPlay(SfxId::Cancel);
    GameExit(false);
    return;
  }

  count = (count + 1) % (256 + 128);

  if (IsDraw()) {
    GrpBackend_Clear();

    rc = {0, (264 - 8), 224, (296 - 24)};
    GrpSurface_Blit({(320 - 112), 20}, SURFACE_ID::SYSTEM, rc);

    rc = PIXEL_LTWH{0, 272, 64, 24};
    GrpSurface_Blit({(120 - 32), (260 - 12)}, SURFACE_ID::SYSTEM, rc);

    uint8_t d = (((count / 8) % 2) << 3);
    rc = PIXEL_LTWH{72, (272 + d), 56, 8};
    GrpSurface_Blit({(400 - 28 + 4), 420}, SURFACE_ID::SYSTEM, rc);

    for (i = 0; i < 3; i++) {
      // for(i=0;i<4;i++){
      // d = (-i+Players.Weapon())*64 + deg - 64;
      d = ((-i + Players.Weapon()) * 85) + deg - 64;
      x = 120 + cosl(d, 90) - (56 / 2);
      y = 260 + sinl(d, 110) - (48 / 2);
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src[i]);
    }

    GrpGeom->Lock();
    GrpGeom->SetColor({0, 0, 1});
    GrpGeom->SetAlphaNorm(128);
    for (i = 0; i < 3; i++) {
      if ((Games.game_stage != kGfxExStage) ||
          (((1 << i) & ConfigDat.extra_stg_flags) != 0)) {
        continue;
      }

      d = ((-i + Players.Weapon()) * 85) + deg - 64;
      x = 120 + cosl(d, 90) - (56 / 2);
      y = 260 + sinl(d, 110) - (48 / 2);
      GrpGeom->DrawBoxA(x, y, (x + 56), (y + 48));
    }
    GrpGeom->Unlock();

    Players.SetPower(static_cast<uint8_t>(std::min(count, 255)));
    if (Players.Power() < 31) {
      Players.ClearLaserState();
    }

    Enemies.homing_flag = HOMING_DUMMY;
    Key_Data = KEY_TAMA | shift_held;

    Players.ClearInvincibility();
    Players.SetPosition((400 * 64) + sinl((count / 3) * 6, 60 * 64),
                        (350 * 64) + sinl((count / 3) * 4, 30 * 64));

    Players.Update();
    Players.MoveMaidShot();

    GrpBackend_SetClip({(400 - 110), (400 - 300 + 2), (400 + 110), (400 + 10)});
    for (x = 400 - 110 - 2; x < 400 + 110; x += 32) {
      for (y = 400 - 300 + 2 + ((count * 2) % 32) - 32; y < 400 + 10; y += 32) {
        d = Players.Weapon() << 4;
        rc = PIXEL_LTWH{224, 256, 32, 32};
        // rc = PIXEL_LTWH{ d, (296 - 24), 16, 16 };
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, rc);
      }
    }
    Players.Draw();
    Players.DrawMaidShot();

    rc = PIXEL_LTWH{72, (272 + 16), 56, 8};
    GrpSurface_Blit({468, 400}, SURFACE_ID::SYSTEM, rc);
    GrpPutScore(
        500, 400,
        std::format("{}", ((Cast::up<uint16_t>(Players.Power()) + 1) >> 5))
            .c_str());

    GrpBackend_SetClip(GRP_RES_RECT);

    GrpGeom->Lock();
    GrpGeom->SetColor({0, 0, 4});
    GrpGeom->DrawLine((400 - 110), (400 - 300), (400 + 110), (400 - 300));
    GrpGeom->DrawLine((400 - 110), (400 + 10), (400 + 110), (400 + 10));
    GrpGeom->DrawLine((400 - 110), (400 - 300), (400 - 110), (400 + 10));
    GrpGeom->DrawLine((400 + 110), (400 - 300), (400 + 110), (400 + 10));

    if (abs(deg) <= 25) {
      GrpGeom->SetColor({2, 2, 5});
      GeomCircle({120, 150}, (39 + 10 - (2 * abs(deg))));
      GrpGeom->SetColor({4, 4, 5});
      GeomCircle({120, 150}, (41 + 10 - (2 * abs(deg))));
    }
    GrpGeom->Unlock();

    //	HDC		hdc;
    //	char	buf[100];
    //	DxObj.Back->GetDC(&hdc);
    //	sprintf(buf,"Players.Weapon() = %d",Players.Weapon());
    //	TextOut(hdc,0,0,buf,strlen(buf));
    //	DxObj.Back->ReleaseDC(hdc);
    //
    Grp_Flip();
  }
}

void GameFlowManager::TitleProc(bool &quit) {
  constexpr PIXEL_LTRB src = {0, 0, 640, 396};
  // PIXEL_LTRB	src = { 0, 0, 350, 403 };
  // PIXEL_LTRB	src = { 0, 0, 195, 256 };
  // PIXEL_LTRB	src = { 0, 0, 275, 256 };

  //	// Pigeon protect?
  //	if(
  //		(GetAsyncKeyState(VK_F1) & 0x80000000) &&
  //		(GetAsyncKeyState(VK_F10) & 0x8000000)
  //	) {
  //		quit = true;
  //	}
  // Running this here to prevent MIDI processing from jumping over a large
  // number of events once the player enters the Music Room.
  BGM_UpdateMIDITables();

  if (Key_Data == 0) {
    demo_timer += 1;
  } else {
    demo_timer = 0;
  }
  if (UI.Main().Depth() != 0) {
    demo_timer = 0;
  }

  if (demo_timer == 60 * 10) { // 60*3
    DemoInit();
    return;
  }

  auto *window_active = UI.ActiveMenu();
  window_active->Tick(Key_Data);
  UI.MsgHelp();
  UI.MsgTick();

  // Start pending replay after menu closes
  if (!Demos.pending_replay_file.empty()) {
    auto fn = Demos.pending_replay_file;
    Demos.pending_replay_file.clear();
    GameReplayInitAll(fn.c_str());
    return;
  }

  if (current_state != GameState::Title) {
    return;
  }

  if (!UI.Main().Active()) {
    switch (UI.Main().SelectionAt(0)) {
    case 0:
      WeaponSelectInit(false);
      return;

    default:
      quit = true;
      return;
    }
  }

  // Silly hack for excessively tall submenus...
  UI.Main().AdjustYForTallMenu(MAIN_WINDOW_TOPLEFT.y, 9);

  if (IsDraw()) {
    GrpBackend_Clear();
    GrpSurface_Blit({0, 42}, SURFACE_ID::TITLE, src);
    // GrpSurface_Blit({ (320 - 175), 77 }, SURFACE_ID::TITLE, src);
    UI.MsgDraw();
    window_active->Draw();

    // Placing this here avoids flickering with the Vulkan backend if any
    // of the above windows had to re-render text?!
    Version::Render(438);

    Grp_Flip();
  }
}

void PauseProc(bool & /*unused*/) {
  UI.Exit().Tick(Key_Data);
  if (GameFlow.current_state != GameState::Pause) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();

    GrpBackend_SetClip(GRP_RES_RECT);
    UI.Exit().Draw();
    GrpBackend_SetClip(PLAYFIELD_CLIP);

    Grp_Flip();
  }
}

// inline XAdd(DWORD old,int id)
//{
//	RndBuf[id] += (random_ref-old);
// }

void GameMove() {
  UI.MsgTick();

  Scroller.Move();

  Bosses.Move();
  Enemies.Move();
  Items.Move();
  Bullets.Move();
  Lasers.Move();
  Lasers.MoveLong();
  Lasers.MoveHoming();
  Effects.MoveFragments();
  Effects.MoveStringEffects();
  Effects.MoveCircleEffects();
  Effects.MoveBombEffects();
  Effects.MoveLockOn();

  Effects.MoveWarningEffect();
  Effects.MoveScreenEffect();

  // Changed position of these two lines
  Players.Update();
  Players.MoveMaidShot();
}

void GameDraw() {
  GrpBackend_Clear();

  Scroller.Draw();
  Effects.DrawCircleEffects();

  Bosses.Draw();

  Players.DrawWideBomb(); // Probably fine here but...

  Effects.DrawBombEffects();

  Enemies.Draw();

  Players.DrawMaidShot();

  Players.Draw();

  if (GrpGeom_FB() != nullptr) {
    Lasers.DrawLong();
  }

  Effects.DrawLockOn();

  Effects.DrawFragments();
  Items.Draw();

  if (GrpGeom_Poly() != nullptr) {
    Lasers.DrawLong();
  }

  Lasers.DrawHoming();
  Lasers.Draw();
  Bullets.Draw();

#ifdef PBG_DEBUG
  if (ConfigDat.hitbox_display != 0) {
    BulletDebug_DrawHitboxes(ConfigDat.hitbox_display);
  }
#endif

  // static uint8_t test = 0;

  // if((Key_Data&KEY_UP  ) && test<64) test++;
  // if((Key_Data&KEY_DOWN) && test!=0 ) test--;
  Effects.DrawWarningEffect();
  // MoveWarning(test++);
  // DrawWarning();

  Effects.DrawStringEffects();
  Players.DrawStatus();

  Bosses.DrawHPG();
  Effects.DrawScreenEffect();

  UI.MsgDraw();
  // GrpBackend_SetClip(PLAYFIELD_CLIP);

  GrpBackend_SetClip(GRP_RES_RECT);
  StdStatusOutput();
  GrpBackend_SetClip({X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)});
}

bool GameFlowManager::IsDraw() {
  if (Grp_FPSDivisor != 0U) {
    draw_count++;
    if ((draw_count % Grp_FPSDivisor) != 0U) {
      return false;
    }
  }

  return true;
}

#ifdef PBG_DEBUG
static void GalleryUpdateAngles() {
  for (uint16_t i = 0; i < Bullets.count_small; i++) {
    auto *t = &Bullets.bullets[Bullets.indices_small[i]];
    if ((t->c & 0xF0) == TAMA_ANGLE) {
      t->d += 4;
    }
  }
  for (uint16_t i = 0; i < Bullets.count_large; i++) {
    auto *t = &Bullets.bullets[Bullets.indices_large[i]];
    const auto cat = t->c & 0xF0;
    if (cat == TAMA_ANGLE || cat == TAMA_EXTRA2) {
      t->d += 4;
    }
  }
}

static void GalleryDrawLabels() {
  static constexpr int x0 = 160;
  static constexpr int y0 = 50;
  static constexpr int dx = 64;
  static constexpr int dy = 80;
  static constexpr uint8_t c_grid[5][6] = {
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
      {0x10, 0x11, 0x12, 0x13, 0x14, 0x15},
      {0x20, 0x21, 0x22, 0x23, 0x24, 0x25},
      {0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF},
      {0x40, 0x41, 0x42, 0x43, 0xFF, 0xFF},
  };
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 6; col++) {
      const auto c = c_grid[row][col];
      if (c == 0xFF) {
        continue;
      }
      const auto label = std::format("{:02X}", c);
      const auto px = x0 + col * dx - 4;
      const auto py = y0 + row * dy + 16;
      GrpPut16(px, py, label.c_str());
    }
  }
}

static void SpawnGalleryBullets() {
  static constexpr uint8_t c_grid[5][6] = {
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
      {0x10, 0x11, 0x12, 0x13, 0x14, 0x15},
      {0x20, 0x21, 0x22, 0x23, 0x24, 0x25},
      {0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF},
      {0x40, 0x41, 0x42, 0x43, 0xFF, 0xFF},
  };
  static constexpr int x0 = 160;
  static constexpr int y0 = 50;
  static constexpr int dx = 64;
  static constexpr int dy = 80;

  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 6; col++) {
      const uint8_t c = c_grid[row][col];
      if (c == 0xFF) {
        continue;
      }
      const int wx = (x0 + col * dx) * 64;
      const int wy = (y0 + row * dy) * 64;

      if ((c & 0xF0) == TAMA_SMALL) {
        const auto idx = Bullets.count_small;
        auto *t = &Bullets.bullets[Bullets.indices_small[idx]];
        Bullets.count_small++;
        t->x = wx;
        t->y = wy;
        t->vx = 0;
        t->vy = 0;
        t->v = 0;
        t->v0 = 0;
        t->c = c;
        t->d = 0;
        t->d16 = 0;
        t->effect = 0;
        t->flag = 0;
        t->type = T_NORM;
        t->rep = 0;
        t->option = 0;
        t->a = 0;
        t->vd = 0;
        t->count = 0;
        t->tx = 0;
        t->ty = 0;
      } else {
        const auto idx = Bullets.count_large;
        auto *t = &Bullets.bullets[Bullets.indices_large[idx]];
        Bullets.count_large++;
        t->x = wx;
        t->y = wy;
        t->vx = 0;
        t->vy = 0;
        t->v = 0;
        t->v0 = 0;
        t->c = c;
        t->d = 0;
        t->d16 = 0;
        t->effect = 0;
        t->flag = 0;
        t->type = T_NORM;
        t->rep = 0;
        t->option = 0;
        t->a = 0;
        t->vd = 0;
        t->count = 0;
        t->tx = 0;
        t->ty = 0;
      }
    }
  }
}

static void BulletGalleryProc(bool & /*quit*/) {
  if ((Key_Data & KEY_ESC) != 0U) {
    ConfigDat.bullet_gallery_active = false;
    Bullets.Clear();
    (void)gfx.LoadStage(kGfxTitle);
    GrpBackend_SetClip(GRP_RES_RECT);
    GameFlow.game_main = [](bool &q) { GameFlow.TitleProc(q); };
    GameFlow.current_state = GameState::Title;
    return;
  }

  GalleryUpdateAngles();

  if (!GameFlow.IsDraw()) {
    return;
  }

  GrpBackend_Clear();
  Bullets.Draw();

  if (ConfigDat.hitbox_display != 0) {
    BulletDebug_DrawHitboxes(ConfigDat.hitbox_display);
  }

  GalleryDrawLabels();

  GrpBackend_SetClip(GRP_RES_RECT);
  GrpPut16(140, 460, "Bullet Gallery  |  ESC to exit");
  GrpBackend_SetClip({X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)});

  Grp_Flip();
}

void BulletGalleryInit() {
  if (!gfx.LoadStage(1)) {
    return;
  }
  (void)gfx.LoadGalleryEnemySurfaces();
  Bullets.SetIndices(400 + 200);
  Bullets.Clear();
  SpawnGalleryBullets();
  ConfigDat.bullet_gallery_active = true;
  GameFlow.game_main = BulletGalleryProc;
  GameFlow.current_state = GameState::BulletGallery;
}
#endif
