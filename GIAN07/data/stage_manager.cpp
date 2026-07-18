///
/// StageManager — ECL/SCL/map script loading
///
#include "stage_manager.h"

#include <cassert>

#include "core/config.h"
#include "core/game_manager.h"
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

bool StageManager::LoadStageData(uint8_t stage_num) {
  Enemies.scl_now = nullptr;
  Enemies.ecl_head = {};
  Enemies.scl_head = {};
  Scroller.scroll.DataHead = nullptr;

  const auto &map_pack = packs.Map();

  if (stage_num == kGfxExStage) {
    if ((Enemies.ecl_head = LoadEmbeddedScript(24)).data() == nullptr ||
        (Enemies.scl_head = LoadEmbeddedScript(25)).data() == nullptr ||
        (Scroller.scroll.DataHead = map_pack.Extract(12)) == nullptr) {
      return false;
    }
  } else if (stage_num == kGfxEnding) {
    if ((Enemies.scl_head = LoadEmbeddedScript(47)).data() == nullptr) {
      return false;
    }
    Enemies.scl_now = Enemies.scl_head.data();
    Games.game_count = 0;
    return true;
  } else {
    if ((stage_num < 1) || (stage_num > STAGE_MAX)) {
      return false;
    }
    if ((Enemies.ecl_head = LoadEmbeddedScript(stage_num - 1)).data() ==
            nullptr ||
        (Enemies.scl_head = LoadEmbeddedScript(stage_num + 5)).data() ==
            nullptr ||
        (Scroller.scroll.DataHead = map_pack.Extract(stage_num - 1)) ==
            nullptr) {
      return false;
    }
  }

  if (!Scroller.Init()) {
    return false;
  }

  Enemies.scl_now = Enemies.scl_head.data();
  Games.game_count = 0;

  anime_data::SetupStageAnime(stage_num);

  return true;
}

BYTE_BUFFER_OWNED StageManager::LoadDemo(int stage_num) {
  return packs.Map().Extract(stage_num - 1 + 6);
}
