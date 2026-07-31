/// Active gameplay flow: live runs, replays, demos, pause, and game over.

#include <array>
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
#include "sys/log.h"
#include "util/math_utils.h"
#include "util/time.h"

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
  context.player.SetFocusHitboxVisible(context.config.game.show_focus_hitbox);
  context.enemies.Reset();
  context.ui.UpdateBossHud(context.enemies.BossHud());
  context.bullets.Init();
  context.items.Reset();
  context.effects.Reset();
  context.effects.StartScreenTransition(ScreenTransition::CircleFadeIn);
  BGM_SetTempo(0);
}

bool GameplayState::LoadCurrentStage() {
  auto &context = context_;
  if (!context.graphics.LoadStage(context.session.stage)) {
    logging::Error(logging::Channel::Graphics,
                   "Failed to load graphics for stage {}",
                   std::to_underlying(context.session.stage) + 1);
    return false;
  }
  if (!context.stage_loader.Load(context.session.stage, context.enemies,
                                 context.stage)) {
    return false;
  }
  return true;
}

void GameplayState::InitializeGameplayView(bool interactive) {
  auto &context = context_;
  TextObj.Clear();
  if (mode_ != Mode::Demo) {
    BGM_FadeOut(240);
    context.effects.InitializeTextRenderer();
  }

  if (mode_ != Mode::Demo) {
    const auto flags = MsgWindowFlags::WITH_FACE;
    if (context.config.ui.message_window == MessageWindowMode::Upper) {
      context.ui.InitMessageWindow({128, 16, 640 - 128, 96}, flags);
    } else if (context.config.ui.message_window == MessageWindowMode::Lower) {
      context.ui.InitMessageWindow({128, 400, 640 - 128, 480}, flags);
    }
    if (interactive) {
      context.ui.InitExit();
      context.ui.InitGameOver();
    }
  }
  GrpBackend_SetClip(playfield::kClip);
}

bool GameplayState::EnterLive() {
  auto &context = context_;
  mode_ = Mode::Live;
  phase_ = Phase::Running;
  if (!LoadCurrentStage()) {
    return false;
  }
  InitializeGameplayView(true);
  return true;
}

bool GameplayState::EnterReplay(std::string_view path, StageId stage) {
  auto &context = context_;
  previous_level_ = context.session.level;
  if (!context.records.LoadReplay(path, stage)) {
    return false;
  }

  GrpBackend_Clear();
  Grp_Flip();
  mode_ = Mode::Replay;
  phase_ = Phase::Running;
  ResetGameplayRuntime(context);
  if (!context.records.ConfigurePlayback(context.player, context.session)) {
    return false;
  }
  context.records.RestorePlaybackStage(context.player, context.session);
  if (!LoadCurrentStage()) {
    StopPlayback();
    return false;
  }
  InitializeGameplayView(false);
  overlay_timer_ = 0;
  return true;
}

bool GameplayState::EnterDemo() {
  auto &context = context_;
  previous_level_ = context.session.level;
  GrpBackend_Clear();
  Grp_Flip();
  mode_ = Mode::Demo;
  phase_ = Phase::Running;
  ResetGameplayRuntime(context);
  math::SeedRandom(Time_SteadyTicksMS());
  std::array<StageId, kRegularStageCount> available{};
  size_t available_count = 0;
  for (uint8_t index = 0; index < kRegularStageCount; ++index) {
    const auto stage = static_cast<StageId>(index);
    if (context.records.HasStageDemo(stage)) {
      available[available_count++] = stage;
    }
  }
  if (available_count == 0) {
    return false;
  }
  context.session.stage = available[math::RandomInt() % available_count];
  if (!context.records.LoadStageDemo(context.session.stage, context.player,
                                     context.session)) {
    return false;
  }
  if (!LoadCurrentStage()) {
    StopPlayback();
    return false;
  }
  InitializeGameplayView(false);
  overlay_timer_ = 0;
  demo_visible_ = false;
  return true;
}

bool GameplayState::LoadNextStage() {
  auto &context = context_;
  context.session.AdvanceStage();
  ResetGameplayRuntime(context);
  context.player.PrepareNextStage();
  context.records.BeginStage(context.player, context.session);
  return LoadCurrentStage();
}

bool GameplayState::LoadNextReplayStage() {
  auto &context = context_;
  if (!context.records.AdvancePlaybackStage()) {
    return false;
  }
  ResetGameplayRuntime(context);
  if (!context.records.ConfigurePlayback(context.player, context.session)) {
    return false;
  }
  context.records.RestorePlaybackStage(context.player, context.session);
  if (!LoadCurrentStage()) {
    StopPlayback();
    return false;
  }
  return true;
}

