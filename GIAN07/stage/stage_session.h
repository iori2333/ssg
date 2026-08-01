///
/// StageSession - owns the active stage timeline and background
///
#pragma once

#include <cstdint>
#include <span>

#include "scene_program.h"
#include "stage_background.h"

#include "sys/input.h"

class EffectManager;
class EnemyManager;
struct GameSession;
class MusicPlayer;
class UiManager;

namespace i18n {
class Localization;
}

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
  EffectManager &effects;
  UiManager &ui;
  data::GraphicsLoader &graphics;
  MusicPlayer &music;
  const GameSession &session;
  i18n::Localization &localization;
  bool messages_disabled;
};

class StageSession {
public:
  [[nodiscard]] bool Load(std::span<const uint8_t> map,
                          std::span<const uint8_t> scene);

  [[nodiscard]] StageTransition Update(StageUpdateContext context,
                                       InputBits input);
  void Draw() const { background_.Draw(); }
  void Command(BackgroundCommand command, EffectManager &effects) {
    background_.Command(command, effects);
  }
  void CommandRocks(Stage4RockCommand command) {
    background_.CommandRocks(command);
  }

  [[nodiscard]] uint32_t Frame() const { return scene_.Frame(); }
  [[nodiscard]] bool DialogueActive() const { return scene_.MessageActive(); }

private:
  struct SceneStepResult {
    StageTransition transition = StageTransition::None;
    bool advance_frame = false;
  };

  [[nodiscard]] SceneStepResult RunScene(StageUpdateContext &context,
                                         InputBits input);
  [[nodiscard]] int32_t FindBossTimeout() const;
  void ExecuteEffect(SceneEffect effect, StageUpdateContext &context);

  SceneRunner scene_;
  StageBackground background_;
};

} // namespace stage
