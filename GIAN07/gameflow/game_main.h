///
/// GameMain - Window system switching, etc.
///

#pragma once

#include <functional>

#include "gameplay/game_rules.h"

[[nodiscard]] bool
GameInit(std::function<void(bool &)> next_proc); // Initialize the game
void GameRestart(); // Restart the game (from ESC exit)
[[nodiscard]] bool GameExit(bool bNeedChgMusic = true); // Exit the game
void GameOverInit(); // Game over pre-processing
void GameContinue(); // Perform continue
void GameOverExit(bool save_replay);
void GameClearResults(bool extra_stage, bool change_music);

[[nodiscard]] bool GameReplayInit(const char *path, StageId stage);

[[nodiscard]] bool SProjectInit(); // Initialize West Project display

[[nodiscard]] bool GameExstgInit(); // Start extra stage

[[nodiscard]] bool GameNextStage(); // Move to next stage

void BulletGalleryInit();
