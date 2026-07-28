/// Application game-flow state machine.

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "flow_types.h"
#include "frontend_states.h"
#include "game_flow.h"
#include "gameplay_state.h"

#include "app/game_context.h"
#include "gameplay/game_rules.h"
#include "gfx/graphics.h"
#include "record/record_system.h"
#include "sys/input.h"
#include "ui/scenes/ending_scene.h"
#include "ui/scenes/music_room_scene.h"
#include "ui/scenes/replay_scene.h"
#include "ui/scenes/score_scene.h"

namespace gameflow {
namespace {

template <class... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};

class EndingFlowState {
public:
  explicit EndingFlowState(GameContext &context)
      : scene_(context.graphics, context.stage_loader, context.music) {}

  [[nodiscard]] bool Enter() { return scene_.Enter(); }
  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    if (scene_.Update(frame.should_draw)) {
      return FinishRun{
          .extra_stage = false, .change_music = false, .save_replay = true};
    }
    return NoEvent{};
  }

private:
  EndingScene scene_;
};

class MusicRoomFlowState {
public:
  explicit MusicRoomFlowState(GameContext &context)
      : scene_(context.data, context.graphics, context.music, context.config) {}

  [[nodiscard]] bool Enter() { return scene_.Enter(); }
  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    if (scene_.Update(frame.gameplay, frame.system, frame.should_draw)) {
      return ReturnToTitle{.change_music = true};
    }
    return NoEvent{};
  }

private:
  MusicRoomScene scene_;
};

class ScoreFlowState {
public:
  explicit ScoreFlowState(GameContext &context)
      : context_(context),
        scene_(context.records, context.graphics, context.music, context.ui) {}

  [[nodiscard]] bool EnterBrowser(GameLevel difficulty,
                                  INPUT_BITS initial_input) {
    registration_ = false;
    return scene_.ShowLeaderboard(difficulty, initial_input);
  }

  [[nodiscard]] ScoreRegistrationStart
  EnterRegistration(ScoreRecord record, INPUT_BITS initial_input,
                    bool change_music, bool save_replay, bool extra_stage) {
    registration_ = true;
    save_replay_ = save_replay;
    extra_stage_ = extra_stage;
    return scene_.StartNameRegistration(std::move(record), initial_input,
                                        change_music);
  }

  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    const auto result =
        registration_
            ? scene_.UpdateNameRegistration(frame.gameplay, frame.should_draw)
            : scene_.UpdateLeaderboard(frame.gameplay, frame.should_draw);
    if (result == ScoreSceneResult::ExitRequested) {
      return ReturnToTitle{.change_music = false};
    }
    if (result != ScoreSceneResult::RegistrationComplete) {
      return NoEvent{};
    }
    if (save_replay_ && context_.records.HasRecordedStages()) {
      return SaveReplayAndExit{.extra_stage = extra_stage_};
    }
    context_.records.CancelRecording();
    return ReturnToTitle{.change_music = true};
  }

private:
  GameContext &context_;
  ScoreScene scene_;
  bool registration_ = false;
  bool save_replay_ = false;
  bool extra_stage_ = false;
};

class ReplayFlowState {
public:
  explicit ReplayFlowState(GameContext &context)
      : context_(context),
        scene_(context.records, context.graphics, context.ui) {}

  [[nodiscard]] bool EnterBrowser(INPUT_BITS initial_input) {
    return scene_.EnterBrowser(initial_input);
  }

  [[nodiscard]] bool EnterSave(bool extra_stage, INPUT_BITS initial_input) {
    return scene_.BeginSave(extra_stage, initial_input);
  }

  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    auto result = scene_.Update(frame.gameplay, frame.should_draw);
    switch (result.type) {
    case ReplaySceneResult::Type::Running:
      return NoEvent{};
    case ReplaySceneResult::Type::ExitRequested:
      return ReturnToTitle{.change_music = false};
    case ReplaySceneResult::Type::PlaybackRequested:
      return StartReplay{.path = std::move(result.replay_path),
                         .stage = *result.stage};
    case ReplaySceneResult::Type::SaveComplete:
      if (!result.saved) {
        context_.records.CancelRecording();
      }
      return ReturnToTitle{.change_music = true};
    }
    std::unreachable();
  }

private:
  GameContext &context_;
  ReplayScene scene_;
};

using FlowState =
    std::variant<std::monostate, ProjectState, TitleState, WeaponSelectState,
                 GameplayState, EndingFlowState, ScoreFlowState,
                 ReplayFlowState, MusicRoomFlowState, BulletGalleryState>;

} // namespace

struct GameFlow::Impl {
  explicit Impl(GameContext &context) : context_(context) {}
  ~Impl() { context_.records.StopPlayback(context_.config, context_.session); }

  [[nodiscard]] bool Start() {
    auto &project = state_.emplace<ProjectState>();
    if (project.Enter(context_)) {
      return true;
    }
    return EnterTitle(true);
  }

