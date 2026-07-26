///
/// StageLoader - installs validated stage assets into a game session
///
#include <cstddef>
#include <utility>

#include "anime_data.h"
#include "scroll_manager.h"
#include "stage_loader.h"

#include "enemy/enemy_manager.h"
#include "scripts_data.h"

namespace {

BYTE_BUFFER_BORROWED LoadEmbeddedScript(int file_no) {
  for (size_t i = 0; i < embedded_script_count; ++i) {
    if (embedded_scripts[i].index == file_no) {
      return {embedded_scripts[i].data, embedded_scripts[i].size};
    }
  }
  return {};
}

} // namespace

namespace stage {

bool StageLoader::Load(StageId stage, EnemyManager &enemies,
                       ScrollManager &scroller, GameManager &game) const {
  const auto stage_index = std::to_underlying(stage);
  if (stage_index > std::to_underlying(StageId::EXTRA)) {
    return false;
  }

  const bool extra = stage == StageId::EXTRA;
  auto ecl = LoadEmbeddedScript(extra ? 24 : stage_index);
  auto scl = LoadEmbeddedScript(extra ? 25 : stage_index + 6);
  auto map = data_->ExtractMap(extra ? 12 : stage_index);
  if (ecl.empty() || scl.empty() || !map) {
    return false;
  }

  enemies.ecl_head = ecl;
  enemies.scl_head = scl;
  enemies.scl_now = scl.data();
  scroller.scroll.DataHead = std::move(map);
  if (!scroller.Init()) {
    enemies.scl_now = nullptr;
    enemies.ecl_head = {};
    enemies.scl_head = {};
    scroller.scroll.DataHead = nullptr;
    return false;
  }

  game.count = 0;
  anime_data::SetupStageAnime(stage);
  return true;
}

bool StageLoader::LoadEnding(EnemyManager &enemies, ScrollManager &scroller,
                             GameManager &game) const {
  auto scl = LoadEmbeddedScript(47);
  if (scl.empty()) {
    return false;
  }
  enemies.ecl_head = {};
  enemies.scl_head = scl;
  enemies.scl_now = scl.data();
  scroller.scroll.DataHead = nullptr;
  game.count = 0;
  return true;
}

} // namespace stage
