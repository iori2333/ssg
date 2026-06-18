///
/// Item - Item processing
///

#pragma once

#include <array>
#include <cstdint>

// [ Constants ]

// Maximum count
inline constexpr auto ITEM_MAX = 100;

// Type or state
inline constexpr auto ITEM_DELETE = 0x00; // delete request
inline constexpr auto ITEM_SCORE = 0x01;  // score item
inline constexpr auto ITEM_EXTEND = 0x02; // remaining maid count up
inline constexpr auto ITEM_BOMB = 0x03;   // bomb

// Other
inline constexpr auto ITEM_GRAVITY = 3;          // Y acceleration for items
inline constexpr auto ITEM_HITX = (8 + 8) * 64;  // item X collision
inline constexpr auto ITEM_HITY = (16 + 8) * 64; // item Y collision

// [ Struct ]
struct ItemData {
  int x, y;
  int vx, vy;
  uint32_t count;
  uint8_t type;
  bool auto_collect; // whether auto-collect is active
};
// (ITEM_DATA alias removed — use ItemData directly)

// [ Functions ]
// Migrated to ItemManager

// [ Variables ]
// Access directly via Items.entities, Items.indices, Items.count
