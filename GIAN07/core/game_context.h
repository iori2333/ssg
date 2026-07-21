///
/// GameContext — Centralized dependency holder (DI root)
///

#pragma once

#include <functional>

#include "bullet/bullet_manager.h"
#include "config.h"
#include "game_manager.h"
#include "gameflow/demo_manager.h"
#include "gameflow/ending_manager.h"
#include "gameflow/score_manager.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "stage/menu/ui_manager.h"

struct GameContext {
  BulletManager bullets;
  ItemManager items;
  GameManager game;
  Player player;
  EndingManager ending;
  ScoreManager scores;
  DemoManager demos;
  UIManager ui;

  const GameConfig *game_cfg = nullptr;
  const GraphicsConfig *graphics_cfg = nullptr;
  const AudioConfig *audio_cfg = nullptr;
  const InputConfig *input_cfg = nullptr;
#ifdef PBG_DEBUG
  const DebugConfig *debug_cfg = nullptr;
#endif

  std::function<void()> save_config;
  ConfigData *cfg = nullptr;
};
