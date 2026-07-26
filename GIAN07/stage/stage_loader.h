///
/// StageLoader - installs validated stage assets into a game session
///
#pragma once

#include "core/game_manager.h"
#include "data/game_data.h"

struct EnemyManager;
struct ScrollManager;

namespace stage {

class StageLoader {
public:
  explicit StageLoader(const data::GameData &data) : data_(&data) {}

  [[nodiscard]] bool Load(StageId stage, EnemyManager &enemies,
                          ScrollManager &scroller, GameManager &game) const;
  [[nodiscard]] bool LoadEnding(EnemyManager &enemies,
                                ScrollManager &scroller,
                                GameManager &game) const;

private:
  const data::GameData *data_;
};

} // namespace stage
