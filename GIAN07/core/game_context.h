///
/// GameContext — Centralized dependency holder (DI root)
///

#pragma once

#include "bullet/bullet_manager.h"
#include "bullet/laser_manager.h"
#include "game_manager.h"
#include "item/item_manager.h"

struct GameContext {
  BulletManager bullets;
  LaserManager lasers;
  ItemManager items;
  GameManager game;
};
