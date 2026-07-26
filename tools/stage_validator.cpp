///
/// stage_validator - validates embedded SCL and optional extracted stage maps
///
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

#include "scripts_data.h"
#include "stage/scene_program.h"
#include "stage/stage_map.h"

namespace {

constexpr std::array kSceneIds = {6, 7, 8, 9, 10, 11, 25, 47};
constexpr std::array kMapIds = {0, 1, 2, 3, 4, 5, 12};

const EmbeddedScript *FindScript(int index) {
  for (size_t i = 0; i < embedded_script_count; ++i) {
    if (embedded_scripts[i].index == index) {
      return &embedded_scripts[i];
    }
  }
  return nullptr;
}

bool ValidateScenes() {
  for (const int index : kSceneIds) {
    const auto *script = FindScript(index);
    if (script == nullptr) {
      std::cerr << "missing embedded SCL " << index << '\n';
      return false;
    }
    const auto program =
        stage::SceneProgram::Parse({script->data, script->size});
    if (!program || program->Instructions().empty()) {
      std::cerr << "invalid embedded SCL " << index << '\n';
      return false;
    }
  }
  std::cout << "validated " << kSceneIds.size() << " embedded SCL programs\n";
  return true;
}

bool ValidateTimeline() {
  constexpr std::array<uint8_t, 7> script = {
      0x00, 0x02, 0x00, 0x00, 0x00, // TIME 2
      0x10,                         // STAGECLEAR
      0x04,                         // END
  };
  stage::SceneRunner runner;
  if (!runner.Load(script) || runner.TimeReady(2, false)) {
    return false;
  }
  runner.AdvanceFrame();
  if (runner.TimeReady(2, false)) {
    return false;
  }
  runner.AdvanceFrame();
  if (!runner.TimeReady(2, false) || runner.Frame() != 2) {
    return false;
  }

  runner.SetMessageActive(true);
  runner.SetFrame(0);
  if (!runner.TimeReady(10, true) || runner.Frame() != 10) {
    return false;
  }

  runner.SetMessageActive(false);
  for (int frame = 0; frame < 179; ++frame) {
    if (runner.KeyReady(false)) {
      return false;
    }
  }
  if (!runner.KeyReady(false)) {
    return false;
  }
  std::cout << "validated scene frame and key-wait boundaries\n";
  return true;
}

bool ValidateMaps(const std::filesystem::path &directory) {
  for (const int index : kMapIds) {
    const auto path = directory / (std::to_string(index / 100) +
                                   std::to_string((index / 10) % 10) +
                                   std::to_string(index % 10) + ".bin");
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      std::cerr << "missing extracted map " << path << '\n';
      return false;
    }
    std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    const auto map = stage::StageMap::Parse(bytes);
    if (!map) {
      std::cerr << "invalid extracted map " << path << '\n';
      return false;
    }
  }
  std::cout << "validated " << kMapIds.size() << " extracted stage maps\n";
  return true;
}

} // namespace

int main(int argc, char **argv) {
  if (!ValidateScenes()) {
    return 1;
  }
  if (!ValidateTimeline()) {
    std::cerr << "invalid scene timeline behavior\n";
    return 1;
  }
  const std::array<uint8_t, 4> invalid_map{};
  if (stage::StageMap::Parse(invalid_map)) {
    std::cerr << "invalid map fixture was accepted\n";
    return 1;
  }
  if (argc > 1 && !ValidateMaps(argv[1])) {
    return 1;
  }
  return 0;
}
