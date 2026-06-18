///
/// EffectManager - Centralized visual effects system state
///

#pragma once

#include "bomb_efc.h"
#include "effect.h"
#include "effect3d.h"
#include "fragment.h"
#include "game/text.h"
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
struct Stg6Raster {
  int x, y;
  char vy;
  uint8_t type;
  uint8_t deg;
  uint8_t amp;
};
struct Stg6Star {
  int x, y;
  int vy;
};

struct EffectManager {
  // ========================================================================
  // Data members
  // ========================================================================

  // EFFECT.cpp
  std::array<StringEffectData, SEFFECT_MAX> string_effects;
  std::array<CircleEffectData, CIRCLE_EFC_MAX> circle_effects;
  std::array<LockOnInfo, LOCKON_MAX> lock_info;
  ScreenEffectState screen_info;
  TEXTRENDER_RECT_ID mtitle_rect = {};
  std::string_view mtitle_strs[2] = {"♪ "};
  bool enable_warn_efc = false;
  uint16_t warn_efc_time = 0;

  // FRAGMENT.cpp
  std::array<FragmentData, FRAGMENT_MAX> fragments;
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

  // EFFECT3D.cpp — warning wireframe state (was file-static Warning[8])
  LineList3D warning_lines[8];

  // EFFECT3D.cpp — Move3DCube animation state (was static locals)
  uint16_t cube_anim_d = 0;
  uint16_t cube_anim_dx = 0;
  uint16_t cube_anim_dy = 0;
  uint16_t cube_anim_dz = 0;

  // ========================================================================
  // EFFECT.cpp methods (string/circle/screen/lockon/warning effects)
  // ========================================================================

  void InitMusicTitle();
  void InitStringEffects();
  void SpawnStringEffect(int x, int y, const char *s);
  void SpawnPointEffect(int x, int y, uint32_t point);
  void SpawnGameOverEffect();
  void SetMusicTitle(int y, std::string_view s);
  void MoveStringEffects();
  void DrawStringEffects();

  void InitCircleEffects();
  void MoveCircleEffects();
  void DrawCircleEffects();
  void SpawnCircleEffect(int x, int y, uint8_t type);

  void InitScreenEffect();
  void SetScreenEffect(uint8_t cmd);
  void MoveScreenEffect();
  void DrawScreenEffect();

  void InitLockOn();
  void LockOn(int *x, int *y, int wx64, int hx64);
  void MoveLockOn();
  void DrawLockOn();

  void InitWarningEffect();
  void SetWarningEffect();
  void MoveWarningEffect();
  void DrawWarningEffect();

  static void CircleFadeOut(int x, int y, int r);

  void RenderMusicTitle(WINDOW_POINT topleft, const PIXEL_LTWH &subrect);

  // ========================================================================
  // FRAGMENT.cpp methods
  // ========================================================================

  void InitFragments();
  void SpawnFragment(int x, int y, uint8_t cmd);
  void MoveFragments();
  void DrawFragments();

  // ========================================================================
  // BOMBEFC.cpp methods
  // ========================================================================

  void InitBombEffects();
  void SpawnBombEffect(int x, int y, uint8_t type);
  void DrawBombEffects();
  void MoveBombEffects();

  // ========================================================================
  // EFFECT3D.cpp methods (3D effects)
  // ========================================================================

  void Init3DCubes();
  void Draw3DCubes();
  void Move3DCubes();
  void InitFakeECL();
  void MoveFakeECL();
  void DrawFakeECL();
  void InitStg4Rocks();
  void MoveStg4Rocks();
  void DrawStg4Rocks();
  void SendCmdStg4Rocks(uint8_t cmd, uint8_t param);
  void InitStg6Rasters();
  void MoveStg6Rasters();
  void DrawStg6Rasters();
  void InitStg3Stars();
  void MoveStg3Stars();
  void DrawStg3Stars();

  // Warning wireframe 3D text
  void InitWarningText();
  void DrawWarningText();
  void MoveWarningText(uint8_t count);

private:
  // Internal helpers (BOMBEFC.cpp)
  static void InitBombEffectSTD(BombEfcCtrl *p);
  static void DrawBombEffectSTD(BombEfcCtrl *p);
  static void MoveBombEffectSTD(BombEfcCtrl *p);
};

extern EffectManager Effects;
