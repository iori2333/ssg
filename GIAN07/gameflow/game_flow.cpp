/// Application game-flow state machine.

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "flow_types.h"
#include "game_flow.h"
#include "gameplay_state.h"

#include "app/game_context.h"
#include "gameplay/game_rules.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "i18n/localization.h"
#include "platform/text_backend.h"
#include "player/loadout/player_loadout.h"
#include "record/record_system.h"
#include "sys/input.h"
#include "ui/menu/menu_controller.h"
#include "ui/menu/menu_tree.h"
#include "ui/scenes/bullet_gallery_scene.h"
#include "ui/scenes/ending_scene.h"
#include "ui/scenes/music_room_scene.h"
#include "ui/scenes/replay_scene.h"
#include "ui/scenes/score_scene.h"
#include "ui/scenes/startup_scene.h"
#include "ui/scenes/title_scene.h"
#include "ui/scenes/weapon_select_scene.h"

namespace gameflow {
namespace {

// NOLINTNEXTLINE(misc-multiple-inheritance) - intentional overload pattern.
template <typename... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};

class StartupFlowState {
public:
  explicit StartupFlowState(GameContext &context) : scene_(context.graphics) {}

  [[nodiscard]] bool Enter() { return scene_.Enter(); }
  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    if (scene_.Update(frame.should_draw) == StartupSceneResult::Complete) {
      return ReturnToTitle{.change_music = true};
    }
    return NoEvent{};
  }

private:
  StartupScene scene_;
};

class TitleFlowState {
public:
  explicit TitleFlowState(GameContext &context)
      : context_(context), scene_(context.graphics, context.music,
                                  context.session, context.ui, context.audio) {}

  [[nodiscard]] bool Enter(InputBits initial_input, bool change_music) {
    return scene_.Enter(initial_input, change_music);
  }

  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    switch (scene_.Update(frame.gameplay, frame.should_draw)) {
    case TitleSceneResult::Running:
      return NoEvent{};
    case TitleSceneResult::QuitRequested:
      return QuitRequested{};
    case TitleSceneResult::StartGame:
      return StartWeaponSelect{.extra_stage = false};
    case TitleSceneResult::StartExtra:
      return StartWeaponSelect{.extra_stage = true};
    case TitleSceneResult::StartDemo:
      return StartDemo{};
    case TitleSceneResult::OpenReplay:
      return OpenReplayBrowser{};
    case TitleSceneResult::OpenScore:
      return OpenScoreBrowser{
          .difficulty = context_.session.stage == StageId::Extra
                            ? GameLevel::Extra
                            : context_.session.level,
      };
    case TitleSceneResult::OpenMusicRoom:
      return OpenMusicRoom{};
    case TitleSceneResult::OpenBulletGallery:
      return OpenBulletGallery{};
    }
    std::unreachable();
  }

private:
  GameContext &context_;
  TitleScene scene_;
};

class WeaponSelectFlowState {
public:
  explicit WeaponSelectFlowState(GameContext &context)
      : context_(context),
        scene_(context.config, context.enemies, context.session, context.player,
               context.audio),
        difficulty_menu_(context.audio) {
    BuildDifficultyMenu();
  }

  void Enter(bool extra_stage) {
    GraphicsBackendClear();
    GraphicsFlip();
    extra_stage_ = extra_stage;
    phase_ = Phase::WeaponSelect;
    context_.session.level = extra_stage ? GameLevel::Extra : GameLevel::Normal;
    ResetGameplayRuntime(context_);
    context_.session.ResetRank();
    context_.player.SelectType(PlayerType::Wide);
    context_.player.Initialize(context_.config.game.player_stock,
                               context_.config.game.bomb_stock);
    if (extra_stage) {
      context_.session.stage = StageId::Extra;
    }
    scene_.Enter();
  }

  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    if (phase_ == Phase::DifficultySelect) {
      difficulty_menu_.Tick(frame.gameplay);
      if (selected_difficulty_) {
        context_.session.level = *selected_difficulty_;
        context_.session.ResetRank();
        return BeginGame();
      }
      if (!difficulty_menu_.Active()) {
        phase_ = Phase::WeaponSelect;
        scene_.Enter();
        return NoEvent{};
      }
      if (frame.should_draw) {
        scene_.DrawPreview();
        difficulty_menu_.Draw();
        GraphicsFlip();
      }
      return NoEvent{};
    }

