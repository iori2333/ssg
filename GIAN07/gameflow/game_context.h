///
/// GameContext - assembles the application-lifetime game systems
///

#pragma once

#include "ending_scene.h"
#include "music_room_scene.h"
#include "score_scene.h"

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
#include "replay/replay_system.h"
#include "settings/config.h"
#include "stage/stage_loader.h"
#include "stage/stage_session.h"
#include "ui/ui_manager.h"

struct GameContext {
  data::GameData data;
  data::GraphicsLoader graphics{data};
  data::SfxLoader sound_effects{data};
  MusicPlayer music{data};
  stage::StageLoader stage_loader{data};
  stage::StageSession stage;

  EffectManager effects;
  GameSession session;
  Player player{effects};
  ItemSystem items{player, effects};
  BulletManager bullets{items, session, player, effects};
  EnemyManager enemies{bullets, items, session, player, stage, effects};
  EndingScene ending;
  MusicRoomScene music_room;
  ScoreScene score;
  ReplaySystem replay{data};
  UIManager ui;

  ConfigData &config = AppConfig();
};
