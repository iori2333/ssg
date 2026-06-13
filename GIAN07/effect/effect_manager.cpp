/*
 *   EffectManager — centralized visual effects system state
 */

#include "effect_manager.h"

// --- グローバルインスタンス ---
EffectManager Effects;

// --- 後方互換用参照ラッパー ---
// EFFECT.cpp
std::array<SEFFECT_DATA, SEFFECT_MAX>& SEffect = Effects.string_effects;
std::array<CIRCLE_EFC_DATA, CIRCLE_EFC_MAX>& CEffect = Effects.circle_effects;
std::array<LOCKON_INFO, LOCKON_MAX>& LockInfo = Effects.lock_info;
SCREENEFC_INFO& ScreenInfo = Effects.screen_info;
unsigned int& MTitleRect = Effects.mtitle_rect;  // TEXTRENDER_RECT_ID
Narrow::string_view (&MTitleStrs)[2] = Effects.mtitle_strs;
bool& bEnableWarnEfc = Effects.enable_warn_efc;
uint16_t& WarnEfcTime = Effects.warn_efc_time;

// FRAGMENT.cpp
std::array<FRAGMENT_DATA, FRAGMENT_MAX>& Fragment = Effects.fragments;
int& FragmentPtr = Effects.fragment_ptr;

// BOMBEFC.cpp
std::array<BombEfcCtrl, EXBOMB_MAX>& BombEfc = Effects.bomb_effects;

// EFFECT3D.cpp
std::array<Circle3D, CIRCLE_MAX>& Cir = Effects.circles;
std::array<Cube3D, CUBE_MAX>& Cube = Effects.cubes;
std::array<Star2D, STAR_MAX>& Star = Effects.stars;
std::array<Rock3D, ROCK_MAX>& Rock = Effects.rocks;
WFLine2D& WFLine = Effects.wf_line;
std::array<FakeECLString, FAKE_ECLSTR_MAX>& FakeECLStr = Effects.fake_ecl_strs;
std::array<Stg6Raster, S6RASTER_MAX>& S6Ras = Effects.s6_ras;
std::array<Stg6Star, S3STAR_MAX>& S6Star = Effects.s6_stars;
