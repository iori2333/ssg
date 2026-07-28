///
/// GameMain - Window system switching and other processing
///
#include <algorithm>
#include <chrono>
#include <format>
#include <utility>

#include "game_main.h"
#include "gameflow_manager.h"

#include "audio/bgm.h"
#include "audio/snd.h"
#include "audio/snd_backend.h"
#include "bullet/bullet_manager.h"
#include "effect/effect_manager.h"
#include "effect/lens.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "item/item_system.h"
#include "platform/text_backend.h"
#include "player/player.h"
#include "settings/config.h"
#include "stage/stage_session.h"
#include "sys/input.h"
#include "ui/ui_manager.h"
#include "util/debug.h"
#include "util/time.h"
#include "util/ut_math.h"

constexpr WINDOW_POINT MAIN_WINDOW_TOPLEFT = {400, 250};

constexpr uint8_t PlayerTypeIndex(PlayerType type) {
  return std::to_underlying(type);
}

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

void GameProc(bool & /*unused*/);
void GameOverProc(bool & /*unused*/); // Game over
void PauseProc(bool & /*unused*/);
void DemoProc(bool & /*unused*/); // Demo play

void ReplayProcAll(bool & /*unused*/);

// West Project initialization section
void SProjectProc(bool & /*unused*/); // West Project display operation section

void GameSTD_Init(); // Initialization functions required when starting the game
bool DemoInit();     // Initialize demo play

void GameDraw();
void GameMove();

// Initialization functions required when starting the game
void GameSTD_Init() {
  GameFlow.ctx.ui.ForceCloseMessageWindow();
  // GrpBackend_Clear();
  // Grp_Flip();

  // --- DI ---
  GameFlow.ctx.player.Bind(GameFlow.ctx.session);
  GameFlow.ctx.player.Bind(GameFlow.ctx.stage);
  GameFlow.ctx.player.Configure(GameFlow.ctx.config.game.practice_mode,
                                GameFlow.ctx.config.input.z_spd_down_enabled);

  GameFlow.ctx.enemies.Reset();
  GameFlow.ctx.ui.UpdateBossHud(GameFlow.ctx.enemies.BossHud());
  GameFlow.ctx.bullets.Init();
  GameFlow.ctx.items.Reset();
  GameFlow.ctx.effects.Reset();
  GameFlow.ctx.effects.StartScreenTransition(ScreenTransition::CircleFadeIn);
  // WarningEffectSet();

  BGM_SetTempo(0);

  // draw_count = 0;
}

bool GameFlowManager::WeaponSelectInit(bool ExStg) {
  GrpBackend_Clear();
  Grp_Flip();

  GameFlow.ctx.session.level =
      (ExStg ? GameLevel::Extra : GameFlow.ctx.config.game.game_level);

  GameSTD_Init();
  GameFlow.ctx.session.ResetRank();

  GameFlow.ctx.player.SelectType(PlayerType::Wide);
  GameFlow.ctx.player.Initialize(GameFlow.ctx.config.game.player_stock,
                                 GameFlow.ctx.config.game.bomb_stock);
  GrpBackend_SetClip(GRP_RES_RECT);

  weapon_key_wait = 1;
  game_main = [](bool &q) { GameFlow.WeaponSelectProc(q); };
  current_state = GameState::WeaponSelect;
  if (ExStg) {
    GameFlow.ctx.session.stage = StageId::Extra;
  }

  return true;
}

