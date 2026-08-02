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
#include <string>
#include <string_view>
#include <vector>

#include "enemy/ecl/ecl.h"
#include "enemy/ecl/ecl_program.h"
#include "i18n/localization.h"
#include "scripts_data.h"
#include "stage/scene_program.h"
#include "stage/stage_map.h"

namespace {

constexpr std::array kSceneIds = {6, 7, 8, 9, 10, 11, 25, 47};
constexpr std::array kEnemyIds = {0, 1, 2, 3, 4, 5, 24};
constexpr std::array kMapIds = {0, 1, 2, 3, 4, 5, 6};

const EmbeddedScript *FindScript(int index) {
  for (size_t i = 0; i < embedded_script_count; ++i) {
    if (embedded_scripts[i].index == index) {
      return &embedded_scripts[i];
    }
  }
  return nullptr;
}

bool ValidateScenes() {
  constexpr std::array languages = {"ja", "en", "zh"};
  i18n::Localization localization;
  if (!localization.Initialize("ja")) {
    std::cerr << "invalid embedded message catalogs\n";
    return false;
  }
  const auto sample_id = i18n::TextIdFromKey("stage1_msg_000");
  for (const auto *const language : languages) {
    if (!localization.SetLanguage(language) ||
        !localization.Text(sample_id).contains('\n') ||
        localization.Lines(sample_id).size() != 4 ||
        localization.Lines(sample_id).front() != "[VIVIT]") {
      std::cerr << "invalid multiline message text for " << language << '\n';
      return false;
    }
  }
  size_t message_references = 0;
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
    for (const auto &instruction : program->Instructions()) {
      if (instruction.opcode != stage::SceneOpcode::MessageReference) {
        continue;
      }
      message_references++;
      for (size_t language = 0; language < localization.LanguageCount();
           ++language) {
        if (!localization.HasText(language, instruction.text_id)) {
          std::cerr << "missing message 0x" << std::hex << instruction.text_id
                    << std::dec << " in " << localization.LanguageAt(language)
                    << '\n';
          return false;
        }
      }
    }
  }
  std::cout << "validated " << kSceneIds.size() << " embedded SCL programs and "
            << message_references << " localized message references\n";
  return true;
}

bool ValidateUiCatalogs() {
  constexpr std::array languages = {"ja", "en", "zh"};
  constexpr std::array main_menu_titles = {"メインメニュー", "Main Menu",
                                           "主菜单"};
  i18n::Localization localization;
  if (!localization.Initialize("ja")) {
    std::cerr << "invalid embedded text catalogs\n";
    return false;
  }
  const auto title_id = i18n::TextIdFromKey("ui.menu.main.title");
  for (size_t i = 0; i < languages.size(); ++i) {
    if (!localization.SetLanguage(languages[i]) ||
        localization.Text(title_id) != main_menu_titles[i]) {
      std::cerr << "invalid UI text catalog for " << languages[i] << '\n';
      return false;
    }
  }
  std::cout << "validated " << languages.size()
            << " runtime UI text catalogs\n";
  return true;
}

bool ValidateMusicCatalogs() {
  constexpr std::array languages = {"ja", "en", "zh"};
  constexpr std::array track_one_titles = {
      "フォルスストロベリー", "False Strawberry", "False Strawberry"};
  constexpr size_t track_count = 20;
  i18n::Localization localization;
  if (!localization.Initialize("ja")) {
    std::cerr << "invalid embedded text catalogs\n";
    return false;
  }
  for (size_t language = 0; language < languages.size(); ++language) {
    if (!localization.SetLanguage(languages[language]) ||
        localization.MusicTitle(1) != track_one_titles[language] ||
        !localization.MusicComment(0).contains('\n')) {
      std::cerr << "invalid Music Room text catalog for " << languages[language]
                << '\n';
      return false;
    }
    for (size_t track = 0; track < track_count; ++track) {
      if (localization.MusicTitle(track).empty() ||
          localization.MusicComment(track).empty()) {
        std::cerr << "missing Music Room text for track " << track << " in "
                  << languages[language] << '\n';
        return false;
      }
    }
  }
  std::cout << "validated " << languages.size() << " Music Room catalogs and "
            << track_count << " tracks\n";
  return true;
}

bool ValidateEnemies() {
  for (const int index : kEnemyIds) {
    const auto *script = FindScript(index);
    if (script == nullptr) {
      std::cerr << "missing embedded ECL " << index << '\n';
      return false;
    }
    const auto program = EclProgram::Parse({script->data, script->size});
    if (!program || !program->Entry(0)) {
      std::cerr << "invalid embedded ECL " << index << '\n';
      return false;
    }
  }
  std::cout << "validated " << kEnemyIds.size() << " embedded ECL programs\n";
  return true;
}

bool ValidateEnemyDecoderGuards() {
  constexpr std::array<uint8_t, 9> minimal = {
      1,
      0,
      0,
      0, // one script
      8,
      0,
      0,
      0, // entry at first instruction
      static_cast<uint8_t>(EclOpcode::End),
  };
  if (!EclProgram::Parse(minimal)) {
    return false;
  }

  auto unknown_opcode = minimal;
  unknown_opcode.back() = 0xff;
  if (EclProgram::Parse(unknown_opcode)) {
    return false;
  }

  constexpr std::array<uint8_t, 13> invalid_jump = {
      1, 0, 0, 0, 8, 0, 0, 0, static_cast<uint8_t>(EclOpcode::Jump),
      9, 0, 0, 0, // middle of the jump instruction
  };
  if (EclProgram::Parse(invalid_jump)) {
    return false;
  }

  std::cout << "validated ECL opcode and branch guards\n";
  return true;
}

bool ValidateTimeline() {
  constexpr std::array<uint8_t, 5> legacy_message = {
      static_cast<uint8_t>(stage::SceneOpcode::Message), 'o', 'k', 0,
      static_cast<uint8_t>(stage::SceneOpcode::End)};
  const auto legacy_program = stage::SceneProgram::Parse(legacy_message);
  if (!legacy_program || legacy_program->Instructions().size() != 2 ||
      legacy_program->Instructions().front().text != "ok") {
    return false;
  }

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
                                   std::to_string(index % 10) + ".map");
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

namespace {
int MainImpl(int argc, char **argv) {
  if (!ValidateEnemies()) {
    return 1;
  }
  if (!ValidateEnemyDecoderGuards()) {
    std::cerr << "invalid ECL fixture was accepted\n";
    return 1;
  }
  if (!ValidateScenes()) {
    return 1;
  }
  if (!ValidateUiCatalogs()) {
    return 1;
  }
  if (!ValidateMusicCatalogs()) {
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
} // namespace

int main(int argc, char **argv) {
  try {
    return MainImpl(argc, argv);
  } catch (...) {
    return 1;
  }
}
