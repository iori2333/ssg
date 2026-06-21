///
/// Scroll - Scrolling processing
///

#pragma once

#include "game/coords.h"
#include "game/buffer.h"
#include <array>

// [Change history]
// 2000/04/01 : Added scroll command (STAGE2_BOSS)
// 2000/02/28 : Changed BOSS instruction to Level 2
// 2000/02/24 : Added midi instruction
// 2000/02/18 : Integrated with enemy placement processing
// 2000/02/01 : Development started
//

// [Constants]

// Constants shared with map editor
inline constexpr auto LAYER_MAX = 5;         // Layer depth
inline constexpr auto TIME_PER_FRAME = 20;   // Time advance per frame
inline constexpr auto MAP_WIDTH = 24;        // Map width
inline constexpr auto MAPDATA_NONE = 0xffff; // Nothing placed on map

// Scroll status
inline constexpr auto SST_NORMAL = 0x00; // Normal
inline constexpr auto SST_STOP = 0x01;   // Stopped

// Scroll commands
inline constexpr auto SCMD_QUAKE = 0x01;      // Shake screen
inline constexpr auto SCMD_STG2BOSS = 0x02;   // Stage 2 boss reverse scroll
inline constexpr auto SCMD_RASTER_ON = 0x03;  // Raster scroll start
inline constexpr auto SCMD_RASTER_OFF = 0x04; // Raster scroll end
inline constexpr auto SCMD_STG3BOSS =
    0x05; // Change background clouds to gates mode (?)
inline constexpr auto SCMD_STG3RESET = 0x06; // Return stage 3 background to normal mode
inline constexpr auto SCMD_STG6CUBE = 0x07;  // Stage 6 boss 3D cube mode
inline constexpr auto SCMD_STG6RNDECL =
    0x08;                                    // Stage 6 boss random fake ECL column arrangement
inline constexpr auto SCMD_STG4ROCK = 0x09;  // Stage 4 rock
inline constexpr auto SCMD_STG4LEAVE = 0x0a; // Eject stage 4 rocks off screen
inline constexpr auto SCMD_STG6RASTER = 0x0b; // Stage 6 raster
inline constexpr auto SCMD_STG3STAR = 0x0c;   // Stage 3 high-speed stars

// [Types]
using PBGMAP = uint16_t; // Type for map parts storage

// [Macros]

// [Structures]

// Scroll management structure
struct ScrollState {
//	GRP		lpMapOffs;					//
// Map parts data (Graphic) storage destination

  BYTE_BUFFER_OWNED DataHead; // Map data header

  PBGMAP *LayerHead[LAYER_MAX];  // Each layer header
  PBGMAP *LayerPtr[LAYER_MAX];   // Current layer pointer
  uint32_t LayerWait[LAYER_MAX]; // Layer weight
  int LayerCount[LAYER_MAX];     // Per-layer counter
  uint8_t LayerDy[LAYER_MAX];    // Layer offset in 1-dot units

  int NumLayer;      // Number of layers
  int ScrollSpeed;   // Scroll speed
  uint32_t Count;    // Current time
  uint32_t InfStart; // Infinite loop start time (default 0)
  uint32_t InfEnd;   // Infinite loop end time (default map length minimum)

  uint8_t State;   // Scroll status
  uint8_t IsQuake; // Whether shaking

  char RasterDx[31];   // Raster scroll X offset
  uint8_t RasterWidth; // Raster scroll amplitude
  uint8_t RasterDeg;   // Raster scroll angle

  void (*ExCmd)(void); // Special command
  uint32_t ExCount;    // Special command counter
};

// SCL management structure
struct SceneState {
  bool MsgFlag;    // Message skip flag
  bool ReturnFlag; // Return key flag
};

// Backward-compatible aliases
// (SCROLL_INFO alias removed — use ScrollState directly)
// (SCL_INFO alias removed — use SceneState directly)

// [Functions]
// Backward-compat inline wrapper moved to end of scroll_manager.h
// Implementation migrated to ScrollManager methods

// [Variables]
// Access directly via Scroller.scroll, Scroller.scene, Scroller.key_wait_count,
// Scroller.map_chip_rects
