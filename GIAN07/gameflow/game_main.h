///
/// GameMain - Window system switching, etc.
///

#pragma once

#include <functional>

[[nodiscard]] bool
GameInit(std::function<void(bool &)> next_proc); // Initialize the game
void GameRestart(); // Restart the game (from ESC exit)
[[nodiscard]] bool GameExit(bool bNeedChgMusic = true); // Exit the game
void GameOverInit(); // Game over pre-processing
void GameContinue(); // Perform continue

[[nodiscard]] bool
GameReplayInitAll(const char *fn); // Initialize for multi-stage replay

[[nodiscard]] bool SProjectInit(); // Initialize West Project display

[[nodiscard]] bool GameExstgInit(); // Start extra stage

[[nodiscard]] bool GameNextStage(); // Move to next stage

void BulletGalleryInit();