bool GameInit(std::function<void(bool &)> next_proc) {
  TextObj.Clear();
  if (GameFlow.current_state != GameState::Demo) {
    BGM_FadeOut(240);
    GameFlow.ctx.effects.InitializeTextRenderer();
  }
  if (GameFlow.current_state == GameState::Game ||
      GameFlow.current_state == GameState::ReplayAll) {
    // Set window display position
    // Replays don't show dialog, so this is the only place where we need
    // to do this.
    const auto flags = MsgWindowFlags::WITH_FACE;
    if (GameFlow.ctx.config.graphics.window_upper) {
      GameFlow.ctx.ui.InitMessageWindow({128, 16, (640 - 128), 96}, flags);
    } else if (!GameFlow.ctx.config.graphics.msg_disable) {
      GameFlow.ctx.ui.InitMessageWindow({128, 400, (640 - 128), 480}, flags);
    }

    if (GameFlow.current_state == GameState::Game) {
      GameFlow.ctx.ui.InitExit();
      GameFlow.ctx.ui.InitGameOver();
    }
  }

  // Wire up UI callbacks into the gameflow layer
  GameFlow.ctx.ui.on_game_exit = [] { GameExit(); };
  GameFlow.ctx.ui.on_game_restart = [] { GameRestart(); };
  GameFlow.ctx.ui.on_game_continue = [] { GameContinue(); };
  GameFlow.ctx.ui.on_game_over_exit = [](bool save_replay) {
    GameOverExit(save_replay);
  };

  GrpBackend_SetClip(playfield::kClip);
  GameFlow.game_main = std::move(next_proc);
  return true;
}

