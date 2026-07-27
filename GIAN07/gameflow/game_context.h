///
/// GameContext - assembles the application-lifetime game systems
///

#pragma once

#include "demo_manager.h"
#include "ending_manager.h"
#include "score_manager.h"

#include "bullet/bullet_manager.h"
#include "data/game_data.h"
#include "data/graphics_loader.h"
#include "data/sfx_loader.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_session.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "settings/config.h"
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

  EffectManager effects;
  GameSession session;
  Player player{effects};
  ItemManager items{player, effects};
  BulletManager bullets{items, session, player, effects};
  EnemyManager enemies{bullets, items, session, player, stage, effects};
  EndingManager ending;
  ScoreManager scores;
  DemoManager demos{data};
  UIManager ui;

  ConfigData &config = AppConfig();
};