    switch (scene_.Update(frame.gameplay, frame.should_draw)) {
    case WeaponSelectSceneResult::Running:
      return NoEvent{};
    case WeaponSelectSceneResult::StartGame:
      if (!extra_stage_) {
        phase_ = Phase::DifficultySelect;
        OpenDifficultyMenu(frame.gameplay);
        return NoEvent{};
      }
      return BeginGame();
    case WeaponSelectSceneResult::Cancelled:
      return ReturnToTitle{.change_music = false};
    }
    std::unreachable();
  }

private:
  enum class Phase : uint8_t {
    WeaponSelect,
    DifficultySelect,
  };

  void BuildDifficultyMenu() {
    constexpr std::array difficulties = {
        GameLevel::Easy,
        GameLevel::Normal,
        GameLevel::Hard,
        GameLevel::Lunatic,
    };
    std::vector<std::unique_ptr<menu::IMenuNode>> items;
    items.reserve(difficulties.size());
    for (const auto difficulty : difficulties) {
      items.push_back(std::make_unique<menu::ActionNode>(
          std::string(GameLevelName(difficulty)), "",
          [this, difficulty](menu::MenuController &) {
            selected_difficulty_ = difficulty;
            return false;
          }));
    }

    const auto title_id = i18n::TextIdFromKey("ui.menu.difficulty.title");
    difficulty_menu_root_ = std::make_unique<menu::EntryNode>(
        menu::MenuText(
            [this, title_id] { return context_.localization.Text(title_id); }),
        "", std::move(items));
  }

  void OpenDifficultyMenu(InputBits initial_input) {
    constexpr WindowPoint kTopLeft = {240, 192};
    constexpr int kWidth = 160;
    constexpr int kInitialSelection = std::to_underlying(GameLevel::Normal);

    TextRenderer().Clear();
    selected_difficulty_.reset();
    difficulty_menu_.Init(kWidth);
    difficulty_menu_.Navigate(*difficulty_menu_root_, kInitialSelection);
    difficulty_menu_.Open(kTopLeft, kInitialSelection, initial_input);
  }

  [[nodiscard]] FlowEvent BeginGame() {
    scene_.PrepareGameStart();
    if (context_.config.debug.demo_recording) {
      context_.records.BeginDemoCapture(context_.player, context_.session,
                                        context_.config);
    } else {
      context_.records.BeginRecording(context_.player, context_.session,
                                      context_.config);
    }
    return StartLiveGame{};
  }

  GameContext &context_;
  WeaponSelectScene scene_;
  std::unique_ptr<menu::EntryNode> difficulty_menu_root_;
  menu::MenuController difficulty_menu_;
  std::optional<GameLevel> selected_difficulty_;
  Phase phase_ = Phase::WeaponSelect;
  bool extra_stage_ = false;
};

class BulletGalleryFlowState {
public:
  explicit BulletGalleryFlowState(GameContext &context)
      : scene_(context.config, context.graphics, context.bullets,
               context.player, context.localization) {}

  [[nodiscard]] bool Enter() { return scene_.Enter(); }
  [[nodiscard]] FlowEvent Update(const FrameInput &frame) {
    if (scene_.Update(frame.gameplay, frame.should_draw) ==
        BulletGallerySceneResult::ExitRequested) {
      return ReturnToTitle{.change_music = false};
    }
    return NoEvent{};
  }

private:
  BulletGalleryScene scene_;
};