// Transition to next stage
bool GameNextStage() {
  // SCL_STAGECLEAR is never issued after Stage 6; SCL_GAMECLEAR handles it.
  GameFlow.ctx.session.AdvanceStage();

  GameSTD_Init();
  GameFlow.ctx.player.PrepareNextStage();
  GameFlow.ctx.records.BeginStage(GameFlow.ctx.player, GameFlow.ctx.session);

  if (!GameFlow.ctx.graphics.LoadStage(GameFlow.ctx.session.stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  if (!GameFlow.ctx.stage_loader.Load(GameFlow.ctx.session.stage,
                                      GameFlow.ctx.enemies,
                                      GameFlow.ctx.stage)) {
    DebugOut("MAP.PAK が破壊されています");
    return false;
  }

  return true;
}

bool GameNextReplayStage() {
  if (!GameFlow.ctx.records.AdvancePlaybackStage()) {
    return false;
  }

  GameSTD_Init();
  GameFlow.ctx.records.RestorePlaybackStage(GameFlow.ctx.player,
                                            GameFlow.ctx.session);
  if (!GameFlow.ctx.graphics.LoadStage(GameFlow.ctx.session.stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                      GameFlow.ctx.session);
    return false;
  }
  if (!GameFlow.ctx.stage_loader.Load(GameFlow.ctx.session.stage,
                                      GameFlow.ctx.enemies,
                                      GameFlow.ctx.stage)) {
    DebugOut("MAP.PAK が破壊されています");
    GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                      GameFlow.ctx.session);
    return false;
  }
  return true;
}

// Initialize a replay from the selected stage checkpoint.
bool GameReplayInit(const char *path, StageId stage) {
  if (!GameFlow.ctx.records.LoadReplay(path, stage) ||
      !GameFlow.ctx.records.ConfigurePlayback(GameFlow.ctx.config,
                                              GameFlow.ctx.session)) {
    return false;
  }

  GrpBackend_Clear();
  Grp_Flip();
  GameSTD_Init();
  GameFlow.ctx.records.RestorePlaybackStage(GameFlow.ctx.player,
                                            GameFlow.ctx.session);

  if (!GameFlow.ctx.graphics.LoadStage(GameFlow.ctx.session.stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                      GameFlow.ctx.session);
    return false;
  }
  if (!GameFlow.ctx.stage_loader.Load(GameFlow.ctx.session.stage,
                                      GameFlow.ctx.enemies,
                                      GameFlow.ctx.stage)) {
    DebugOut("MAP.PAK が破壊されています");
    GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                      GameFlow.ctx.session);
    return false;
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
      Key_Data = GameFlow.ctx.records.NextInput();
    }

    if ((Key_Data & KEY_ESC) != 0) {
      GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                        GameFlow.ctx.session);
      GameExit();
      return;
    }

    GameMove();

    if (GameFlow.current_state != GameState::ReplayAll) {
      GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                        GameFlow.ctx.session);
      GameExit();
      return;
    }

    if (!GameFlow.ctx.records.IsPlaying()) {
      GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                        GameFlow.ctx.session);
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

  GameFlow.ctx.player.Initialize(GameFlow.ctx.config.game.player_stock,
                                 GameFlow.ctx.config.game.bomb_stock);
  rnd_seed_set(Time_SteadyTicksMS());
  GameFlow.ctx.session.stage = static_cast<StageId>(rnd() % kRegularStageCount);

  if (!GameFlow.ctx.records.LoadStageDemo(GameFlow.ctx.session.stage)) {
    // DebugOut("Demo play data does not exist");
    return false;
  }

  GameFlow.ctx.session.ResetRank();

  if (!GameFlow.ctx.graphics.LoadStage(GameFlow.ctx.session.stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  if (!GameFlow.ctx.stage_loader.Load(GameFlow.ctx.session.stage,
                                      GameFlow.ctx.enemies,
                                      GameFlow.ctx.stage)) {
    DebugOut("MAP.PAK が破壊されています");
    return false;
  }
  // ...

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

  if (!GameFlow.ctx.graphics.LoadProjectScreen()) {
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

  if (!GameFlow.ctx.graphics.LoadTitle()) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  GrpBackend_SetClip(GRP_RES_RECT);

  Snd_SEStop(8); // Stop warning sound

  const auto flags = MsgWindowFlags::CENTER;
  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GameFlow.ctx.ui.InitMessageWindow(
      {(128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480}, flags);
  GameFlow.ctx.ui.OpenMessageWindow();
  // MWinFace(0);

  GameFlow.demo_timer = 0;

  GameFlow.ctx.session.stage = StageId::Stage1;

  if (GameFlow.current_state != GameState::Demo) {
    if (bNeedChgMusic) {
      GameFlow.ctx.music.Play(0);
    }
  }

  // Must come after the BGM switch to correctly initialize the sound
  // configuration menu.
  GameFlow.ctx.ui.InitMain();
  GameFlow.ctx.ui.Main().Open(MAIN_WINDOW_TOPLEFT, 0);
  // MainWindow.Open({ 150, 200 }, 0);
  // MainWindow.Open({ 250, 150 }, 0);

  Version::Init();

  GameFlow.game_main = [](bool &q) { GameFlow.TitleProc(q); };
  GameFlow.current_state = GameState::Title;

  return true;
}

// Game over preprocessing
void GameOverInit() {
  GameFlow.ctx.effects.SpawnGameOver();

  GameFlow.game_over_timer = 120;

  GameFlow.game_main = [](bool &q) { GameFlow.GameOverProc0(q); };
  GameFlow.current_state = GameState::GameOver0;
}

// When continuing
void GameContinue() {
  if (GameFlow.ctx.player.Credits() == 0U) {
    return;
  }

  BGM_Resume();
  SndBackend_ResumeAll();
  GameFlow.ctx.records.CancelRecording();
  GameFlow.ctx.player.ResetForContinue(GameFlow.ctx.config.game.player_stock);

  GameFlow.game_main = GameProc;
  GameFlow.current_state = GameState::Game;

  // If we don't reach here, it's a bug...
  if (GameFlow.ctx.player.Credits() != 0U) {
    // If credits remain (go to continue Y/N processing)
    GameFlow.ctx.player.UseCredit();
  }
}

void GameOverExit(bool save_replay) {
  const bool extra_stage = GameFlow.ctx.session.stage == StageId::Extra;
  if (!save_replay) {
    GameFlow.ctx.records.CancelRecording();
  }
  auto score = GameFlow.ctx.records.CaptureScore(GameFlow.ctx.player,
                                                 GameFlow.ctx.session);
  (void)GameFlow.ctx.score.StartNameRegistration(
      std::move(score), true, [save_replay, extra_stage] {
        if (save_replay && GameFlow.ctx.records.HasRecordedStages()) {
          GameFlow.ctx.replay_scene.BeginSave(extra_stage,
                                              [](bool) { (void)GameExit(); });
        } else {
          (void)GameExit();
        }
      });
}

void GameClearResults(bool extra_stage, bool change_music) {
  auto score = GameFlow.ctx.records.CaptureScore(GameFlow.ctx.player,
                                                 GameFlow.ctx.session);
  (void)GameFlow.ctx.score.StartNameRegistration(
      std::move(score), change_music, [extra_stage] {
        if (GameFlow.ctx.records.HasRecordedStages()) {
          GameFlow.ctx.replay_scene.BeginSave(extra_stage,
                                              [](bool) { (void)GameExit(); });
        } else {
          GameFlow.ctx.records.CancelRecording();
          (void)GameExit();
        }
      });
}

void GameProc(bool & /*unused*/) {
  // Record current input (always-on multi-stage or legacy single-stage)
  GameFlow.ctx.records.Record(Key_Data);

  if ((Key_Data & KEY_ESC) != 0) {
    // Show exit dialog
    GameFlow.ctx.ui.Exit().Open({250, 150}, 1);
    BGM_Pause();
    SndBackend_PauseAll();
    GameFlow.game_main = PauseProc;
    GameFlow.current_state = GameState::Pause;
    return;
  }
  //	static BYTE count;
  //	if(count) count--;
  //	if((Key_Data & KEY_TAMA) && count==0){
  //		CEffectSet(GameFlow.ctx.player.X(),GameFlow.ctx.player.Y(),CEFC_CIRCLE2);//STAR);
  //		count = 30;
  //	}
  //	if((Key_Data & KEY_BOMB) && count==0){
  //		CEffectSet(GameFlow.ctx.player.X(),GameFlow.ctx.player.Y(),CEFC_CIRCLE1);//STAR);
  //		count = 30;
  //	}
  GameMove();
  GameFlow.ctx.records.UpdateLastRecordedInput(Key_Data);
  if (GameFlow.current_state != GameState::Game) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    if (GameFlow.ctx.records.IsRecording()) {
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
    GameFlow.ctx.effects.UpdateGameOver();
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

    BGM_Pause();
    SndBackend_PauseAll();
    GameFlow.ctx.ui.GameOver().Open({200, 176}, 0);
    game_main = GameOverProc;
    current_state = GameState::GameOver;
    return;
  }

  if (IsDraw()) {
    GameDraw();
    Grp_Flip();
  }
}

// Game over
void GameOverProc(bool & /*unused*/) {
  GameFlow.ctx.ui.GameOver().Tick(Key_Data);
  if (GameFlow.current_state != GameState::GameOver) {
    GameFlow.ctx.effects.ClearTextEffects();
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();
    GameFlow.ctx.ui.GameOver().Draw();
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
    Key_Data = GameFlow.ctx.records.NextInput();
  }

  GameFlow.ctx.session.is_demoplay = true;

  // Exit immediately if ESC is pressed
  if ((Key_Data & KEY_ESC) != 0) {
    GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                      GameFlow.ctx.session);
    GameFlow.ctx.session.is_demoplay = false;
    GameExit();
    return;
  }

  GameMove();

  if (GameFlow.current_state != GameState::Demo) {
    GameFlow.ctx.records.StopPlayback(GameFlow.ctx.config,
                                      GameFlow.ctx.session); // Cleanup
    GameFlow.ctx.session.is_demoplay = false;
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
    if (spd < 0) {
      GameFlow.ctx.player.RotateType(-1);
    } else {
      GameFlow.ctx.player.RotateType(1);
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
      GameFlow.ctx.player.RotateType(-1);
      deg += 85;
    }
    spd = 3;
    break;

  case KEY_LEFT:
    if (spd > 0) {
      GameFlow.ctx.player.RotateType(1);
      deg -= 85;
    }
    spd = -3;
    break;

  case KEY_TAMA:
  case KEY_RETURN:
    if (spd != 0) {
      break;
    }
    if (GameFlow.ctx.session.stage == StageId::Extra) {
      if (((1 << PlayerTypeIndex(GameFlow.ctx.player.Type()))) == 0) {
        break;
      }
    }

    GameFlow.ctx.player.Initialize(GameFlow.ctx.config.game.player_stock,
                                   GameFlow.ctx.config.game.bomb_stock);
    count = 0;

    Snd_SEPlay(SfxId::Select);
    if (GameFlow.ctx.session.stage != StageId::Extra) {
      if (forceStage != 0) {
        GameFlow.ctx.session.stage = static_cast<StageId>(forceStage - 1);
        if (GameFlow.ctx.session.stage == StageId::Stage2) {
          GameFlow.ctx.player.SetPower(160);
        }
        if (GameFlow.ctx.session.stage >= StageId::Stage3) {
          GameFlow.ctx.player.SetPower(255);
        }
      } else {
        GameFlow.ctx.session.stage = StageId::Stage1;
      }
    } else {
      GameFlow.ctx.player.SetCredits(0);
      GameFlow.ctx.player.SetLives(2);
      GameFlow.ctx.player.SetPower(255);
    }

    GameFlow.ctx.records.BeginRecording(
        GameFlow.ctx.player, GameFlow.ctx.session, GameFlow.ctx.config);

    if (!GameFlow.ctx.graphics.LoadStage(GameFlow.ctx.session.stage)) {
      DebugOut("IMAGES.PAK が破壊されています");
      return;
    }
    if (!GameFlow.ctx.stage_loader.Load(GameFlow.ctx.session.stage,
                                        GameFlow.ctx.enemies,
                                        GameFlow.ctx.stage)) {
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
      d = ((-i + PlayerTypeIndex(GameFlow.ctx.player.Type())) * 85) + deg - 64;
      x = 120 + cosl(d, 90) - (56 / 2);
      y = 260 + sinl(d, 110) - (48 / 2);
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src[i]);
    }

    GrpGeom->Lock();
    GrpGeom->SetColor({0, 0, 1});
    GrpGeom->SetAlphaNorm(128);
    for (i = 0; i < 3; i++) {
      if ((GameFlow.ctx.session.stage != StageId::Extra) ||
          (((1 << i) & GameFlow.ctx.session.extra_stg_flags) != 0)) {
        continue;
      }

      d = ((-i + PlayerTypeIndex(GameFlow.ctx.player.Type())) * 85) + deg - 64;
      x = 120 + cosl(d, 90) - (56 / 2);
      y = 260 + sinl(d, 110) - (48 / 2);
      GrpGeom->DrawBoxA(x, y, (x + 56), (y + 48));
    }
    GrpGeom->Unlock();

    GameFlow.ctx.player.SetPower(static_cast<uint8_t>(std::min(count, 255)));
    if (GameFlow.ctx.player.Power() < 31) {
      GameFlow.ctx.player.ClearContinuousAttack();
    }

    GameFlow.ctx.enemies.ResetHomingTarget();
    Key_Data = KEY_TAMA | shift_held;

    GameFlow.ctx.player.ClearInvincibility();
    GameFlow.ctx.player.SetPosition(400_px + sinl((count / 3) * 6, 60_px),
                                    350_px + sinl((count / 3) * 4, 30_px));

    static_cast<void>(
        GameFlow.ctx.player.Update(GameFlow.ctx.enemies, Key_Data));

    GrpBackend_SetClip({(400 - 110), (400 - 300 + 2), (400 + 110), (400 + 10)});
    for (x = 400 - 110 - 2; x < 400 + 110; x += 32) {
      for (y = 400 - 300 + 2 + ((count * 2) % 32) - 32; y < 400 + 10; y += 32) {
        rc = PIXEL_LTWH{224, 256, 32, 32};
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, rc);
      }
    }
    GameFlow.ctx.player.Draw();
    GameFlow.ctx.player.DrawProjectiles();

    rc = PIXEL_LTWH{72, (272 + 16), 56, 8};
    GrpSurface_Blit({468, 400}, SURFACE_ID::SYSTEM, rc);
    GrpPutScore(
        500, 400,
        std::format(
            "{}", ((Cast::up<uint16_t>(GameFlow.ctx.player.Power()) + 1) >> 5))
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
  if (GameFlow.ctx.ui.Main().Depth() > 1) {
    demo_timer = 0;
  }

  if (demo_timer == 60 * 10) { // 60*3
    DemoInit();
    return;
  }

  auto *window_active = GameFlow.ctx.ui.ActiveMenu();
  window_active->Tick(Key_Data);
  GameFlow.ctx.ui.ShowMenuHelp();
  GameFlow.ctx.ui.TickMessageWindow();

  if (current_state != GameState::Title) {
    return;
  }

  if (!GameFlow.ctx.ui.Main().Active()) {
    switch (GameFlow.ctx.ui.Main().ClosedSelection()) {
    case 0:
      WeaponSelectInit(false);
      return;

    default:
      quit = true;
      return;
    }
  }

  // Silly hack for excessively tall submenus...
  GameFlow.ctx.ui.Main().AdjustYForTallMenu(MAIN_WINDOW_TOPLEFT.y, 9);

  if (IsDraw()) {
    GrpBackend_Clear();
    GrpSurface_Blit({0, 42}, SURFACE_ID::TITLE, src);
    // GrpSurface_Blit({ (320 - 175), 77 }, SURFACE_ID::TITLE, src);
    GameFlow.ctx.ui.DrawMessageWindow();
    window_active->Draw();

    // Placing this here avoids flickering with the Vulkan backend if any
    // of the above windows had to re-render text?!
    Version::Render(438);

    Grp_Flip();
  }
}

void PauseProc(bool & /*unused*/) {
  GameFlow.ctx.ui.Exit().Tick(Key_Data);
  if (GameFlow.current_state != GameState::Pause) {
    return;
  }

  if (GameFlow.IsDraw()) {
    GameDraw();

    GrpBackend_SetClip(GRP_RES_RECT);
    GameFlow.ctx.ui.Exit().Draw();
    GrpBackend_SetClip(playfield::kClip);

    Grp_Flip();
  }
}

// inline XAdd(DWORD old,int id)
//{
//	RndBuf[id] += (random_ref-old);
// }

void HandleStageTransition(stage::StageTransition transition) {
  switch (transition) {
  case stage::StageTransition::None:
    return;

  case stage::StageTransition::NextStage:
    if (GameFlow.ctx.records.IsRecording()) {
      GameFlow.ctx.records.FlushStage();
      (void)GameNextStage();
      return;
    }
    if (GameFlow.ctx.records.IsMultiStagePlayback()) {
      (void)GameNextReplayStage();
      return;
    }
    (void)GameNextStage();
    return;

  case stage::StageTransition::GameClear:
    if (GameFlow.ctx.records.IsMultiStagePlayback()) {
      return;
    }
    if (GameFlow.ctx.session.level != GameLevel::Easy) {
      GameFlow.ctx.session.extra_stg_flags |= static_cast<uint8_t>(
          1U << PlayerTypeIndex(GameFlow.ctx.player.Type()));
    }
    SaveConfigFile(GameFlow.ctx.config);
    if (GameFlow.ctx.records.IsRecording()) {
      GameFlow.ctx.records.FlushStage();
    }
    (void)GameFlow.ctx.ending.Enter();
    return;

  case stage::StageTransition::ExtraClear:
    if (GameFlow.ctx.records.IsMultiStagePlayback()) {
      return;
    }
    if (GameFlow.ctx.records.IsRecording()) {
      GameFlow.ctx.records.FlushStage();
    }
    GameClearResults(true, true);
    return;
  }
}

void GameMove() {
  GameFlow.ctx.ui.TickMessageWindow();

  const auto transition = GameFlow.ctx.stage.Update(
      {.enemies = GameFlow.ctx.enemies,
       .effects = GameFlow.ctx.effects,
       .ui = GameFlow.ctx.ui,
       .graphics = GameFlow.ctx.graphics,
       .music = GameFlow.ctx.music,
       .session = GameFlow.ctx.session,
       .messages_disabled = GameFlow.ctx.config.graphics.msg_disable},
      Key_Data);
  if (transition != stage::StageTransition::None) {
    HandleStageTransition(transition);
    return;
  }

  GameFlow.ctx.enemies.Update();
  GameFlow.ctx.ui.UpdateBossHud(GameFlow.ctx.enemies.BossHud());
  GameFlow.ctx.items.Update();
  GameFlow.ctx.bullets.Update(GameFlow.ctx.enemies.HomingTarget());
  GameFlow.ctx.effects.Update();

  if (GameFlow.ctx.player.Update(GameFlow.ctx.enemies, Key_Data)) {
    GameFlow.ctx.bullets.Clear();
  }
  GameFlow.ctx.session.UpdateRank(GameFlow.ctx.stage.Frame());
}

void GameDraw() {
  const GameplayHudModel hud_model{
      .score = GameFlow.ctx.player.Score(),
      .bombs = GameFlow.ctx.player.Bombs(),
      .lives = GameFlow.ctx.player.Lives(),
      .credits = GameFlow.ctx.player.Credits(),
      .graze_count = GameFlow.ctx.player.GrazeCount(),
      .graze_wait_time = GameFlow.ctx.player.GrazeWaitTime(),
      .miss_count = GameFlow.ctx.player.MissCount(),
      .bomb_used = GameFlow.ctx.player.BombUsed(),
      .deathbomb_count = GameFlow.ctx.player.DeathbombCount(),
      .star_counter = GameFlow.ctx.player.StarCounter(),
      .star_threshold = GameFlow.ctx.player.StarThreshold(),
      .rank = GameFlow.ctx.session.rank,
      .level_name = GameLevelName(GameFlow.ctx.session.level),
      .practice_mode = GameFlow.ctx.config.game.practice_mode,
  };

  GrpBackend_Clear();

  GameFlow.ctx.stage.Draw();
  GameFlow.ctx.effects.DrawCircles();

  GameFlow.ctx.enemies.DrawBosses();

  GameFlow.ctx.player.DrawBombBackground();

  GameFlow.ctx.effects.DrawBombExplosions();

  GameFlow.ctx.enemies.DrawRegular();

  GameFlow.ctx.player.DrawProjectiles();

  GameFlow.ctx.player.Draw();

  GameFlow.ctx.effects.DrawFragments();
  GameFlow.ctx.items.Draw();

  GameFlow.ctx.bullets.Render();

  if (GameFlow.ctx.config.debug.hitbox_display != 0) {
    GameFlow.ctx.bullets.RenderDebugHitboxes(
        GameFlow.ctx.config.debug.hitbox_display);
    GameFlow.ctx.player.DrawDebugHitbox();
  }

  // static uint8_t test = 0;

  // if((Key_Data&KEY_UP  ) && test<64) test++;
  // if((Key_Data&KEY_DOWN) && test!=0 ) test--;
  GameFlow.ctx.effects.DrawForeground();
  GameFlow.ctx.ui.DrawTopHud(hud_model);

  GameFlow.ctx.ui.DrawBossHud(GameFlow.ctx.stage.Frame());
  GameFlow.ctx.effects.DrawScreenTransition();

  GameFlow.ctx.ui.DrawMessageWindow();
  // GrpBackend_SetClip(playfield::kClip);

  GrpBackend_SetClip(GRP_RES_RECT);
  GameFlow.ctx.ui.DrawSidebarHud(hud_model);
  GrpBackend_SetClip({playfield::kLeft, playfield::kTop,
                      (playfield::kRight + 1), (playfield::kBottom + 1)});
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

static void GalleryUpdateAngles() {
  GameFlow.ctx.bullets.RotateDisplayAngles();
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
      const int wx = PixelToWorld(x0 + col * dx);
      const int wy = PixelToWorld(y0 + row * dy);
      GameFlow.ctx.bullets.PlaceDisplayBullet(wx, wy, c);
    }
  }
}

static void BulletGalleryProc(bool & /*quit*/) {
  if ((Key_Data & KEY_ESC) != 0U) {
    GameFlow.ctx.bullets.Clear();
    (void)GameFlow.ctx.graphics.LoadTitle();
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
  GameFlow.ctx.bullets.Render();

  if (GameFlow.ctx.config.debug.hitbox_display != 0) {
    GameFlow.ctx.bullets.RenderDebugHitboxes(
        GameFlow.ctx.config.debug.hitbox_display);
    GameFlow.ctx.player.DrawDebugHitbox();
  }

  GalleryDrawLabels();

  GrpBackend_SetClip(GRP_RES_RECT);
  GrpPut16(140, 460, "Bullet Gallery  |  ESC to exit");
  GrpBackend_SetClip({playfield::kLeft, playfield::kTop,
                      (playfield::kRight + 1), (playfield::kBottom + 1)});

  Grp_Flip();
}

void BulletGalleryInit() {
  if (!GameFlow.ctx.graphics.LoadBulletGallery()) {
    return;
  }
  GameFlow.ctx.bullets.Init();
  GameFlow.ctx.bullets.Clear();
  SpawnGalleryBullets();
  GameFlow.game_main = BulletGalleryProc;
  GameFlow.current_state = GameState::BulletGallery;
}
