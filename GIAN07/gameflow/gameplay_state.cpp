/// Active gameplay flow: live runs, replays, demos, pause, and game over.

#include <cstdint>
#include <string_view>
#include <utility>

#include "flow_types.h"
#include "gameplay_state.h"

#include "app/game_context.h"
#include "audio/bgm.h"
#include "audio/snd_backend.h"
#include "effect/effect_manager.h"
#include "gameplay/game_rules.h"
#include "gameplay/playfield.h"
#include "gfx/font_uty.h"
#include "gfx/graphics_backend.h"
#include "platform/text_backend.h"
#include "player/loadout/player_loadout.h"
#include "settings/config.h"
#include "sys/input.h"
#include "util/debug.h"
#include "util/time.h"
#include "util/ut_math.h"

namespace gameflow {
namespace {

constexpr uint8_t PlayerTypeIndex(PlayerType type) {
  return std::to_underlying(type);
}

} // namespace

void ResetGameplayRuntime(GameContext &context) {
  context.ui.ForceCloseMessageWindow();
  context.player.Configure(context.config.game.practice_mode,
                           context.config.input.z_spd_down_enabled);
  context.enemies.Reset();
  context.ui.UpdateBossHud(context.enemies.BossHud());
  context.bullets.Init();
  context.items.Reset();
  context.effects.Reset();
  context.effects.StartScreenTransition(ScreenTransition::CircleFadeIn);
  BGM_SetTempo(0);
}

bool GameplayState::EnterLive() { return EnterLive(context_); }

bool GameplayState::EnterReplay(std::string_view path, StageId stage) {
  return EnterReplay(context_, path, stage);
}

bool GameplayState::EnterDemo() { return EnterDemo(context_); }

FlowEvent GameplayState::Update(const FrameInput &frame) {
  return Update(context_, frame);
}

bool GameplayState::LoadCurrentStage(GameContext &context) {
  if (!context.graphics.LoadStage(context.session.stage)) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }
  if (!context.stage_loader.Load(context.session.stage, context.enemies,
                                 context.stage)) {
    DebugOut("MAP.PAK が破壊されています");
    return false;
  }
  return true;
}

void GameplayState::InitializeGameplayView(GameContext &context,
                                           bool interactive) {
  TextObj.Clear();
  if (mode_ != Mode::Demo) {
    BGM_FadeOut(240);
    context.effects.InitializeTextRenderer();
  }

  if (mode_ != Mode::Demo) {
    const auto flags = MsgWindowFlags::WITH_FACE;
    if (context.config.graphics.window_upper) {
      context.ui.InitMessageWindow({128, 16, 640 - 128, 96}, flags);
    } else if (!context.config.graphics.msg_disable) {
      context.ui.InitMessageWindow({128, 400, 640 - 128, 480}, flags);
    }
    if (interactive) {
      context.ui.InitExit();
      context.ui.InitGameOver();
    }
  }
  GrpBackend_SetClip(playfield::kClip);
}

bool GameplayState::EnterLive(GameContext &context) {
  mode_ = Mode::Live;
  phase_ = Phase::Running;
  if (!LoadCurrentStage(context)) {
    return false;
  }
  InitializeGameplayView(context, true);
  return true;
}

bool GameplayState::EnterReplay(GameContext &context, std::string_view path,
                                StageId stage) {
  if (!context.records.LoadReplay(path, stage) ||
      !context.records.ConfigurePlayback(context.config, context.session)) {
    return false;
  }

  GrpBackend_Clear();
  Grp_Flip();
  mode_ = Mode::Replay;
  phase_ = Phase::Running;
  ResetGameplayRuntime(context);
  context.records.RestorePlaybackStage(context.player, context.session);
  if (!LoadCurrentStage(context)) {
    StopPlayback(context);
    return false;
  }
  InitializeGameplayView(context, false);
  overlay_timer_ = 0;
  return true;
}

bool GameplayState::EnterDemo(GameContext &context) {
  GrpBackend_Clear();
  Grp_Flip();
  mode_ = Mode::Demo;
  phase_ = Phase::Running;
  ResetGameplayRuntime(context);
  context.player.Initialize(context.config.game.player_stock,
                            context.config.game.bomb_stock);
  rnd_seed_set(Time_SteadyTicksMS());
  context.session.stage = static_cast<StageId>(rnd() % kRegularStageCount);
  if (!context.records.LoadStageDemo(context.session.stage, context.player,
                                     context.session, context.config)) {
    return false;
  }
  context.session.ResetRank();
  if (!LoadCurrentStage(context)) {
    StopPlayback(context);
    return false;
  }
  InitializeGameplayView(context, false);
  overlay_timer_ = 0;
  return true;
}

