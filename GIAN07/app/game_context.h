/// Application-lifetime game systems and gameplay runtime ownership.

#pragma once

#include "display_controller.h"

#include "bullet/bullet_manager.h"
#include "data/game_data.h"
#include "data/graphics_loader.h"
#include "data/sfx_loader.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_session.h"
#include "item/item_system.h"
#include "music/music_player.h"
#include "player/player.h"
#include "record/record_system.h"
#include "settings/config.h"
#include "stage/stage_loader.h"
#include "stage/stage_session.h"
#include "ui/ui_manager.h"

struct GameContext {
  explicit GameContext(const ConfigData &config) : config(config) {}

  const ConfigData &config;

  data::GameData data;
  data::GraphicsLoader graphics{data};
  DisplayController display{graphics};
  data::SfxLoader sound_effects{data};
  MusicPlayer music{data};
  stage::StageLoader stage_loader{data};
  RecordSystem records{data};
  stage::StageSession stage;

  EffectManager effects;
  GameSession session;
  Player player{effects, session, stage};
  ItemSystem items{player, effects};
  BulletManager bullets{items, session, player, effects};
  EnemyManager enemies{bullets, items, session, player, stage, effects};
  UIManager ui;
};
