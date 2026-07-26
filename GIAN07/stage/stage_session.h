///
/// StageSession - owns the active stage timeline and background
///
#pragma once

#include <cstdint>

#include "scene_program.h"
#include "stage_background.h"

#include "sys/input.h"

struct BossManager;
struct EffectManager;
struct EnemyManager;
struct GameManager;
class TrackManager;
class UIManager;

namespace data {
class GraphicsLoader;
}

namespace stage {

enum class StageTransition : uint8_t {
  None,
  NextStage,
  GameClear,
  ExtraClear,
};

struct StageUpdateContext {
  EnemyManager &enemies;
  BossManager &bosses;
  EffectManager &effects;
  UIManager &ui;
  data::GraphicsLoader &graphics;
  TrackManager &tracks;
  const GameManager &game;
  bool messages_disabled;
};

class StageSession {
public:
  [[nodiscard]] bool Load(BYTE_BUFFER_OWNED map, BYTE_BUFFER_BORROWED scene);

  [[nodiscard]] StageTransition Update(StageUpdateContext context,
                                       INPUT_BITS input);
  void Draw(EffectManager &effects) const { background_.Draw(effects); }
  void Command(BackgroundCommand command, EffectManager &effects) {
    background_.Command(command, effects);
  }

  [[nodiscard]] uint32_t Frame() const { return scene_.Frame(); }
  [[nodiscard]] bool DialogueActive() const { return scene_.MessageActive(); }

private:
  struct SceneStepResult {
    StageTransition transition = StageTransition::None;
    bool advance_frame = false;
  };

  [[nodiscard]] SceneStepResult RunScene(StageUpdateContext &context,
                                         INPUT_BITS input);
  [[nodiscard]] int32_t FindBossTimeout() const;
  void ExecuteEffect(SceneEffect effect, StageUpdateContext &context);

  SceneRunner scene_;
  StageBackground background_;
};

} // namespace stage
