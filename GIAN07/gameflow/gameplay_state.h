/// Active gameplay flow: live runs, replays, demos, pause, and game over.

#pragma once

#include <cstdint>
#include <string_view>

#include "flow_types.h"

#include "stage/stage_session.h"

struct GameContext;

namespace gameflow {

void ResetGameplayRuntime(GameContext &context);

class GameplayState {
public:
  enum class Mode : uint8_t { Live, Replay, Demo };

  explicit GameplayState(GameContext &context) : context_(context) {}

  [[nodiscard]] bool EnterLive();
  [[nodiscard]] bool EnterReplay(std::string_view path, StageId stage);
  [[nodiscard]] bool EnterDemo();
  [[nodiscard]] FlowEvent Update(const FrameInput &frame);

private:
  enum class Phase : uint8_t { Running, Paused, GameOverIntro, GameOverMenu };
  enum class StepResult : uint8_t {
    Running,
    GameOver,
    GameClear,
    ExtraClear,
    LoadFailed,
  };

  [[nodiscard]] bool LoadCurrentStage();
  [[nodiscard]] bool LoadNextStage();
  [[nodiscard]] bool LoadNextReplayStage();
  void InitializeGameplayView(bool interactive);
  [[nodiscard]] StepResult Step(InputBits &input);
  [[nodiscard]] StepResult
  HandleStageTransition(stage::StageTransition transition);
  [[nodiscard]] FlowEvent UpdateLive(const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateReplay(const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateDemo(const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdatePause(const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateGameOverIntro(const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateGameOverMenu(const FrameInput &frame);
  [[nodiscard]] FlowEvent ExitDemoCapture();
  void BeginGameOver();
  void StopPlayback();
  void Draw() const;

  GameContext &context_;
  GameLevel previous_level_ = GameLevel::Normal;
  Mode mode_ = Mode::Live;
  Phase phase_ = Phase::Running;
  int game_over_timer_ = 0;
  uint8_t overlay_timer_ = 0;
  bool demo_visible_ = false;
};

} // namespace gameflow
