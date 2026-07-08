///
/// Scene - SCL definition file
///

#pragma once

// [Change history]

// 2000/03/14 : Added WAITEX, STAGECLEAR instructions
// 2000/02/28 : Changed BOSS instruction
// 2000/02/24 : Added midi-related functions
// 2000/02/18 : Started system update

// About special instruction specifications

// WAITEX <wait condition (BYTE)>,<option (DWORD)>
// The BOSSHP wait condition
// is mainly used for background effect changes, etc. (not used for state
// transitions)

#include <cstdint>

// [Constants]

// SCL instructions
inline constexpr uint8_t SCL_TIME = 0x00;  // Next event trigger time
inline constexpr uint8_t SCL_ENEMY = 0x01; // Enemy event
inline constexpr uint8_t SCL_SSP = 0x02;   // Scroll speed change
inline constexpr uint8_t SCL_EFC = 0x03;   // Effect set
inline constexpr uint8_t SCL_END = 0x04;   // SCL end
inline constexpr uint8_t SCL_BOSS =
    0x05; // Boss spawn (args: X(16),Y(16),BossID(8))

// SCL level 2 instructions
inline constexpr uint8_t SCL_MWOPEN = 0x06;  // Open message window
inline constexpr uint8_t SCL_MWCLOSE = 0x07; // Close message window
inline constexpr uint8_t SCL_MSG = 0x08;     // Output message
inline constexpr uint8_t SCL_KEY = 0x09;     // Wait for key input
inline constexpr uint8_t SCL_NPG = 0x0a;     // Change to new page
inline constexpr uint8_t SCL_FACE = 0x0b;    // Display face
inline constexpr uint8_t SCL_MUSIC = 0x0c;   // Load music data
inline constexpr uint8_t SCL_BOSSDEAD =
    0x0d; // Force boss destruction (timeout)
inline constexpr uint8_t SCL_LOADFACE =
    0x0e; // Load face graphic (args: SurfaceID(BYTE), FileNo(BYTE))
inline constexpr uint8_t SCL_WAITEX = 0x0f; // Stop SCL until a condition is met
inline constexpr uint8_t SCL_STAGECLEAR =
    0x10; // Stage is complete. Go to next stage!
inline constexpr uint8_t SCL_MAPPALETTE =
    0x11; // Initialize palette for map parts (For 8BitMode)
inline constexpr uint8_t SCL_GAMECLEAR =
    0x12; // Return to title (with name register)
inline constexpr uint8_t SCL_DELENEMY =
    0x13; // Force delete enemy (index array itself)
inline constexpr uint8_t SCL_ENEMYPALETTE = 0x14; // Switch to enemy palette
inline constexpr uint8_t SCL_STAFF = 0x15;        // Staff ID set
inline constexpr uint8_t SCL_EXTRACLEAR = 0x16;   // Extra stage clear

// EFC instruction arguments
inline constexpr uint8_t SEFC_WARN = 0x00;      // Warning sound start
inline constexpr uint8_t SEFC_WARNSTOP = 0x01;  // Warning sound stop
inline constexpr uint8_t SEFC_MUSICFADE = 0x02; // Music fade out (Level 2)
inline constexpr uint8_t SEFC_STG2BOSS =
    0x03; // Stage 2 boss scroll activation!
inline constexpr uint8_t SEFC_RASTERON =
    0x04; // Raster scroll start (usable for desert, undersea city, etc.)
inline constexpr uint8_t SEFC_RASTEROFF = 0x05; // Raster scroll end
inline constexpr uint8_t SEFC_CFADEIN = 0x06;   // Circular fade in
inline constexpr uint8_t SEFC_CFADEOUT = 0x07;  // Circular fade out
inline constexpr uint8_t SEFC_STG3BOSS = 0x08;  // Stage 3 boss clouds
inline constexpr uint8_t SEFC_STG3RESET = 0x09; // Stage 3 boss cloud reset
inline constexpr uint8_t SEFC_STG6CUBE = 0x0a;  // Stage 6 boss 3D cube
inline constexpr uint8_t SEFC_STG6RNDECL =
    0x0b; // Stage 6 boss fake ECL arrangement
inline constexpr uint8_t SEFC_STG4ROCK = 0x0c; // Stage 4 rock
inline constexpr uint8_t SEFC_STG4LEAVE =
    0x0d;                                      // Eject stage 4 rocks off screen
inline constexpr uint8_t SEFC_WHITEIN = 0x0e;  // White in
inline constexpr uint8_t SEFC_WHITEOUT = 0x0f; // White out
inline constexpr uint8_t SEFC_LOADEX01 = 0x10; // Load extra boss 1 image
inline constexpr uint8_t SEFC_LOADEX02 = 0x11; // Load extra boss 2 image
inline constexpr uint8_t SEFC_STG6RASTER = 0x12; // Stage 6 raster

// WAITEX instruction arguments (Level 2)
inline constexpr uint8_t SWAIT_BOSSLEFT =
    0x00; // Boss remaining count (OPT: boss count)
inline constexpr uint8_t SWAIT_BOSSHP =
    0x01; // Boss total HP below specified value (OPT: remaining HP)
