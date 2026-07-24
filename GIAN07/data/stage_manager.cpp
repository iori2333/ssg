///
/// StageManager — ECL/SCL/map script loading
///
#include "stage_manager.h"

#include <cassert>

#include "core/config.h"
#include "gameflow/gameflow_manager.h"
#include "enemy/enemy_manager.h"
#include "gfx_manager.h"
#include "pack_manager.h"
#include "scripts_data.h"
#include "stage/anime_data.h"
#include "stage/scroll_manager.h"

namespace {

BYTE_BUFFER_BORROWED LoadEmbeddedScript(int filno) {
  for (size_t i = 0; i < embedded_script_count; i++) {
    if (embedded_scripts[i].index == filno) {
      return BYTE_BUFFER_BORROWED(embedded_scripts[i].data,
                                  embedded_scripts[i].size);
    }
  }
  return {};
}

} // namespace

bool StageManager::LoadStageData(AssetId stage_num) {
  Enemies.scl_now = nullptr;
  Enemies.ecl_head = {};
  Enemies.scl_head = {};
  Scroller.scroll.DataHead = nullptr;

  const auto &map_pack = packs.Map();

  if (stage_num == AssetId::EXTRA) {
    if ((Enemies.ecl_head = LoadEmbeddedScript(24)).data() == nullptr ||
        (Enemies.scl_head = LoadEmbeddedScript(25)).data() == nullptr ||
        (Scroller.scroll.DataHead = map_pack.Extract(12)) == nullptr) {
      return false;
    }
  } else if (stage_num == AssetId::ENDING) {
    if ((Enemies.scl_head = LoadEmbeddedScript(47)).data() == nullptr) {
      return false;
    }
    Enemies.scl_now = Enemies.scl_head.data();
    GameFlow.ctx.game.count = 0;
    return true;
  } else {
    const auto stage_val = std::to_underlying(stage_num);
    if (stage_val >= STAGE_MAX) {
      return false;
    }
    if ((Enemies.ecl_head = LoadEmbeddedScript(stage_val)).data() ==
            nullptr ||
        (Enemies.scl_head = LoadEmbeddedScript(stage_val + 6)).data() ==
            nullptr ||
        (Scroller.scroll.DataHead = map_pack.Extract(stage_val)) ==
            nullptr) {
      return false;
    }
  }

  if (!Scroller.Init()) {
    return false;
  }

  Enemies.scl_now = Enemies.scl_head.data();
  GameFlow.ctx.game.count = 0;

  anime_data::SetupStageAnime(stage_num);

  return true;
}

BYTE_BUFFER_OWNED StageManager::LoadDemo(StageId stage) {
  return packs.Map().Extract(std::to_underlying(stage) + 6);
}
