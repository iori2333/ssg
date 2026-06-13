/*
 *   EffectManager — centralized visual effects system state
 */

#pragma once

#include "BOMBEFC.h"
#include "EFFECT.h"
#include "EFFECT3D.h"
#include "FRAGMENT.h"
#include <array>
#include <cstdint>
#include <string_view>

// EFFECT3D constants (moved from EFFECT3D.cpp)
inline constexpr auto CIRCLE_MAX = 40;
inline constexpr auto CUBE_MAX = 8;
inline constexpr auto STAR_MAX = 40;
inline constexpr auto ROCK_MAX = 28;
inline constexpr auto FAKE_ECLSTR_MAX = 80;
inline constexpr auto S6RASTER_MAX = 28;
inline constexpr auto S6STAR_MAX = 60;
inline constexpr auto S3STAR_MAX = 180;

// Stg6 types (moved from EFFECT3D.cpp)
typedef struct { int x, y; char vy; uint8_t type; uint8_t deg; uint8_t amp; } Stg6Raster;
typedef struct { int x, y; int vy; } Stg6Star;

struct EffectManager {
  // EFFECT.cpp
  std::array<SEFFECT_DATA, SEFFECT_MAX> string_effects;
  std::array<CIRCLE_EFC_DATA, CIRCLE_EFC_MAX> circle_effects;
  std::array<LOCKON_INFO, LOCKON_MAX> lock_info;
  SCREENEFC_INFO screen_info;
  unsigned int mtitle_rect = 0;  // TEXTRENDER_RECT_ID
  Narrow::string_view mtitle_strs[2] = {"♪ "};
  bool enable_warn_efc = false;
  uint16_t warn_efc_time = 0;

  // FRAGMENT.cpp
  std::array<FRAGMENT_DATA, FRAGMENT_MAX> fragments;
  int fragment_ptr = 0;

  // BOMBEFC.cpp
  std::array<BombEfcCtrl, EXBOMB_MAX> bomb_effects;

  // EFFECT3D.cpp
  std::array<Circle3D, CIRCLE_MAX> circles;
  std::array<Cube3D, CUBE_MAX> cubes;
  std::array<Star2D, STAR_MAX> stars;
  std::array<Rock3D, ROCK_MAX> rocks;
  WFLine2D wf_line;
  std::array<FakeECLString, FAKE_ECLSTR_MAX> fake_ecl_strs;
  std::array<Stg6Raster, S6RASTER_MAX> s6_ras;
  std::array<Stg6Star, S3STAR_MAX> s6_stars;
};

extern EffectManager Effects;

// --- 後方互換用参照 ---
// EFFECT.cpp
extern std::array<SEFFECT_DATA, SEFFECT_MAX>& SEffect;
extern std::array<CIRCLE_EFC_DATA, CIRCLE_EFC_MAX>& CEffect;
extern std::array<LOCKON_INFO, LOCKON_MAX>& LockInfo;
extern SCREENEFC_INFO& ScreenInfo;
extern unsigned int& MTitleRect;
extern Narrow::string_view (&MTitleStrs)[2];
extern bool& bEnableWarnEfc;
extern uint16_t& WarnEfcTime;

// FRAGMENT.cpp
extern std::array<FRAGMENT_DATA, FRAGMENT_MAX>& Fragment;
extern int& FragmentPtr;

// BOMBEFC.cpp
extern std::array<BombEfcCtrl, EXBOMB_MAX>& BombEfc;

// EFFECT3D.cpp
extern std::array<Circle3D, CIRCLE_MAX>& Cir;
extern std::array<Cube3D, CUBE_MAX>& Cube;
extern std::array<Star2D, STAR_MAX>& Star;
extern std::array<Rock3D, ROCK_MAX>& Rock;
extern WFLine2D& WFLine;
extern std::array<FakeECLString, FAKE_ECLSTR_MAX>& FakeECLStr;
extern std::array<Stg6Raster, S6RASTER_MAX>& S6Ras;
extern std::array<Stg6Star, S3STAR_MAX>& S6Star;
