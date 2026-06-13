/*
 *   GameManager — centralized game state (singleton)
 */

#include "game_manager.h"

GameManager& GameManager::Instance() {
  static GameManager instance;
  return instance;
}