  [[nodiscard]] bool Tick(INPUT_BITS input, INPUT_BITS system_input) {
    if (quit_) {
      return false;
    }
    current_input_ = input;
    const FrameInput frame{
        .gameplay = input,
        .system = system_input,
        .should_draw = ShouldDraw(),
    };
    auto event = std::visit(
        Overload{
            [](std::monostate &) -> FlowEvent { return QuitRequested{}; },
            [&](ProjectState &state) { return state.Update(context_, frame); },
            [&](TitleState &state) { return state.Update(context_, frame); },
            [&](WeaponSelectState &state) {
              return state.Update(context_, frame);
            },
            [&](GameplayState &state) { return state.Update(context_, frame); },
            [&](EndingFlowState &state) { return state.Update(frame); },
            [&](ScoreFlowState &state) { return state.Update(frame); },
            [&](ReplayFlowState &state) { return state.Update(frame); },
            [&](MusicRoomFlowState &state) { return state.Update(frame); },
            [&](BulletGalleryState &state) {
              return state.Update(context_, frame);
            },
        },
        state_);
    Handle(std::move(event));
    return !quit_;
  }

private:
  [[nodiscard]] bool ShouldDraw() {
    if (Grp_FPSDivisor != 0U) {
      draw_count_++;
      return (draw_count_ % Grp_FPSDivisor) == 0U;
    }
    return true;
  }

  [[nodiscard]] bool EnterTitle(bool change_music) {
    auto &title = state_.emplace<TitleState>();
    if (title.Enter(context_, current_input_, change_music)) {
      return true;
    }
    quit_ = true;
    return false;
  }

  void EnterReplaySave(bool extra_stage) {
    auto &state = state_.emplace<ReplayFlowState>(context_);
    if (!state.EnterSave(extra_stage, current_input_)) {
      context_.records.CancelRecording();
      (void)EnterTitle(true);
    }
  }

  void EnterScoreRegistration(const FinishRun &finish) {
    auto score =
        context_.records.CaptureScore(context_.player, context_.session);
    auto &state = state_.emplace<ScoreFlowState>(context_);
    const auto start = state.EnterRegistration(
        std::move(score), current_input_, finish.change_music,
        finish.save_replay, finish.extra_stage);
    if (start == ScoreRegistrationStart::Complete) {
      if (finish.save_replay && context_.records.HasRecordedStages()) {
        EnterReplaySave(finish.extra_stage);
      } else {
        context_.records.CancelRecording();
        (void)EnterTitle(true);
      }
    }
  }

  void Handle(FlowEvent event) {
    std::visit(
        Overload{
            [](NoEvent) {},
            [&](QuitRequested) { quit_ = true; },
            [&](ReturnToTitle event) { (void)EnterTitle(event.change_music); },
            [&](StartWeaponSelect event) {
              auto &state = state_.emplace<WeaponSelectState>();
              if (!state.Enter(context_, event.extra_stage)) {
                (void)EnterTitle(true);
              }
            },
            [&](StartLiveGame) {
              auto &state = state_.emplace<GameplayState>();
              if (!state.EnterLive(context_)) {
                context_.records.CancelRecording();
                (void)EnterTitle(true);
              }
            },
            [&](StartDemo) {
              auto &state = state_.emplace<GameplayState>();
              if (!state.EnterDemo(context_)) {
                (void)EnterTitle(false);
              }
            },
            [&](StartReplay &&event) {
              auto &state = state_.emplace<GameplayState>();
              if (!state.EnterReplay(context_, event.path, event.stage)) {
                auto &browser = state_.emplace<ReplayFlowState>(context_);
                if (!browser.EnterBrowser(current_input_)) {
                  (void)EnterTitle(false);
                }
              }
            },
            [&](OpenReplayBrowser) {
              auto &state = state_.emplace<ReplayFlowState>(context_);
              if (!state.EnterBrowser(current_input_)) {
                (void)EnterTitle(false);
              }
            },
            [&](OpenScoreBrowser event) {
              auto &state = state_.emplace<ScoreFlowState>(context_);
              if (!state.EnterBrowser(event.difficulty, current_input_)) {
                (void)EnterTitle(false);
              }
            },
            [&](OpenMusicRoom) {
              context_.ui.ForceCloseMessageWindow();
              auto &state = state_.emplace<MusicRoomFlowState>(context_);
              if (!state.Enter()) {
                (void)EnterTitle(true);
              }
            },
            [&](OpenBulletGallery) {
              auto &state = state_.emplace<BulletGalleryState>();
              if (!state.Enter(context_)) {
                (void)EnterTitle(false);
              }
            },
            [&](OpenEnding) {
              auto &state = state_.emplace<EndingFlowState>(context_);
              if (!state.Enter()) {
                EnterScoreRegistration(FinishRun{.extra_stage = false,
                                                 .change_music = true,
                                                 .save_replay = true});
              }
            },
            [&](FinishRun event) { EnterScoreRegistration(event); },
            [&](SaveReplayAndExit event) {
              EnterReplaySave(event.extra_stage);
            },
        },
        std::move(event));
  }

  GameContext &context_;
  FlowState state_;
  INPUT_BITS current_input_ = 0;
  uint32_t draw_count_ = 0;
  bool quit_ = false;
};

GameFlow::GameFlow(GameContext &context)
    : impl_(std::make_unique<Impl>(context)) {}

GameFlow::~GameFlow() = default;

bool GameFlow::Start() { return impl_->Start(); }

bool GameFlow::Tick(INPUT_BITS input, INPUT_BITS system_input) {
  return impl_->Tick(input, system_input);
}

} // namespace gameflow
