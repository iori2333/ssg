/*
 *   GameManager — centralized game state (singleton)
 */

#include "game/game_manager.h"

GameManager &GameManager::Instance() {
  static GameManager instance;
  return instance;
}
