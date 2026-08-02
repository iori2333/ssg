///
/// GameRules - shared game mode values
///

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

enum class GameLevel : uint8_t {
  Easy = 0,
  Normal = 1,
  Hard = 2,
  Lunatic = 3,
  Extra = 4,
};

enum class PracticeMode : uint8_t {
  Off = 0,
  AutoBomb = 1,
  Invincible = 2,
};

enum class StageId : uint8_t {
  Stage1 = 0,
  Stage2,
  Stage3,
  Stage4,
  Stage5,
  Stage6,
  Extra,
};

inline constexpr std::array<std::string_view, 5> kGameLevelNames = {
    "Easy", "Normal", "Hard", "Lunatic", "Extra"};
inline constexpr std::array<std::string_view, 7> kStageNames = {
    "Stage 1", "Stage 2", "Stage 3", "Stage 4", "Stage 5", "Stage 6", "Extra"};

[[nodiscard]] constexpr std::string_view GameLevelName(GameLevel level) {
  const auto index = std::to_underlying(level);
  return index < kGameLevelNames.size() ? kGameLevelNames[index] : "Unknown";
}

[[nodiscard]] constexpr std::string_view StageName(StageId stage) {
  const auto index = std::to_underlying(stage);
  return index < kStageNames.size() ? kStageNames[index] : "Unknown";
}
