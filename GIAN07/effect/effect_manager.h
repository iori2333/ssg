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
  // ========================================================================
  // Data members
  // ========================================================================

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
  void SpawnStringEffect(int x, int y, const char* s);
  void SpawnPointEffect(int x, int y, uint32_t point);
  void SpawnGameOverEffect();
  void SetMusicTitle(int y, Narrow::string_view s);
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
  void LockOn(int* x, int* y, int wx64, int hx64);
  void MoveLockOn();
  void DrawLockOn();

  void InitWarningEffect();
  void SetWarningEffect();
  void MoveWarningEffect();
  void DrawWarningEffect();

  void CircleFadeOut(int x, int y, int r);

  void RenderMusicTitle(WINDOW_POINT topleft, const PIXEL_LTWH& subrect);

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
  void InitBombEffectSTD(BombEfcCtrl* p);
  void DrawBombEffectSTD(BombEfcCtrl* p);
  void MoveBombEffectSTD(BombEfcCtrl* p);
};

extern EffectManager Effects;

// ============================================================
// Backward-compat inline wrappers (was in EFFECT.h)
// ============================================================

inline void MTitleInit()        { Effects.InitMusicTitle(); }
inline void SEffectInit()       { Effects.InitStringEffects(); }
inline void StringEffect(int x, int y, const char* s) { Effects.SpawnStringEffect(x, y, s); }
inline void StringEffect2(int x, int y, uint32_t point) { Effects.SpawnPointEffect(x, y, point); }
inline void StringEffect3()     { Effects.SpawnGameOverEffect(); }
inline void SetMusicTitle(int y, Narrow::string_view s) { Effects.SetMusicTitle(y, s); }
inline void SEffectMove()       { Effects.MoveStringEffects(); }
inline void SEffectDraw()       { Effects.DrawStringEffects(); }

inline void CEffectInit()       { Effects.InitCircleEffects(); }
inline void CEffectMove()       { Effects.MoveCircleEffects(); }
inline void CEffectDraw()       { Effects.DrawCircleEffects(); }
inline void CEffectSet(int x, int y, uint8_t type) { Effects.SpawnCircleEffect(x, y, type); }

inline void ScreenEffectInit()  { Effects.InitScreenEffect(); }
inline void ScreenEffectSet(uint8_t cmd) { Effects.SetScreenEffect(cmd); }
inline void ScreenEffectMove()  { Effects.MoveScreenEffect(); }
inline void ScreenEffectDraw()  { Effects.DrawScreenEffect(); }

inline void WarningEffectInit() { Effects.InitWarningEffect(); }
inline void WarningEffectSet()  { Effects.SetWarningEffect(); }
inline void WarningEffectMove() { Effects.MoveWarningEffect(); }
inline void WarningEffectDraw() { Effects.DrawWarningEffect(); }

inline void ObjectLockOnInit()  { Effects.InitLockOn(); }
inline void ObjectLockOn(int* x, int* y, int wx64, int hx64) { Effects.LockOn(x, y, wx64, hx64); }
inline void ObjectLockMove()    { Effects.MoveLockOn(); }
inline void ObjectLockDraw()    { Effects.DrawLockOn(); }

inline void CircleFadeOut(int x, int y, int r) { Effects.CircleFadeOut(x, y, r); }

// FRAGMENT.cpp wrappers
inline void fragment_set(int x, int y, uint8_t cmd) { Effects.SpawnFragment(x, y, cmd); }
inline void fragment_move()  { Effects.MoveFragments(); }
inline void fragment_draw()  { Effects.DrawFragments(); }
inline void fragment_setup() { Effects.InitFragments(); }

// BOMBEFC.cpp wrappers
inline void ExBombEfcInit()                         { Effects.InitBombEffects(); }
inline void ExBombEfcSet(int x, int y, uint8_t t)   { Effects.SpawnBombEffect(x, y, t); }
inline void ExBombEfcDraw()                         { Effects.DrawBombEffects(); }
inline void ExBombEfcMove()                         { Effects.MoveBombEffects(); }

// EFFECT3D.cpp wrappers
inline void Init3DCube()           { Effects.Init3DCubes(); }
inline void Draw3DCube()           { Effects.Draw3DCubes(); }
inline void Move3DCube()           { Effects.Move3DCubes(); }
inline void InitEffectFakeECL()    { Effects.InitFakeECL(); }
inline void MoveEffectFakeECL()    { Effects.MoveFakeECL(); }
inline void DrawEffectFakeECL()    { Effects.DrawFakeECL(); }
inline void InitStg4Rock()         { Effects.InitStg4Rocks(); }
inline void MoveStg4Rock()         { Effects.MoveStg4Rocks(); }
inline void DrawStg4Rock()         { Effects.DrawStg4Rocks(); }
inline void SendCmdStg4Rock(uint8_t Cmd, uint8_t Param) { Effects.SendCmdStg4Rocks(Cmd, Param); }
inline void InitStg6Raster()       { Effects.InitStg6Rasters(); }
inline void MoveStg6Raster()       { Effects.MoveStg6Rasters(); }
inline void DrawStg6Raster()       { Effects.DrawStg6Rasters(); }
inline void InitStg3Star()         { Effects.InitStg3Stars(); }
inline void MoveStg3Star()         { Effects.MoveStg3Stars(); }
inline void DrawStg3Star()         { Effects.DrawStg3Stars(); }
inline void InitWarning()          { Effects.InitWarningText(); }
inline void DrawWarning()          { Effects.DrawWarningText(); }
inline void MoveWarning(uint8_t count) { Effects.MoveWarningText(count); }