class EndingFlowState {
public:
  explicit EndingFlowState(GameContext &context)
      : scene_(context.graphics, context.stage_loader, context.music,
               context.localization, context.audio) {}

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
      : scene_(context.graphics, context.music, context.localization,
               context.audio) {}

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
        scene_(context.records, context.graphics, context.music, context.ui,
               context.localization, context.audio) {}

  [[nodiscard]] bool EnterBrowser(GameLevel difficulty,
                                  InputBits initial_input) {
    registration_ = false;
    return scene_.ShowLeaderboard(difficulty, initial_input);
  }

  [[nodiscard]] ScoreRegistrationStart
  EnterRegistration(ScoreRecord record, InputBits initial_input,
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
      : context_(context), scene_(context.records, context.graphics, context.ui,
                                  context.localization, context.audio) {}

  [[nodiscard]] bool EnterBrowser(InputBits initial_input) {
    return scene_.EnterBrowser(initial_input);
  }

  [[nodiscard]] bool EnterSave(bool extra_stage, InputBits initial_input) {
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

using FlowState = std::variant<std::monostate, StartupFlowState, TitleFlowState,
                               WeaponSelectFlowState, GameplayState,
                               EndingFlowState, ScoreFlowState, ReplayFlowState,
                               MusicRoomFlowState, BulletGalleryFlowState>;

} // namespace

struct GameFlow::Impl {
  explicit Impl(GameContext &context) : context_(context) {}
  ~Impl() { context_.records.StopPlayback(); }
  Impl(const Impl &) = delete;
  Impl(Impl &&) = delete;
  Impl &operator=(const Impl &) = delete;
  Impl &operator=(Impl &&) = delete;

  [[nodiscard]] bool Start() {
    auto &startup = state_.emplace<StartupFlowState>(context_);
    if (startup.Enter()) {
      return true;
    }
    return EnterTitle(true);
  }

  [[nodiscard]] bool Tick(InputBits input, InputBits system_input) {
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
            [&](auto &state) -> FlowEvent { return state.Update(frame); },
        },
        state_);
    Handle(std::move(event));
    return !quit_;
  }

private:
  [[nodiscard]] bool ShouldDraw() {
    if (FrameRateDivisor() != 0U) {
      draw_count_++;
      return (draw_count_ % FrameRateDivisor()) == 0U;
    }
    return true;
  }

  [[nodiscard]] bool EnterTitle(bool change_music) {
    auto &title = state_.emplace<TitleFlowState>(context_);
    if (title.Enter(current_input_, change_music)) {
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
        RecordSystem::CaptureScore(context_.player, context_.session);
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
              auto &state = state_.emplace<WeaponSelectFlowState>(context_);
              state.Enter(event.extra_stage);
            },
            [&](StartLiveGame) {
              auto &state = state_.emplace<GameplayState>(context_);
              if (!state.EnterLive()) {
                context_.records.CancelRecording();
                (void)EnterTitle(true);
              }
            },
            [&](StartDemo) {
              auto &state = state_.emplace<GameplayState>(context_);
              if (!state.EnterDemo()) {
                (void)EnterTitle(false);
              }
            },
            [&](const StartReplay &event) {
              auto &state = state_.emplace<GameplayState>(context_);
              if (!state.EnterReplay(event.path, event.stage)) {
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
              auto &state = state_.emplace<BulletGalleryFlowState>(context_);
              if (!state.Enter()) {
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
  InputBits current_input_ = 0;
  uint32_t draw_count_ = 0;
  bool quit_ = false;
};

GameFlow::GameFlow(GameContext &context)
    : impl_(std::make_unique<Impl>(context)) {}

GameFlow::~GameFlow() = default;

bool GameFlow::Start() { return impl_->Start(); }

bool GameFlow::Tick(InputBits input, InputBits system_input) {
  return impl_->Tick(input, system_input);
}

} // namespace gameflow