bool GameplayState::LoadNextStage(GameContext &context) {
  context.session.AdvanceStage();
  ResetGameplayRuntime(context);
  context.player.PrepareNextStage();
  context.records.BeginStage(context.player, context.session);
  return LoadCurrentStage(context);
}

bool GameplayState::LoadNextReplayStage(GameContext &context) {
  if (!context.records.AdvancePlaybackStage()) {
    return false;
  }
  ResetGameplayRuntime(context);
  context.records.RestorePlaybackStage(context.player, context.session);
  if (!LoadCurrentStage(context)) {
    StopPlayback(context);
    return false;
  }
  return true;
}

GameplayState::StepResult
GameplayState::HandleStageTransition(GameContext &context,
                                     stage::StageTransition transition) {
  switch (transition) {
  case stage::StageTransition::None:
    return StepResult::Running;
  case stage::StageTransition::NextStage:
    if (context.records.IsRecording()) {
      context.records.FlushStage();
      return LoadNextStage(context) ? StepResult::Running
                                    : StepResult::LoadFailed;
    }
    if (context.records.IsMultiStagePlayback()) {
      return LoadNextReplayStage(context) ? StepResult::Running
                                          : StepResult::LoadFailed;
    }
    return LoadNextStage(context) ? StepResult::Running
                                  : StepResult::LoadFailed;
  case stage::StageTransition::GameClear:
    if (context.records.IsMultiStagePlayback()) {
      return StepResult::Running;
    }
    if (context.session.level != GameLevel::Easy) {
      context.session.extra_stg_flags |=
          static_cast<uint8_t>(1U << PlayerTypeIndex(context.player.Type()));
    }
    SaveConfigFile(context.config);
    if (context.records.IsRecording()) {
      context.records.FlushStage();
    }
    return StepResult::GameClear;
  case stage::StageTransition::ExtraClear:
    if (context.records.IsMultiStagePlayback()) {
      return StepResult::Running;
    }
    if (context.records.IsRecording()) {
      context.records.FlushStage();
    }
    return StepResult::ExtraClear;
  }
  std::unreachable();
}

GameplayState::StepResult GameplayState::Step(GameContext &context,
                                              INPUT_BITS &input) {
  context.ui.TickMessageWindow();
  const auto transition = context.stage.Update(
      {.enemies = context.enemies,
       .effects = context.effects,
       .ui = context.ui,
       .graphics = context.graphics,
       .music = context.music,
       .session = context.session,
       .messages_disabled = context.config.graphics.msg_disable},
      input);
  if (transition != stage::StageTransition::None) {
    return HandleStageTransition(context, transition);
  }

  context.enemies.Update();
  context.ui.UpdateBossHud(context.enemies.BossHud());
  context.items.Update();
  context.bullets.Update(context.enemies.HomingTarget());
  context.effects.Update();

  const auto player_result = context.player.Update(context.enemies, input);
  input = player_result.effective_input;
  if (player_result.clear_bullets) {
    context.bullets.Clear();
  }
  if (player_result.game_over) {
    BeginGameOver(context);
  }
  context.session.UpdateRank(context.stage.Frame());
  if (player_result.game_over) {
    return StepResult::GameOver;
  }
  return StepResult::Running;
}

void GameplayState::BeginGameOver(GameContext &context) {
  context.effects.SpawnGameOver();
  game_over_timer_ = 120;
  phase_ = Phase::GameOverIntro;
}

FlowEvent GameplayState::Update(GameContext &context, const FrameInput &frame) {
  switch (phase_) {
  case Phase::Paused:
    return UpdatePause(context, frame);
  case Phase::GameOverIntro:
    return UpdateGameOverIntro(context, frame);
  case Phase::GameOverMenu:
    return UpdateGameOverMenu(context, frame);
  case Phase::Running:
    break;
  }

  switch (mode_) {
  case Mode::Live:
    return UpdateLive(context, frame);
  case Mode::Replay:
    return UpdateReplay(context, frame);
  case Mode::Demo:
    return UpdateDemo(context, frame);
  }
  std::unreachable();
}

