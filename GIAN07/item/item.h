///
/// Item - Item processing
///

#pragma once

#include <cstdint>

#include "gfx/coords.h"

// [ Constants ]

// Maximum count
inline constexpr auto ITEM_MAX = 100;

// Type or state
inline constexpr auto ITEM_DELETE = 0x00; // delete request
inline constexpr auto ITEM_SCORE = 0x01;  // score item
inline constexpr auto ITEM_EXTEND = 0x02; // remaining maid count up
inline constexpr auto ITEM_BOMB = 0x03;   // bomb

// Other
inline constexpr auto ITEM_GRAVITY = 3; // Y acceleration for items
inline constexpr auto ITEM_HIT_RADIUS = 16_px;
inline constexpr auto ITEM_HIT_RADIUS_LARGE = 28_px;

int GetItemHitRadius(uint8_t type);

// [ Struct ]
struct ItemData {
  int x, y;
  int vx, vy;
  uint32_t count;
  uint8_t type;
  bool auto_collect; // whether auto-collect is active
};
// (ITEM_DATA alias removed — use ItemData directly)