GameplayState::StepResult
GameplayState::HandleStageTransition(stage::StageTransition transition) {
  auto &context = context_;
  switch (transition) {
  case stage::StageTransition::None:
    return StepResult::Running;
  case stage::StageTransition::NextStage:
    if (context.records.IsRecording()) {
      context.records.FlushStage();
      return LoadNextStage() ? StepResult::Running : StepResult::LoadFailed;
    }
    if (context.records.IsMultiStagePlayback()) {
      return LoadNextReplayStage() ? StepResult::Running
                                   : StepResult::LoadFailed;
    }
    return LoadNextStage() ? StepResult::Running : StepResult::LoadFailed;
  case stage::StageTransition::GameClear:
    if (context.records.IsMultiStagePlayback()) {
      return StepResult::Running;
    }
    if (context.session.level != GameLevel::Easy) {
      auto &flags = context.config.progress.extra_stg_flags;
      const auto unlocked =
          static_cast<uint8_t>(1U << PlayerTypeIndex(context.player.Type()));
      if ((flags & unlocked) == 0) {
        flags |= unlocked;
        context.save_config();
      }
    }
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

GameplayState::StepResult GameplayState::Step(INPUT_BITS &input) {
  auto &context = context_;
  context.ui.TickMessageWindow();
  const auto transition = context.stage.Update(
      {.enemies = context.enemies,
       .effects = context.effects,
       .ui = context.ui,
       .graphics = context.graphics,
       .music = context.music,
       .session = context.session,
       .localization = context.localization,
       .messages_disabled =
           context.config.ui.message_window == MessageWindowMode::Hidden},
      input);
  if (transition != stage::StageTransition::None) {
    return HandleStageTransition(transition);
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
    BeginGameOver();
  }
  context.session.UpdateRank(context.stage.Frame());
  if (player_result.game_over) {
    return StepResult::GameOver;
  }
  return StepResult::Running;
}

void GameplayState::BeginGameOver() {
  auto &context = context_;
  context.effects.SpawnGameOver();
  if (mode_ == Mode::Live) {
    BGM_Pause();
  }
  game_over_timer_ = 120;
  phase_ = Phase::GameOverIntro;
}

FlowEvent GameplayState::Update(const FrameInput &frame) {
  switch (phase_) {
  case Phase::Paused:
    return UpdatePause(frame);
  case Phase::GameOverIntro:
    return UpdateGameOverIntro(frame);
  case Phase::GameOverMenu:
    return UpdateGameOverMenu(frame);
  case Phase::Running:
    break;
  }

  switch (mode_) {
  case Mode::Live:
    return UpdateLive(frame);
  case Mode::Replay:
    return UpdateReplay(frame);
  case Mode::Demo:
    return UpdateDemo(frame);
  }
  std::unreachable();
}

FlowEvent GameplayState::UpdateLive(const FrameInput &frame) {
  auto &context = context_;
  auto input = frame.gameplay;
  if (context.config.debug.demo_recording &&
      (frame.system & SYSKEY_DEMO_RECORD) != 0 &&
      !context.records.IsRecording()) {
    (void)context.records.MarkDemoStart();
  }
  context.records.Record(input);
  if ((input & KEY_ESC) != 0) {
    context.ui.PrepareExitMenu(!context.config.debug.demo_recording &&
                               context.records.HasRecordedStages());
    context.ui.Exit().Open({230, 150}, 0, input);
    BGM_Pause();
    SndBackend_PauseAll();
    phase_ = Phase::Paused;
    return NoEvent{};
  }

  const auto result = Step(input);
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
    Draw();
    if (context.records.IsRecording()) {
      constexpr PIXEL_LTRB rc = PIXEL_LTWH{288, 80, 24, 8};
      GrpSurface_Blit({128, 470}, SURFACE_ID::SYSTEM, rc);
    }
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::UpdatePause(const FrameInput &frame) {
  auto &context = context_;
  context.ui.Exit().Tick(frame.gameplay);
  if (const auto action = context.ui.TakePauseAction()) {
    switch (*action) {
    case UIManager::PauseAction::SaveReplayAndExit:
      if (context.config.debug.demo_recording) {
        return ExitDemoCapture();
      }
      return SaveReplayAndExit{.extra_stage =
                                   context.session.stage == StageId::Extra};
    case UIManager::PauseAction::Exit:
      if (context.config.debug.demo_recording) {
        return ExitDemoCapture();
      } else {
        context.records.CancelRecording();
      }
      return ReturnToTitle{.change_music = true};
    case UIManager::PauseAction::Resume:
      BGM_Resume();
      SndBackend_ResumeAll();
      phase_ = Phase::Running;
      return NoEvent{};
    }
  }

  if (frame.should_draw) {
    Draw();
    GrpBackend_SetClip(GRP_RES_RECT);
    context.ui.Exit().Draw();
    GrpBackend_SetClip(playfield::kClip);
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::ExitDemoCapture() {
  auto &context = context_;
  const auto saved = context.records.SaveDemo(context.session.stage);
  if (saved == RecordSaveResult::IoError) {
    logging::Error(logging::Channel::Record, "Demo replay could not be saved");
  }
  if (saved != RecordSaveResult::Saved) {
    context.records.CancelRecording();
  }
  return ReturnToTitle{.change_music = true};
}

FlowEvent GameplayState::UpdateGameOverIntro(const FrameInput &frame) {
  auto &context = context_;
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
    SndBackend_PauseAll();
    context.ui.PrepareGameOverMenu(context.player.Credits() != 0U,
                                   !context.config.debug.demo_recording &&
                                       context.records.HasRecordedStages());
    context.ui.GameOver().Open({200, 176}, 0, frame.gameplay);
    phase_ = Phase::GameOverMenu;
    return NoEvent{};
  }

  if (frame.should_draw) {
    Draw();
    Grp_Flip();
  }
  return NoEvent{};
}

FlowEvent GameplayState::UpdateGameOverMenu(const FrameInput &frame) {
  auto &context = context_;
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
      if (context.config.debug.demo_recording) {
        return ExitDemoCapture();
      }
      return FinishRun{
          .extra_stage = context.session.stage == StageId::Extra,
          .change_music = true,
          .save_replay = true,
      };
    case UIManager::GameOverAction::Exit:
      if (context.config.debug.demo_recording) {
        return ExitDemoCapture();
      }
      context.records.CancelRecording();
      return FinishRun{
          .extra_stage = context.session.stage == StageId::Extra,
          .change_music = true,
          .save_replay = false,
      };
    }
  }

  if (frame.should_draw) {
    Draw();
    context.ui.GameOver().Draw();
    Grp_Flip();
  }
  return NoEvent{};
}

void GameplayState::StopPlayback() {
  auto &context = context_;
  context.records.StopPlayback();
  context.session.level = previous_level_;
  context.player.Configure(context.config.game.practice_mode,
                           context.config.input.z_spd_down_enabled);
  context.session.is_demoplay = false;
}

FlowEvent GameplayState::UpdateReplay(const FrameInput &frame) {
  auto &context = context_;
  overlay_timer_ = (overlay_timer_ + 1) % 128;
  if ((frame.gameplay & KEY_ESC) != 0) {
    StopPlayback();
    return ReturnToTitle{.change_music = true};
  }

  const int speed = (frame.system & SYSKEY_SKIP) != 0 ? 6 : 1;
  for (int i = 0; i < speed; i++) {
    auto input = context.records.NextInput();
    if ((input & KEY_ESC) != 0) {
      StopPlayback();
      return ReturnToTitle{.change_music = true};
    }
    input &= ~KEY_DEMO_START;
    const auto result = Step(input);
    if (result == StepResult::GameOver || result == StepResult::LoadFailed ||
        phase_ != Phase::Running || !context.records.IsPlaying()) {
      StopPlayback();
      return ReturnToTitle{.change_music = true};
    }
  }

  if (frame.should_draw) {
    Draw();
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

FlowEvent GameplayState::UpdateDemo(const FrameInput &frame) {
  auto &context = context_;
  context.session.is_demoplay = true;
  constexpr auto kPrerollStepsPerFrame = 240;
  const auto step_count = demo_visible_ ? 1 : kPrerollStepsPerFrame;
  for (int step = 0; step < step_count; ++step) {
    auto input = frame.gameplay != 0U ? KEY_ESC : context.records.NextInput();
    const bool demo_start = (input & KEY_DEMO_START) != 0;
    input &= ~KEY_DEMO_START;
    if ((input & KEY_ESC) != 0) {
      StopPlayback();
      return ReturnToTitle{.change_music = false};
    }

    const auto result = Step(input);
    if (result != StepResult::Running || phase_ != Phase::Running) {
      StopPlayback();
      return ReturnToTitle{.change_music = false};
    }
    if (demo_start) {
      demo_visible_ = true;
      break;
    }
  }

  if (demo_visible_) {
    overlay_timer_ = (overlay_timer_ + 1) % 128;
  }
  if (frame.should_draw && demo_visible_) {
    Draw();
    if (overlay_timer_ < 64) {
      GrpPut16(200, 200, "D E M O   P L A Y");
    }
    Grp_Flip();
  }
  return NoEvent{};
}

void GameplayState::Draw() const {
  auto &context = context_;
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
      .practice_mode = context.player.Practice(),
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