FlowEvent GameplayState::UpdateLive(GameContext &context,
                                    const FrameInput &frame) {
  auto input = frame.gameplay;
  context.records.Record(input);
  if ((input & KEY_ESC) != 0) {
    context.ui.PrepareExitMenu(context.records.HasRecordedStages());
    context.ui.Exit().Open({250, 150}, 1, input);
    BGM_Pause();
    SndBackend_PauseAll();
    phase_ = Phase::Paused;
    return NoEvent{};
  }

  const auto result = Step(context, input);
  context.records.UpdateLastRecordedInput(input);
  switch (result) {
  case StepResult::Running:
    break;
  case StepResult::GameOver:
    return NoEvent{};
  case StepResult::GameClear:
    return OpenEnding{};
  case StepResult::ExtraClear:
    return FinishRun{
        .extra_stage = true, .change_music = true, .save_replay = true};
  case StepResult::LoadFailed:
    context.records.CancelRecording();
    return ReturnToTitle{.change_music = true};
  }

  if (frame.should_draw) {
    Draw(context);
    if (context.records.IsRecording()) {
      constexpr PIXEL_LTRB rc = PIXEL_LTWH{288, 80, 24, 8};
      GrpSurface_Blit({128, 470}, SURFACE_ID::SYSTEM, rc);
    }
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::UpdatePause(GameContext &context,
                                     const FrameInput &frame) {
  context.ui.Exit().Tick(frame.gameplay);
  if (const auto action = context.ui.TakePauseAction()) {
    switch (*action) {
    case UIManager::PauseAction::SaveReplayAndExit:
      return SaveReplayAndExit{.extra_stage =
                                   context.session.stage == StageId::Extra};
    case UIManager::PauseAction::Exit:
      context.records.CancelRecording();
      return ReturnToTitle{.change_music = true};
    case UIManager::PauseAction::Resume:
      BGM_Resume();
      SndBackend_ResumeAll();
      phase_ = Phase::Running;
      return NoEvent{};
    }
  }

  if (frame.should_draw) {
    Draw(context);
    GrpBackend_SetClip(GRP_RES_RECT);
    context.ui.Exit().Draw();
    GrpBackend_SetClip(playfield::kClip);
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::UpdateGameOverIntro(GameContext &context,
                                             const FrameInput &frame) {
  switch (game_over_timer_) {
  default:
    game_over_timer_--;
    context.effects.UpdateGameOver();
    break;
  case 0:
    if (frame.gameplay != 0) {
      game_over_timer_--;
    }
    break;
  case -1:
    if (frame.gameplay != 0) {
      break;
    }
    BGM_Pause();
    SndBackend_PauseAll();
    context.ui.PrepareGameOverMenu(context.player.Credits() != 0U,
                                   context.records.HasRecordedStages());
    context.ui.GameOver().Open({200, 176}, 0, frame.gameplay);
    phase_ = Phase::GameOverMenu;
    return NoEvent{};
  }

  if (frame.should_draw) {
    Draw(context);
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::UpdateGameOverMenu(GameContext &context,
                                            const FrameInput &frame) {
  context.ui.GameOver().Tick(frame.gameplay);
  if (const auto action = context.ui.TakeGameOverAction()) {
    context.effects.ClearTextEffects();
    switch (*action) {
    case UIManager::GameOverAction::Continue:
      if (context.player.Credits() == 0U) {
        return NoEvent{};
      }
      BGM_Resume();
      SndBackend_ResumeAll();
      context.records.CancelRecording();
      context.player.ResetForContinue(context.config.game.player_stock);
      if (context.player.Credits() != 0U) {
        context.player.UseCredit();
      }
      phase_ = Phase::Running;
      return NoEvent{};
    case UIManager::GameOverAction::SaveReplayAndExit:
      return FinishRun{
          .extra_stage = context.session.stage == StageId::Extra,
          .change_music = true,
          .save_replay = true,
      };
    case UIManager::GameOverAction::Exit:
      context.records.CancelRecording();
      return FinishRun{
          .extra_stage = context.session.stage == StageId::Extra,
          .change_music = true,
          .save_replay = false,
      };
    }
  }

  if (frame.should_draw) {
    Draw(context);
    context.ui.GameOver().Draw();
    Grp_Flip();
  }
  return NoEvent{};
}

void GameplayState::StopPlayback(GameContext &context) {
  context.records.StopPlayback(context.config, context.session);
  context.session.is_demoplay = false;
}

FlowEvent GameplayState::UpdateReplay(GameContext &context,
                                      const FrameInput &frame) {
  overlay_timer_ = (overlay_timer_ + 1) % 128;
  if ((frame.gameplay & KEY_ESC) != 0) {
    StopPlayback(context);
    return ReturnToTitle{.change_music = true};
  }

  const int speed = (frame.system & SYSKEY_SKIP) != 0 ? 6 : 1;
  for (int i = 0; i < speed; i++) {
    auto input = context.records.NextInput();
    if ((input & KEY_ESC) != 0) {
      StopPlayback(context);
      return ReturnToTitle{.change_music = true};
    }
    const auto result = Step(context, input);
    if (result == StepResult::GameOver || result == StepResult::LoadFailed ||
        phase_ != Phase::Running || !context.records.IsPlaying()) {
      StopPlayback(context);
      return ReturnToTitle{.change_music = true};
    }
  }

  if (frame.should_draw) {
    Draw(context);
    constexpr PIXEL_LTWH replay_label = {312, 80, 32, 8};
    GrpSurface_Blit({128, 470}, SURFACE_ID::SYSTEM, replay_label);
    if (overlay_timer_ < 96) {
      GrpGeom->Lock();
      GrpGeom->SetAlphaNorm(128);
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->DrawBoxA(170, 473, 245, 478);
      GrpGeom->Unlock();
      constexpr PIXEL_LTWH skip_label = {312, 88, 72, 8};
      GrpSurface_Blit({173, 474}, SURFACE_ID::SYSTEM, skip_label);
    }
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::UpdateDemo(GameContext &context,
                                    const FrameInput &frame) {
  overlay_timer_ = (overlay_timer_ + 1) % 128;
  auto input = frame.gameplay != 0U ? KEY_ESC : context.records.NextInput();
  context.session.is_demoplay = true;
  if ((input & KEY_ESC) != 0) {
    StopPlayback(context);
    return ReturnToTitle{.change_music = false};
  }

  const auto result = Step(context, input);
  if (result != StepResult::Running || phase_ != Phase::Running) {
    StopPlayback(context);
    return ReturnToTitle{.change_music = false};
  }
  if (frame.should_draw) {
    Draw(context);
    if (overlay_timer_ < 64) {
      GrpPut16(200, 200, "D E M O   P L A Y");
    }
    Grp_Flip();
  }
  return NoEvent{};
}

void GameplayState::Draw(GameContext &context) const {
  const GameplayHudModel hud_model{
      .score = context.player.Score(),
      .bombs = context.player.Bombs(),
      .lives = context.player.Lives(),
      .credits = context.player.Credits(),
      .graze_count = context.player.GrazeCount(),
      .graze_wait_time = context.player.GrazeWaitTime(),
      .miss_count = context.player.MissCount(),
      .bomb_used = context.player.BombUsed(),
      .deathbomb_count = context.player.DeathbombCount(),
      .star_counter = context.player.StarCounter(),
      .star_threshold = context.player.StarThreshold(),
      .rank = context.session.rank,
      .level_name = GameLevelName(context.session.level),
      .practice_mode = context.config.game.practice_mode,
  };

  GrpBackend_Clear();
  context.stage.Draw();
  context.effects.DrawCircles();
  context.enemies.DrawBosses();
  context.player.DrawBombBackground();
  context.effects.DrawBombExplosions();
  context.enemies.DrawRegular();
  context.player.DrawProjectiles();
  context.player.Draw();
  context.effects.DrawFragments();
  context.items.Draw();
  context.bullets.Render();
  if (context.config.debug.hitbox_display != 0) {
    context.bullets.RenderDebugHitboxes(context.config.debug.hitbox_display);
    context.player.DrawDebugHitbox();
  }
  context.effects.DrawForeground();
  context.ui.DrawTopHud(hud_model);
  context.ui.DrawBossHud(context.stage.Frame());
  context.effects.DrawScreenTransition();
  context.ui.DrawMessageWindow();
  GrpBackend_SetClip(GRP_RES_RECT);
  context.ui.DrawSidebarHud(hud_model);
  GrpBackend_SetClip({playfield::kLeft, playfield::kTop, playfield::kRight + 1,
                      playfield::kBottom + 1});
}

} // namespace gameflow
