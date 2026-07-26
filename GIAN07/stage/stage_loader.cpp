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
                       StageSession &session) const {
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

  auto enemy_program = EclProgram::Parse(ecl);
  if (!enemy_program) {
    return false;
  }

  EnemyAnimationSet animations{};
  anime_data::SetupStageAnime(stage, animations);

  if (!session.Load(std::move(map), scl)) {
    return false;
  }

  return enemies.InstallStageAssets(std::move(*enemy_program),
                                    std::move(animations));
}

bool StageLoader::LoadEnding(SceneRunner &scene) const {
  auto scl = LoadEmbeddedScript(47);
  return !scl.empty() && scene.Load(scl);
}

} // namespace stage
