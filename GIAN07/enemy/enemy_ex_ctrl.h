///
/// EnemyExCtrl.h   Special enemy processing
///

#pragma once

#include "boss.h"
#include "core/point.h"
#include "enemy.h"

///// [Revision history] /////

// 2000/12/13 : Bit functions stabilized (using too much time...)
// 2000/12/09 : Added bit functions
// 2000/04/06 : Started development

///// [ Constants ] /////
inline constexpr auto SNAKE_MAX = 4; // Maximum number of snake-type enemies
constexpr auto SNAKEYMOVE_POINTS_PER_ENEMY = 8;

inline constexpr auto BIT_MAX = 6;             // Maximum number of bits
inline constexpr auto BITCMD_STDMOVE = 0x00;   // Normal movement
inline constexpr auto BITCMD_CHGSPD = 0x01;    // Change rotation speed
inline constexpr auto BITCMD_SELECTATK = 0x02; // Change attack command
inline constexpr auto BITCMD_CHGRADIUS = 0x03; // Change radius
inline constexpr auto BITCMD_MOVTARGET =
    0x04; // Boomerang toward target
inline constexpr auto BITCMD_DISABLE = 0xff; // Bit not in use

inline constexpr auto BLASERCMD_OPEN = 0x00; // Open laser
inline constexpr auto BLASERCMD_CLOSE =
    0x01; // Close bit laser
inline constexpr auto BLASERCMD_CLOSEL = 0x02; // Transition to line state

inline constexpr auto BLASERCMD_TYPE_A = 0x03; // Emit unidirectional fixed-angle laser
inline constexpr auto BLASERCMD_TYPE_B =
    0x04; // Emit bidirectional synchronous angle-change laser
inline constexpr auto BLASERCMD_TYPE_C = 0x05;  // Synchronous angle n-point star laser
inline constexpr auto BLASERCMD_DISABLE = 0xff; // Nothing active

///// [Structs] /////

// Snake control structure
template <size_t Len> struct SNAKYMOVE_DATA {
  // Vertex buffer (ExDef.h)
  DegPoint PointBuffer[Len * SNAKEYMOVE_POINTS_PER_ENEMY];

  EnemyData *EnemyPtr[Len]; // Tail data array
  BossData *Parent;         // Parent (head data)
  size_t Head;              // Pointer to head position
  bool bIsUse;              // Whether this structure is in use

  constexpr static size_t Length() { return Len; }
};

struct BitParam {
  EnemyData *pEnemy; // Pointer to target enemy

  uint32_t BitHP; // Bit durability
  uint8_t BitID;  // Which position (0~) from reference angle
  uint8_t Angle;  // Current angle
  char Force;     // Current direction force being applied
};
// (BIT_PARAM alias removed — use BitParam directly)

struct BitData {
  BitParam Bit[BIT_MAX]; // Bit data pointers
  BossData *Parent;      // Parent data pointer

  int x, y; // Rotation center (usually Parent->x, Parent->y)
  int v;    // Speed during acceleration movement
  int a;    // Acceleration during acceleration movement

  uint8_t d;       // Direction angle during acceleration movement
  uint8_t NumBits; // Initial bit count

  int Length;      // Bit rotation radius
  int FinalLength; // Final target rotation radius

  char BitSpeed;      // Bit base rotation speed
  uint8_t State;      // This bit group state
  uint8_t LaserState; // Laser state

  uint16_t BaseAngle; // Bit base rotation angle

  bool bIsLaserEnable; // Whether laser is active
};
// (BIT_DATA alias removed — use BitData directly)

///// [ Functions ] /////
// Backward compatible inline wrapper moved to boss_manager.h
// Implementation moved to BossManager methods
