///
/// StageLoader - installs validated stage assets into a game session
///
#pragma once

#include "data/game_data.h"
#include "gameplay/game_rules.h"

class EnemyManager;

namespace stage {

class SceneRunner;
class StageSession;

class StageLoader {
public:
  explicit StageLoader(const data::GameData &data) : data_(&data) {}

  [[nodiscard]] bool Load(StageId stage, EnemyManager &enemies,
                          StageSession &session) const;
  [[nodiscard]] bool LoadEnding(SceneRunner &scene) const;

private:
  const data::GameData *data_;
};

} // namespace stage
