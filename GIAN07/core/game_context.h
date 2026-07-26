///
/// GameContext — Centralized dependency holder (DI root)
///

#pragma once

#include <functional>

#include "config.h"
#include "game_manager.h"

#include "bullet/bullet_manager.h"
#include "data/game_data.h"
#include "data/graphics_loader.h"
#include "data/sfx_loader.h"
#include "enemy/enemy_system.h"
#include "gameflow/demo_manager.h"
#include "gameflow/ending_manager.h"
#include "gameflow/score_manager.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "stage/stage_loader.h"
#include "stage/stage_session.h"
#include "track_manager/track_manager.h"
#include "ui/ui_manager.h"

struct GameContext {
  data::GameData data;
  data::GraphicsLoader graphics{data};
  data::SfxLoader sound_effects{data};
  TrackManager tracks{data};
  stage::StageLoader stage_loader{data};
  stage::StageSession stage;

  BulletManager bullets;
  ItemManager items;
  GameManager game;
  Player player;
  EnemySystem enemies{bullets, items, game, player, stage};
  EndingManager ending;
  ScoreManager scores;
  DemoManager demos{data};
  UIManager ui;

  const GameConfig *game_cfg = nullptr;
  const GraphicsConfig *graphics_cfg = nullptr;
  const AudioConfig *audio_cfg = nullptr;
  const InputConfig *input_cfg = nullptr;
  const DebugConfig *debug_cfg = nullptr;

  std::function<void()> save_config;
  ConfigData *cfg = nullptr;
};
