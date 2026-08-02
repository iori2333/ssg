/// Application-lifetime game systems and gameplay runtime ownership.

#pragma once

#include <functional>
#include <utility>

#include "display_controller.h"

#include "audio/audio_system.h"
#include "bullet/bullet_manager.h"
#include "data/game_data.h"
#include "data/graphics_loader.h"
#include "data/sfx_loader.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_session.h"
#include "i18n/localization.h"
#include "item/item_system.h"
#include "music/music_player.h"
#include "player/player.h"
#include "record/record_system.h"
#include "settings/config.h"
#include "stage/stage_loader.h"
#include "stage/stage_session.h"
#include "ui/ui_manager.h"

struct GameContext {
  explicit GameContext(ConfigData &config, std::function<void()> save_config)
      : config(config), save_config(std::move(save_config)) {}

  ConfigData &config;
  std::function<void()> save_config;

  i18n::Localization localization;
  data::GameData data;
  audio::AudioSystem audio;
  data::GraphicsLoader graphics{data};
  DisplayController display{graphics};
  data::SfxLoader sound_effects{data, audio};
  MusicPlayer music{data, audio};
  stage::StageLoader stage_loader{data};
  RecordSystem records{data};
  stage::StageSession stage;

  EffectManager effects{audio};
  GameSession session;
  Player player{effects, session, stage, audio};
  ItemSystem items{player, effects, audio};
  BulletManager bullets{items, session, player, effects, audio};
  EnemyManager enemies{bullets, items, session, player, stage, effects, audio};
  UiManager ui{audio};
};
