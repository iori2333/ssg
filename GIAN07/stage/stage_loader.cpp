///
/// StageLoader - installs validated stage assets into a game session
///
#include <cstddef>
#include <utility>

#include "anime_data.h"
#include "scene_program.h"
#include "stage_loader.h"
#include "stage_session.h"

#include "enemy/enemy_manager.h"
#include "scripts_data.h"
#include "sys/log.h"

namespace {

std::span<const uint8_t> LoadEmbeddedScript(int file_no) {
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
                       StageSession &session) const {
  const auto stage_index = std::to_underlying(stage);
  if (stage_index > std::to_underlying(StageId::Extra)) {
    logging::Error(logging::Channel::Stage, "Invalid stage id: {}",
                   stage_index);
    return false;
  }

  const bool extra = stage == StageId::Extra;
  const auto ecl_index = extra ? 24 : stage_index;
  const auto scl_index = extra ? 25 : stage_index + 6;
  const auto map_index = extra ? 6 : stage_index;
  auto ecl = LoadEmbeddedScript(ecl_index);
  auto scl = LoadEmbeddedScript(scl_index);
  auto map = data_->ExtractMap(map_index);
  if (ecl.empty() || scl.empty() || map.empty()) {
    logging::Error(
        logging::Channel::Stage,
        "Missing stage assets: stage={} ecl={}({}) scl={}({}) map={}({})",
        stage_index + 1, ecl_index, ecl.empty() ? "missing" : "ok", scl_index,
        scl.empty() ? "missing" : "ok", map_index,
        map.empty() ? "missing" : "ok");
    return false;
  }

  auto enemy_program = EclProgram::Parse(ecl);
  if (!enemy_program) {
    logging::Error(logging::Channel::Stage,
                   "Failed to parse ECL {} for stage {}", ecl_index,
                   stage_index + 1);
    return false;
  }

  EnemyAnimationSet animations{};
  anime_data::SetupStageAnime(stage, animations);

  if (!session.Load(map, scl)) {
    logging::Error(logging::Channel::Stage,
                   "Failed to load MAP {} or SCL {} for stage {}", map_index,
                   scl_index, stage_index + 1);
    return false;
  }

  if (!enemies.InstallStageAssets(std::move(*enemy_program),
                                  std::move(animations))) {
    logging::Error(logging::Channel::Stage,
                   "Failed to install enemy assets for stage {}",
                   stage_index + 1);
    return false;
  }
  logging::Debug(logging::Channel::Stage, "Loaded stage {}", stage_index + 1);
  return true;
}

bool StageLoader::LoadEnding(SceneRunner &scene) const {
  auto scl = LoadEmbeddedScript(47);
  if (scl.empty() || !scene.Load(scl)) {
    logging::Error(logging::Channel::Stage, "Failed to load ending SCL 47");
    return false;
  }
  return true;
}

} // namespace stage
