///
/// EffectManager - owns transient gameplay and overlay effects.
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "effect_types.h"

#include "gfx/coords.h"
#include "gfx/text.h"

class EffectManager {
public:
  void Reset();
  void ClearTextEffects() { ResetStrings(); }
  void InitializeTextRenderer();
  void Update();
  void UpdateGameOver();

  void SpawnString(int x, int y, std::string_view text);
  void SpawnPointValue(int x, int y, uint32_t points);
  void SpawnGameOver();
  void SetMusicTitle(int y, std::string_view title);
  void SpawnCircle(int x, int y, CircleEffectKind kind);
  void SpawnFragment(int x, int y, FragmentKind kind);
  void SpawnBombExplosion(int x, int y);
  void StartScreenTransition(ScreenTransition transition);
  void StartBossWarning();

  void DrawCircles() const;
  void DrawBombExplosions() const;
  void DrawFragments() const;
  void DrawForeground();
  void DrawScreenTransition() const;

private:
  static constexpr std::size_t kStringEffectCapacity = 1000;
  static constexpr std::size_t kCircleEffectCapacity = 10;
  static constexpr std::size_t kFragmentCapacity = 1000;
  static constexpr std::size_t kBombEffectCapacity = 3;
  static constexpr std::size_t kBombParticleCount = 200;

  enum class StringEffectState : uint8_t {
    Inactive,
    CharacterEntering,
    CharacterPaused,
    CharacterScattering,
    MusicTitleEntering,
    MusicTitleHolding,
    MusicTitleLeaving,
    GameOverEntering,
    GameOverHolding,
    PointValue,
  };

  struct StringEffect {
    int x = 0;
    int y = 0;
    int velocity_x = 0;
    int velocity_y = 0;
    uint32_t time = 0;
    uint32_t points = 0;
    StringEffectState state = StringEffectState::Inactive;
    char character = 0;
  };

  struct CircleEffect {
    int x = 0;
    int y = 0;
    int radius = 0;
    int end_radius = 0;
    uint32_t age = 0;
    CircleEffectKind kind = CircleEffectKind::Star;
    uint8_t angle = 0;
    bool active = false;
  };

  struct Fragment {
    int x = 0;
    int y = 0;
    int velocity_x = 0;
    int velocity_y = 0;
    uint8_t remaining = 0;
    FragmentKind kind = FragmentKind::Graze;
  };

  struct BombParticle {
    int x = 0;
    int y = 0;
    int velocity_x = 0;
    int velocity_y = 0;
    uint8_t frame = 0;
  };

  struct BombExplosion {
    int x = 0;
    int y = 0;
    uint32_t age = 0;
    std::array<BombParticle, kBombParticleCount> particles{};
    bool active = false;
  };

  struct ScreenEffect {
    ScreenTransition transition = ScreenTransition::CircleFadeIn;
    uint32_t age = 0;
    bool active = false;
  };

  struct WarningLine {
    PIXEL_POINT center{};
    std::span<WORLD_POINT> points;
    uint8_t angle_x = 0;
    uint8_t angle_y = 0;
    uint8_t angle_z = 0;
  };

  void ResetStrings();
  void ResetCircles();
  void ResetFragments();
  void ResetBombExplosions();
  void ResetScreenTransition();
  void ResetBossWarning();

  void UpdateStrings();
  void UpdateCircles();
  void UpdateFragments();
  void UpdateBombExplosions();
  void UpdateScreenTransition();
  void UpdateBossWarning();

  void DrawStrings();
  void DrawBossWarning();
  void RenderMusicTitle(WINDOW_POINT top_left, const PIXEL_LTWH &subrect);
  static void DrawCircleFade(int x, int y, int radius);
  static void InitializeBombExplosion(BombExplosion &effect);
  static void UpdateBombExplosion(BombExplosion &effect);
  static void DrawBombExplosion(const BombExplosion &effect);

  void InitializeWarningText();
  void UpdateWarningText(uint8_t age);
  void RotateWarningText(int amount);
  void DrawWarningText();

  std::array<StringEffect, kStringEffectCapacity> strings_{};
  std::array<CircleEffect, kCircleEffectCapacity> circles_{};
  std::array<Fragment, kFragmentCapacity> fragments_{};
  std::array<BombExplosion, kBombEffectCapacity> bomb_explosions_{};
  std::size_t next_fragment_ = 0;

  ScreenEffect screen_{};
  TEXTRENDER_RECT_ID music_title_rect_{};
  std::array<std::string_view, 2> music_title_text_{"\xE2\x99\xAA ", ""};

  std::array<WORLD_POINT, 11> warning_w_{};
  std::array<WORLD_POINT, 8> warning_a_outer_{};
  std::array<WORLD_POINT, 4> warning_a_inner_{};
  std::array<WORLD_POINT, 14> warning_r_{};
  std::array<WORLD_POINT, 9> warning_n_left_{};
  std::array<WORLD_POINT, 5> warning_i_{};
  std::array<WORLD_POINT, 9> warning_n_right_{};
  std::array<WORLD_POINT, 17> warning_g_{};
  std::array<WarningLine, 8> warning_lines_{};
  uint16_t warning_age_ = 0;
  mutable uint16_t warning_pulse_ = 0;
  bool warning_active_ = false;
};
