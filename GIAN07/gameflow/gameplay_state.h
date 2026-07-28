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

  [[nodiscard]] bool EnterLive(GameContext &context);
  [[nodiscard]] bool EnterReplay(GameContext &context, std::string_view path,
                                 StageId stage);
  [[nodiscard]] bool EnterDemo(GameContext &context);
  [[nodiscard]] FlowEvent Update(GameContext &context, const FrameInput &frame);
  [[nodiscard]] bool LoadCurrentStage(GameContext &context);
  [[nodiscard]] bool LoadNextStage(GameContext &context);
  [[nodiscard]] bool LoadNextReplayStage(GameContext &context);
  void InitializeGameplayView(GameContext &context, bool interactive);
  [[nodiscard]] StepResult Step(GameContext &context, INPUT_BITS &input);
  [[nodiscard]] StepResult
  HandleStageTransition(GameContext &context,
                        stage::StageTransition transition);
  [[nodiscard]] FlowEvent UpdateLive(GameContext &context,
                                     const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateReplay(GameContext &context,
                                       const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateDemo(GameContext &context,
                                     const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdatePause(GameContext &context,
                                      const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateGameOverIntro(GameContext &context,
                                              const FrameInput &frame);
  [[nodiscard]] FlowEvent UpdateGameOverMenu(GameContext &context,
                                             const FrameInput &frame);
  void BeginGameOver(GameContext &context);
  void StopPlayback(GameContext &context);
  void Draw(GameContext &context) const;

  GameContext &context_;
  Mode mode_ = Mode::Live;
  Phase phase_ = Phase::Running;
  int game_over_timer_ = 0;
  uint8_t overlay_timer_ = 0;
};

} // namespace gameflow
