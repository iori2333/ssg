/*                                                                           */
/*   GameMain.cpp   ウィンドウシステム切り替えなどの処理                     */
/*                                                                           */
/*                                                                           */
#include "game_main.h"
#include "bomb_efc.h" // 爆発エフェクト処理
#include "config.h"
#include "demo_manager.h"
#include "demo_play.h"
#include "font_uty.h"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "geometry.h"
#include "gian.h"
#include "lens.h"
#include "level.h"
#include "music.h"
#include "platform/text_backend.h"
#include "platform/time.h"
#include "score.h"
#include "window_ctrl.h" // ウィンドウ定義
#include "window_sys.h"
#include <algorithm>
#include <chrono>
#include <format>

constexpr WINDOW_POINT MAIN_WINDOW_TOPLEFT = {400, 250};

namespace Version {
struct LINE {
  TEXTRENDER_RECT_ID trr;
  WINDOW_COORD left;
};

LINE Line[1];

constexpr std::string_view BUILD_LABEL = "BUILD";
constexpr Narrow::string_view BUILD_VALUE = (" " VERSION_TAG);

void Init() {
  const auto build_w = TextObj.TextExtent(FONT_ID::TINY, BUILD_VALUE).w;
  Line[0].trr = TextObj.Register({.w = 136, .h = 10});
  Line[0].left = (GRP_RES.w - build_w);
}

void Render(PIXEL_COORD top) {
  // Matches the font rendered by GrpPutScore().
  const auto gradient_func = [](PIXEL_COORD y) -> uint8_t {
    return ((y <= 3) ? 254 : (y <= 6) ? 220 : 180);
  };

  constexpr auto BUILD_LABEL_EXTENT = GrpExtent5(BUILD_LABEL);
  const WINDOW_POINT build_label_topleft = {
      {.x = (Line[0].left - BUILD_LABEL_EXTENT.w),

       // MS Gothic 10 is actually 7 pixels high and starts at a Y coordinate
       // of 2.
       .y = (top + 2 + 7 - BUILD_LABEL_EXTENT.h)}};

  GrpPut55(build_label_topleft, BUILD_LABEL);
  TextObj.Render({Line[0].left, top}, Line[0].trr, BUILD_VALUE,
                 [gradient_func](auto &s) {
                   const std::span strs = {&BUILD_VALUE, 1};
                   DrawGrdFont(s, strs, FONT_ID::TINY, false, gradient_func);
                 });
}
}; // namespace Version

// GameFlow.demo_timer, draw_count, weapon_key_wait, GameFlow.game_over_timer,
// current_name, current_rank, current_dif, viv_temp, GameState.is_demoplay,
// input_locked → gameflow_manager.cpp の GameFlowManager に移動

// 変換済み関数 → inline wrapper は gameflow_manager.h で提供
// (TitleProc, WeaponSelectProc, GameOverProc0, NameRegistProc, ScoreNameProc,
// ScoreDraw)

void GameProc(bool & /*unused*/);
void GameOverProc(bool & /*unused*/); // ゲームオーバー
void PauseProc(bool & /*unused*/);
void DemoProc(bool & /*unused*/); // デモプレイ

void ReplayProcAll(bool & /*unused*/);
void GameOverSaveProc(bool & /*unused*/);

// 西方Ｐｒｏｊｅｃｔ初期化部
void SProjectProc(bool & /*unused*/); // 西方Ｐｒｏｊｅｃｔ表示動作部

void GameSTD_Init(); // ゲームを立ち上げる際に必ず行う初期化関数群
bool DemoInit();     // デモプレイの初期化を行う

void GameDraw();
void GameMove();

// game_main 初期値 → gameflow_manager.cpp で設定

uint8_t CurrentLevel() {
  return ((GameState.game_stage == GRAPH_ID_EXSTAGE) ? GAME_EXTRA
                                                     : GameState.game_level);
}

// input_locked → gameflow_manager.cpp の GameFlowManager に移動

// スコアネーム表示の準備を行う //
bool ScoreNameInit() {
  GameFlow.current_dif = CurrentLevel();

  GameFlow.current_rank = Scores.SetScoreString(nullptr, GameFlow.current_dif);
  if (GameFlow.current_rank == 0) {
    return GameExit();
  }

  MWinForceClose();
  GrpBackend_Clear();
  Grp_Flip();

  if (!LoadGraph(GRAPH_ID_NAMEREGIST)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);

  GameFlow.input_locked = (Key_Data != 0U);
  GameFlow.game_main = [](bool &q) { GameFlow.ScoreNameProc(q); };
  GameFlow.current_state = GameState::ScoreName;

  return true;
}

