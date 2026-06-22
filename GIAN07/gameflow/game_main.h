///
/// GameMain - Window system switching, etc.
///

#pragma once

// [Changelog]

// 2000/02/03 : Development started

// [Include Files]
#include <functional>

#include "ending.h"

// [Constants]
// [Macros]
// [Structs]

// [Global Variables]
// IsDemoplay → redeclared as extern bool& in game_manager.h
// GameMain, DemoTimer, DrawCount, WeaponKeyWait, GameOverTimer,
// CurrentName, CurrentRank, CurrentDif, VivTemp, InputLocked, FlashState
// → declared as references in gameflow_manager.h

// [Functions]

// WeaponSelectInit → inline wrapper in gameflow_manager.h
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

// NameRegistInit → inline wrapper in gameflow_manager.h
[[nodiscard]] bool ScoreNameInit(); // Name registration screen

[[nodiscard]] bool GameNextStage(); // Move to next stage

#ifdef PBG_DEBUG
void BulletGalleryInit();
#endif