// スコアネームの表示 //
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
    Snd_SEPlay(SOUND_ID_CANCEL);
    GameExit(false);
    return;

  case KEY_UP:
  case KEY_LEFT:
    if (Scores.score_strings[4].bMoveEnable) {
      break;
    }
    Snd_SEPlay(SOUND_ID_SELECT);
    current_dif = (current_dif + 4) % 5;
    current_rank = Scores.SetScoreString(nullptr, current_dif);
    break;

  case KEY_DOWN:
  case KEY_RIGHT:
    if (Scores.score_strings[4].bMoveEnable) {
      break;
    }
    Snd_SEPlay(SOUND_ID_SELECT);
    current_dif = (current_dif + 1) % 5;
    current_rank = Scores.SetScoreString(nullptr, current_dif);
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

// スコアの描画 //
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
    GrpPut16c2(gx, gy, Scores.score_strings[i].Score);

    gx = (Scores.score_strings[i].x >> 6) + 120;
    gy = (Scores.score_strings[i].y >> 6) + 25;
    GrpPutScore(gx, gy, Scores.score_strings[i].Evade);

    // いや、時間が無いのは分かるんだけどさぁ... //
    gx = (Scores.score_strings[i].x >> 6) + 224;
    gy = (Scores.score_strings[i].y >> 6) + 25;
    if (Scores.score_strings[i].Stage[0] == '7') {
      src = {288, 88, (288 + 16), (88 + 8)};
      GrpSurface_Blit({gx, (gy - 1)}, SURFACE_ID::SYSTEM, src);
    } else {
      {
        GrpPutScore(gx, gy, Scores.score_strings[i].Stage);
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

// 座標から選択文字を取得する //
char GameFlowManager::GetAddr2Char(int x, int y) {
  // 大文字 //
  if (y == 0) {
    return ('A' + (x % 26));
  }
  // 小文字 //
  if (y == 1) {
    return ('a' + (x % 26));
  }
  // その他記号など //

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

// お名前入力 //
void GameFlowManager::NameRegistProc(bool & /*unused*/) {
  // <- DemoInit() を修正するのだぞ
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
      Snd_SEPlay(SOUND_ID_SELECT);
      break;

    case KEY_DOWN:
      y = (y + 1) % 3;
      Snd_SEPlay(SOUND_ID_SELECT);
      break;

    case KEY_LEFT:
      if (y == 2 && x > 20) {
        x = (x - 2) % 26;
      } else {
        x = (x + 25) % 26;
      }
      Snd_SEPlay(SOUND_ID_SELECT);
      break;

    case KEY_RIGHT:
      if (y == 2 && x >= 20) {
        x = (x + 2) % 26;
      } else {
        x = (x + 1) % 26;
      }
      Snd_SEPlay(SOUND_ID_SELECT);
      break;

    case KEY_BOMB:
      Snd_SEPlay(SOUND_ID_CANCEL);
      goto BACK_NR_PROC;

    case KEY_TAMA:
    case KEY_RETURN:
      if (input_locked) {
        break;
      }
      Snd_SEPlay(SOUND_ID_SELECT);

      // 最後の文字まで来ていた場合 //
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

      // それ以外の場合 //
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

    // １文字前に戻る //
    BACK_NR_PROC:
      len = strlen(Scores.score_strings[current_rank - 1].Name);
      if (len != 0) {
        Scores.score_strings[current_rank - 1].Name[len - 1] = '\0';
      }
      break;

    // ネームレジスト終了処理 //
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

  // 名前入力用文字列群 //
  src = {0, 432, 416, 480};
  GrpSurface_Blit({112, 420}, sid, src);

  // カーソル //
  if ((x >= 20) && (y == 2)) {
    src = PIXEL_LTWH{432, (432 + ((count >> 3) << 4)), 32, 16};
  } else {
    src = PIXEL_LTWH{416, (432 + ((count >> 3) << 4)), 16, 16};
  }
  GrpSurface_Blit({(112 + (x << 4)), (420 + (y << 4))}, sid, src);

  // sprintf(buf,"(%2d,%2d)", x, y);
  // GrpPut16(0,0,buf);

  /*
          GrpPut16(400,100,temps);
          for(i=0; i<5; i++){
                  GrpPut16(100, 100+i*32, Scores.score_strings[i].Score);
                  if(current_rank == i+1) GrpPut16(85, 100+i*32, "!!");
          }
  */
  Grp_Flip();
}

// お名前入力の初期化 //
bool GameFlowManager::NameRegistInit(bool bNeedChgMusic) {
  for (auto &it : current_name.Name) {
    it = '\0';
  }
  current_name.Score = Players.viv.score;
  current_name.Evade = Players.viv.evade_sum;
  current_name.Weapon = Players.viv.weapon;
  if (GameState.game_stage == GRAPH_ID_EXSTAGE) {
    current_name.Stage = 1;
  } else {
    current_name.Stage = GameState.game_stage;
  }

  // デバッグ用... //
  Snd_SEStop(8); // ワーニング音を止める
  Snd_SEStopAll();

  // ハイスコアで無いならばタイトルに移行する //
  current_rank = Scores.SetScoreString(&current_name, CurrentLevel());
  if (current_rank == 0) {
    return GameExit();
  }

  MWinForceClose();
  GrpBackend_Clear();
  Grp_Flip();

  if (!LoadGraph(GRAPH_ID_NAMEREGIST)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
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

// ゲームを立ち上げる際に必ず行う初期化関数群 //
void GameSTD_Init() {
  Scroller.key_wait_count = 0;
  MWinForceClose();
  // GrpBackend_Clear();
  // Grp_Flip();

  Bosses.Init();

  // MaidSet();
  Players.SetMaidShotIndices();
  Enemies.InitIndices();
  Bullets.SetIndices(400 + 200); // 小型弾に４００
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

  GameState.game_level = (ExStg ? EXTRA_LEVEL : ConfigDat.GameLevel.v);

  GameSTD_Init();
  Ranking.Reset();

  MaidSet();

  GrpBackend_SetClip(GRP_RES_RECT);

  weapon_key_wait = 1;
  Players.viv.weapon = 0;
  game_main = [](bool &q) { GameFlow.WeaponSelectProc(q); };
  current_state = GameState::WeaponSelect;
  if (ExStg) {
    GameState.game_stage = GRAPH_ID_EXSTAGE;
  }

  viv_temp = Players.viv;

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
    // ウィンドウの表示位置を設定する //
    // Replays don't show dialog, so this is the only place where we need
    // to do this.
    const auto flags = MsgWindowFlags::WITH_FACE;
    if ((ConfigDat.GraphFlags.v & GRPF_WINDOW_UPPER) != 0) {
      MWinInit({128, 16, (640 - 128), 96}, flags);
    } else if ((ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE) == 0) {
      MWinInit({128, 400, (640 - 128), 480}, flags);
    }

    if (GameFlow.current_state == GameState::Game) {
      InitExitWindow();
      InitContinueWindow();
    }
  }
  GrpBackend_SetClip(PLAYFIELD_CLIP);
  GameFlow.game_main = std::move(next_proc);
  return true;
}

// 次のステージに移行する //
bool GameNextStage() {
#ifdef PBG_DEBUG
  Demos.SaveDemo();
#endif

  GameState.game_stage++;

  // エンディングに移行する //
  GameState.game_stage =
      std::min<int>(GameState.game_stage, STAGE_MAX); // 後で変更のこと

  GameSTD_Init();
  MaidNextStage();

  if (!LoadGraph(GameState.game_stage)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
    return false;
  }
  if (!LoadStageData(GameState.game_stage)) {
    DebugOut(u8"ENEMY.DAT が破壊されています");
    return false;
  }

  return true;
}

// マルチステージ・リプレイ用の初期化を行う //
bool GameReplayInitAll(const char8_t *fn) {
  MaidSet();

  if (!Demos.LoadReplayAll(fn)) {
    return false;
  }

  GameState.game_stage = Demos.multi_play_info.Stages[0];

  Ranking.Reset();

  GrpBackend_Clear();
  Grp_Flip();
  GameSTD_Init();

  if (!LoadGraph(GameState.game_stage)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
    Demos.Cleanup();
    Demos.load_all_enable = false;
    return false;
  }
  if (!LoadStageData(GameState.game_stage)) {
    DebugOut(u8"ENEMY.DAT が破壊されています");
    Demos.Cleanup();
    Demos.load_all_enable = false;
    return false;
  }

  if (GameState.game_stage == GRAPH_ID_EXSTAGE) {
    Players.viv.credit = 0;
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

// デモプレイの初期化を行う //
bool DemoInit() {
  GrpBackend_Clear();
  Grp_Flip();

  GameSTD_Init();

  MaidSet();

  rnd_seed_set(Time_SteadyTicksMS());
  GameState.game_stage = (rnd() % STAGE_MAX) + 1;

  if (!Demos.LoadDemo(GameState.game_stage)) {
    // DebugOut(u8"デモプレイデータが存在せず");
    return false;
  }

  Ranking.Reset();

  if (!LoadGraph(GameState.game_stage)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
    return false;
  }
  if (!LoadStageData(GameState.game_stage)) {
    DebugOut(u8"ENEMY.DAT が破壊されています");
    return false;
  }

  GameFlow.current_state = GameState::Demo;
  return GameInit([](bool &q) { DemoProc(q); });
}

// 西方Ｐｒｏｊｅｃｔ表示動作部 //
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
    } else {
      GrpBackend_PaletteSet(SProjectPalette.Fade(palette_tone));
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

// 西方Ｐｒｏｊｅｃｔ表示の初期化 //
bool SProjectInit() {
  GrpBackend_PixelAccessStart();

  if (!LoadGraph(GRAPH_ID_SPROJECT)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
    return false;
  }

  // レンズをすでに作成しているのなら、破棄する //
  Lens = GrpCreateLensBall(70, 36);
  if (!Lens) {
    return false;
  }

  GameFlow.game_main = SProjectProc;
  GameFlow.current_state = GameState::SProject;

  return true;
}

// ゲームを再開する(ESC 抜けから) //
void GameRestart() {
  GameFlow.game_main = GameProc;
  GameFlow.current_state = GameState::Game;
}

// ゲームから抜ける //
bool GameExit(bool bNeedChgMusic) {
  GrpBackend_PixelAccessEnd();
  TextObj.Clear();
  GrpBackend_Clear();
  Grp_Flip();

  if (!LoadGraph(GRAPH_ID_TITLE)) {
    DebugOut(u8"GRAPH.DAT が破壊されています");
    return false;
  }
  GrpBackend_SetClip(GRP_RES_RECT);

  Lasers.SetupLong(); // 音を止める
  Snd_SEStop(8);      // ワーニング音を止めるのだ

  const auto flags = MsgWindowFlags::CENTER;
  MWinForceClose();
  MWinInit({(128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480}, flags);
  MWinOpen();
  // MWinFace(0);

  GameFlow.demo_timer = 0;

  GameState.game_stage = 0;

  if (GameFlow.current_state != GameState::Demo) {
    if (bNeedChgMusic) {
      BGM_Switch(0);
    }
  }

  // Must come after the BGM switch to correctly initialize the sound
  // configuration menu.
  InitMainWindow();
  MainWindow.Open(MAIN_WINDOW_TOPLEFT, 0);
  // MainWindow.Open({ 150, 200 }, 0);
  // MainWindow.Open({ 250, 150 }, 0);

  Version::Init();

  GameFlow.game_main = [](bool &q) { GameFlow.TitleProc(q); };
  GameFlow.current_state = GameState::Title;

  return true;
}

// ゲームオーバーの前処理
void GameOverInit() {
  Effects.SpawnGameOverEffect();

  GameFlow.game_over_timer = 120;

  GameFlow.game_main = [](bool &q) { GameFlow.GameOverProc0(q); };
  GameFlow.current_state = GameState::GameOver0;
}

// コンティニューを行う場合
void GameContinue() {
  Players.viv.evade_sum = 0;
  Players.viv.left = ConfigDat.PlayerStock.v;
  Players.viv.score = ((Players.viv.score % 10) + 1);

  GameFlow.game_main = GameProc;
  GameFlow.current_state = GameState::Game;

  // ここに入らなかったらバグなのだが... //
  if (Players.viv.credit != 0U) {
    // クレジットの残っている場合(コンティニュー Y/N 処理へ) //
    Players.viv.credit -= 1;
  }
}

void GameProc(bool & /*unused*/) {
  // Record current input (always-on multi-stage or legacy single-stage)
  const auto replay_over = Demos.Record(Key_Data);

#ifdef PBG_DEBUG
  if (DebugDat.DemoSave && replay_over) {
    Demos.SaveDemo();
  }
#endif

  if ((Key_Data & KEY_ESC) != 0) {
    // Show exit dialog
    ExitWindow.Open({250, 150}, 1);
    GameFlow.game_main = PauseProc;
    GameFlow.current_state = GameState::Pause;
    return;
  }
  /*
          static BYTE count;
          if(count) count--;
          if((Key_Data & KEY_TAMA) && count==0){
                  CEffectSet(Players.viv.x,Players.viv.y,CEFC_CIRCLE2);//STAR);
                  count = 30;
          }
          if((Key_Data & KEY_BOMB) && count==0){
                  CEffectSet(Players.viv.x,Players.viv.y,CEFC_CIRCLE1);//STAR);
                  count = 30;
          }
  */
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

// ゲームオーバー出現用
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
      GameOverSaveWindow.Open({250, 200}, 0);
      game_main = GameOverSaveProc;
      current_state = GameState::GameOverSave;
      return;
    }

    if (Players.viv.credit == 0) {
      NameRegistInit(true);
      // GameExit();
      return; // 仮
    }

    ContinueWindow.Open({250, 200}, 0);
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
  CWinMove(&GameOverSaveWindow);
  if (GameFlow.current_state != GameState::GameOverSave) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    CWinDraw(&GameOverSaveWindow);
    Grp_Flip();
  }
}

// ゲームオーバー
void GameOverProc(bool & /*unused*/) {
  CWinMove(&ContinueWindow);
  if (GameFlow.current_state != GameState::GameOver) {
    Effects.InitStringEffects();
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    CWinDraw(&ContinueWindow);
    /*
    if(DemoplaySaveEnable){
            constexpr PIXEL_LTRB rc = PIXEL_LTWH{ 288, 80, 24, 8 };
            GrpSurface_Blit({ 128, 470 }, SURFACE_ID::SYSTEM, rc);
    }*/
    Grp_Flip();
  }
}

// デモプレイ
void DemoProc(bool & /*unused*/) {
  static uint8_t ExTimer = 0;

  ExTimer = (ExTimer + 1) % 128;

  if (Key_Data != 0U) {
    Key_Data = KEY_ESC;
  } else {
    Key_Data = Demos.Move();
  }

  GameState.is_demoplay = true;

  // ＥＳＣが押されたら即、終了 //
  if ((Key_Data & KEY_ESC) != 0) {
    Demos.Cleanup();
    GameState.is_demoplay = false;
    GameExit();
    return;
  }

  GameMove();

  if (GameFlow.current_state != GameState::Demo) {
    Demos.Cleanup(); // 後始末
    GameState.is_demoplay = false;
    GameExit(); // 強制終了させる(ゲームオーバー対策)
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

// 装備選択 //
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
    // if(spd<0) Players.viv.weapon = (Players.viv.weapon+3)%4;
    // else      Players.viv.weapon = (Players.viv.weapon+1)%4;
    if (spd < 0) {
      Players.viv.weapon = (Players.viv.weapon + 2) % 3;
    } else {
      Players.viv.weapon = (Players.viv.weapon + 1) % 3;
    }
    spd = 0;
    deg = 0;
    Snd_SEPlay(SOUND_ID_BUZZ);
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

  switch (Key_Data) {
  case KEY_RIGHT:
    if (spd < 0) {
      // Players.viv.weapon = (Players.viv.weapon+3)%4;
      // deg+=64;
      Players.viv.weapon = (Players.viv.weapon + 2) % 3;
      deg += 85;
    }
    spd = 3;
    break;

  case KEY_LEFT:
    if (spd > 0) {
      // Players.viv.weapon = (Players.viv.weapon+1)%4;
      // deg-=64;
      Players.viv.weapon = (Players.viv.weapon + 1) % 3;
      deg -= 85;
    }
    spd = -3;
    break;

  case KEY_TAMA:
  case KEY_RETURN:
    if (spd != 0) {
      break;
    }
    if (GameState.game_stage == GRAPH_ID_EXSTAGE) {
      if (((1 << Players.viv.weapon) & ConfigDat.ExtraStgFlags.v) == 0) {
        break;
      }
    }

    viv_temp.weapon = Players.viv.weapon;
    Players.viv = viv_temp;
    Players.SetMaidShotIndices();
    count = 0;

    Snd_SEPlay(SOUND_ID_SELECT);
    if (GameState.game_stage != GRAPH_ID_EXSTAGE) {
#ifdef PBG_DEBUG
      if (forceStage)
        GameState.game_stage = forceStage;
      else
        GameState.game_stage = DebugDat.StgSelect;
      if (GameState.game_stage == 2)
        Players.viv.exp = 160;
      if (GameState.game_stage >= 3)
        Players.viv.exp = 255;
#else
      if (forceStage != 0) {
        GameState.game_stage = forceStage;
        if (GameState.game_stage == 2) {
          Players.viv.exp = 160;
        }
        if (GameState.game_stage >= 3) {
          Players.viv.exp = 255;
        }
      } else if (ConfigDat.StageSelect.v != 0U) {
        GameState.game_stage = ConfigDat.StageSelect.v;
        if (GameState.game_stage == 2) {
          Players.viv.exp = 160;
        }
        if (GameState.game_stage >= 3) {
          Players.viv.exp = 255;
        }
      } else {
        GameState.game_stage = 1;
      }
#endif
    } else {
      Players.viv.credit = 0;
      Players.viv.left = EXTRA_LIVES;
      Players.viv.exp = 255;
    }

    Demos.Init();

    if (!LoadGraph(GameState.game_stage)) {
      DebugOut(u8"GRAPH.DAT が破壊されています");
      return;
    }
    if (!LoadStageData(GameState.game_stage)) {
      DebugOut(u8"ENEMY.DAT が破壊されています");
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
    Snd_SEPlay(SOUND_ID_CANCEL);
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
      // d = (-i+Players.viv.weapon)*64 + deg - 64;
      d = ((-i + Players.viv.weapon) * 85) + deg - 64;
      x = 120 + cosl(d, 90) - (56 / 2);
      y = 260 + sinl(d, 110) - (48 / 2);
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src[i]);
    }

    GrpGeom->Lock();
    GrpGeom->SetColor({0, 0, 1});
    GrpGeom->SetAlphaNorm(128);
    for (i = 0; i < 3; i++) {
      if ((GameState.game_stage != GRAPH_ID_EXSTAGE) ||
          (((1 << i) & ConfigDat.ExtraStgFlags.v) != 0)) {
        continue;
      }

      d = ((-i + Players.viv.weapon) * 85) + deg - 64;
      x = 120 + cosl(d, 90) - (56 / 2);
      y = 260 + sinl(d, 110) - (48 / 2);
      GrpGeom->DrawBoxA(x, y, (x + 56), (y + 48));
    }
    GrpGeom->Unlock();

    Players.viv.exp = std::min(count, 255);
    if (Players.viv.exp < 31) {
      Players.viv.lay_time = Players.viv.lay_grp = 0;
    }

    Enemies.homing_flag = HOMING_DUMMY;
    Key_Data = KEY_TAMA;

    Players.viv.muteki = 0;
    Players.viv.x = (400 * 64) + sinl((count / 3) * 6, 60 * 64);
    Players.viv.y = (350 * 64) + sinl((count / 3) * 4, 30 * 64);

    MaidMove();
    Players.MoveMaidShot();

    GrpBackend_SetClip({(400 - 110), (400 - 300 + 2), (400 + 110), (400 + 10)});
    for (x = 400 - 110 - 2; x < 400 + 110; x += 32) {
      for (y = 400 - 300 + 2 + ((count * 2) % 32) - 32; y < 400 + 10; y += 32) {
        d = Players.viv.weapon << 4;
        rc = PIXEL_LTWH{224, 256, 32, 32};
        // rc = PIXEL_LTWH{ d, (296 - 24), 16, 16 };
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, rc);
      }
    }
    MaidDraw();
    Players.DrawMaidShot();

    rc = PIXEL_LTWH{72, (272 + 16), 56, 8};
    GrpSurface_Blit({468, 400}, SURFACE_ID::SYSTEM, rc);
    GrpPutScore(500, 400,
                std::format("{}",
                            ((Cast::up<uint16_t>(Players.viv.exp) + 1) >> 5))
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

    /*
                    HDC		hdc;
                    char	buf[100];
                    DxObj.Back->GetDC(&hdc);
                    sprintf(buf,"Players.viv.weapon = %d",Players.viv.weapon);
                    TextOut(hdc,0,0,buf,strlen(buf));
                    DxObj.Back->ReleaseDC(hdc);

    #ifdef PBG_DEBUG
                    StdStatusOutput();
    #endif
    */
    Grp_Flip();
  }
}

void GameFlowManager::TitleProc(bool &quit) {
  constexpr PIXEL_LTRB src = {0, 0, 640, 396};
  // PIXEL_LTRB	src = { 0, 0, 350, 403 };
  // PIXEL_LTRB	src = { 0, 0, 195, 256 };
  // PIXEL_LTRB	src = { 0, 0, 275, 256 };

  /*
          // 鳩プロテクト? //
          if(
                  (GetAsyncKeyState(VK_F1) & 0x80000000) &&
                  (GetAsyncKeyState(VK_F10) & 0x8000000)
          ) {
                  quit = true;
          }
  */
  // Running this here to prevent MIDI processing from jumping over a large
  // number of events once the player enters the Music Room.
  BGM_UpdateMIDITables();

  if (Key_Data == 0) {
    demo_timer += 1;
  } else {
    demo_timer = 0;
  }
  if (MainWindow.Depth() != 0) {
    demo_timer = 0;
  }

  if (demo_timer == 60 * 10) { // 60*3
    DemoInit();
    return;
  }

  auto *window_active =
      ((ReplayFilesWindow.Active()) ? &ReplayFilesWindow
       : (BGMPackWindow.Active())   ? &BGMPackWindow
                                    : &MainWindow);
  CWinMove(window_active);
  MWinHelp(window_active);
  MWinMove();

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

  if (!MainWindow.Active()) {
    switch (MainWindow.SelectionAt(0)) {
    case 0:
      WeaponSelectInit(false);
      return;

    default:
      quit = true;
      return;
    }
  }

  // Silly hack for excessively tall submenus...
  MainWindow.AdjustYForTallMenu(MAIN_WINDOW_TOPLEFT.y, 9);

  if (IsDraw()) {
    GrpBackend_Clear();
    GrpSurface_Blit({0, 42}, SURFACE_ID::TITLE, src);
    // GrpSurface_Blit({ (320 - 175), 77 }, SURFACE_ID::TITLE, src);
    MWinDraw();
    CWinDraw(window_active);

    // Placing this here avoids flickering with the Vulkan backend if any
    // of the above windows had to re-render text?!
    Version::Render(438);
#ifdef PBG_DEBUG
    StdStatusOutput();
#endif

    Grp_Flip();
  }
}

void PauseProc(bool & /*unused*/) {
  CWinMove(&ExitWindow);
  if (GameFlow.current_state != GameState::Pause) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();

    GrpBackend_SetClip(GRP_RES_RECT);
    CWinDraw(&ExitWindow);
    GrpBackend_SetClip(PLAYFIELD_CLIP);

    Grp_Flip();
  }
}

/*
inline XAdd(DWORD old,int id)
{
        RndBuf[id] += (random_ref-old);
}
*/

void GameMove() {
  MWinMove();

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

  // この２行の位置を変更しました //
  MaidMove();
  Players.MoveMaidShot();
}

void GameDraw() {
  GrpBackend_Clear();

  Scroller.Draw();
  Effects.DrawCircleEffects();

  Bosses.Draw();

  WideBombDraw(); // 多分、ここで良いと思うが...

  Effects.DrawBombEffects();

  Enemies.Draw();

  Players.DrawMaidShot();

  MaidDraw();

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

  // static uint8_t test = 0;

  // if((Key_Data&KEY_UP  ) && test<64) test++;
  // if((Key_Data&KEY_DOWN) && test!=0 ) test--;
  Effects.DrawWarningEffect();
  // MoveWarning(test++);
  // DrawWarning();

  Effects.DrawStringEffects();
  StateDraw();

  Bosses.DrawHPG();
  Effects.DrawScreenEffect();

  MWinDraw();
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
