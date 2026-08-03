///
/// Text and circular gameplay effects.
///

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>

#include "effect_manager.h"
#include "effect_types.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "gameplay/playfield.h"
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/core/world_math.h"
#include "gfx/render/geometry.h"
#include "gfx/text/text_renderer.h"
#include "ui/bitmap_font.h"
#include "util/math_utils.h"

namespace {

constexpr auto kCharacterScatterGravity = WorldCoord::FromRaw(16);
constexpr auto kPointValueGravity = WorldCoord::FromRaw(3);

} // namespace

void EffectManager::ResetCircles() {
  for (auto &effect : circles_) {
    effect.active = false;
  }
}

void EffectManager::SpawnCircle(WorldCoord x, WorldCoord y,
                                CircleEffectKind kind) {
  if (kind == CircleEffectKind::None) {
    return;
  }
  const auto found = std::ranges::find(circles_, false, &CircleEffect::active);
  if (found == circles_.end()) {
    return;
  }

  *found = {.x = x.ToPixels(), .y = y.ToPixels(), .kind = kind, .active = true};
  switch (kind) {
  case CircleEffectKind::None:
    return;
  case CircleEffectKind::Star:
    audio_.PlaySfx(SfxId::Tamefast);
    found->radius = 400;
    break;
  case CircleEffectKind::Converging:
    found->radius = 650;
    break;
  case CircleEffectKind::Diverging:
    found->radius = 0;
    found->end_radius = 800;
    break;
  }
}

void EffectManager::UpdateCircles() {
  for (auto &effect : circles_) {
    if (!effect.active) {
      continue;
    }
    ++effect.age;
    switch (effect.kind) {
    case CircleEffectKind::None:
      effect.active = false;
      break;
    case CircleEffectKind::Star:
      effect.radius -= 3;
      effect.angle += 2;
      effect.active = effect.radius > 0;
      break;
    case CircleEffectKind::Converging:
      effect.radius -= 15;
      effect.active = effect.radius > 0;
      break;
    case CircleEffectKind::Diverging:
      effect.radius += 13;
      effect.active = effect.radius < effect.end_radius;
      break;
    }
  }
}

void EffectManager::DrawCircles() const {
  static constexpr std::array<uint8_t, 4> kAngleSpeeds = {0, 1, 3, 7};

  for (const auto &effect : circles_) {
    if (!effect.active) {
      continue;
    }
    switch (effect.kind) {
    case CircleEffectKind::None:
      break;
    case CircleEffectKind::Star: {
      const int age = effect.age;
      for (int layer = 0; layer < 4; ++layer) {
        const int radius = effect.radius - layer * 7;
        if (radius < 0) {
          continue;
        }
        geometry::SetColor({5U, static_cast<uint8_t>(layer + 2U),
                            static_cast<uint8_t>(layer + 2U)});
        for (int point = 0; point < 5; ++point) {
          const int angle =
              effect.angle + kAngleSpeeds[layer] * age / 10 + point * 256 / 5;
          const int next_angle = effect.angle + kAngleSpeeds[layer] * age / 10 +
                                 (point + 2) * 256 / 5;
          const auto start = math::RoundedPolarVector(
              static_cast<float>(angle) * math::kLegacyAngleStep, radius);
          const auto end = math::RoundedPolarVector(
              static_cast<float>(next_angle) * math::kLegacyAngleStep, radius);
          geometry::DrawLine(effect.x + start.x, effect.y + start.y,
                             effect.x + end.x, effect.y + end.y);
        }
      }
      break;
    }
    case CircleEffectKind::Converging:
    case CircleEffectKind::Diverging:
      for (int layer = 0; layer < 4; ++layer) {
        const int divisor =
            effect.kind == CircleEffectKind::Converging ? 8 : 12;
        const int radius =
            effect.radius - std::max(2, layer * effect.radius / divisor);
        if (radius < 0) {
          continue;
        }
        geometry::SetColor({5U, static_cast<uint8_t>(layer + 2U),
                            static_cast<uint8_t>(layer + 2U)});
        geometry::DrawCircle({effect.x, effect.y}, radius);
      }
      break;
    }
  }
}

void EffectManager::InitializeTextRenderer() {
  music_title_rect_ = TextRenderer().Register(
      {.x = playfield::kRight + 1 - playfield::kLeft, .y = 20});
}

void EffectManager::ResetStrings() {
  for (auto &effect : strings_) {
    effect.state = StringEffectState::Inactive;
  }
}

void EffectManager::SpawnString(int x, int y, std::string_view text) {
  std::size_t free_index = 0;
  for (std::size_t index = 0; index < text.size(); ++index) {
    while (free_index < strings_.size() &&
           strings_[free_index].state != StringEffectState::Inactive) {
      ++free_index;
    }
    if (free_index == strings_.size()) {
      return;
    }
    strings_[free_index] = {
        .x = WorldCoord::FromPixels(x + static_cast<int>(index << 4) + 512),
        .y = WorldCoord::FromPixels(y),
        .velocity_x = -20_px,
        .time = 26,
        .state = StringEffectState::CharacterEntering,
        .character = text[index],
    };
  }
}

void EffectManager::SpawnPointValue(WorldCoord x, WorldCoord y, int points) {
  const auto found = std::ranges::find(strings_, StringEffectState::Inactive,
                                       &StringEffect::state);
  if (found == strings_.end()) {
    return;
  }
  *found = {.x = x,
            .y = y,
            .velocity_y = -2.5_px,
            .time = 90,
            .points = points,
            .state = StringEffectState::PointValue};
}

void EffectManager::SpawnGameOver() {
  const auto found = std::ranges::find(strings_, StringEffectState::Inactive,
                                       &StringEffect::state);
  if (found != strings_.end()) {
    *found = {.x = playfield::kWorldCenterX,
              .y = playfield::kWorldCenterY - 100_px,
              .time = 85,
              .state = StringEffectState::GameOverEntering};
  }
}

void EffectManager::SetMusicTitle(int y, std::string_view title) {
  const auto found = std::ranges::find(strings_, StringEffectState::Inactive,
                                       &StringEffect::state);
  if (found == strings_.end()) {
    return;
  }

  music_title_text_[1] = title;
  PixelPoint extent{};
  for (const auto text : music_title_text_) {
    const auto text_extent = TextRender::TextExtent(FontId::Normal, text);
    extent.x += text_extent.x;
    extent.y = text_extent.y;
  }
  const int x = std::max(640 - 128 - 32 - extent.x, 128);
  *found = {.x = PixelToWorld(x),
            .y = PixelToWorld(y),
            .extent = extent,
            .time = 128,
            .state = StringEffectState::MusicTitleEntering};
}

void EffectManager::UpdateStrings() {
  for (auto &effect : strings_) {
    switch (effect.state) {
    case StringEffectState::CharacterEntering:
      effect.x += effect.velocity_x;
      effect.y += effect.velocity_y;
      if (effect.time == 0) {
        effect.state = StringEffectState::CharacterPaused;
        effect.time = 256;
      }
      break;
    case StringEffectState::CharacterPaused:
      if (effect.time == 0) {
        const auto angle = static_cast<uint8_t>(128 + math::RandomInt() % 128);
        effect.state = StringEffectState::CharacterScattering;
        effect.time = 64;
        const auto velocity =
            math::RoundedPolarVector(math::AngleFromLegacy(angle), 10_px);
        effect.velocity_x = velocity.x;
        effect.velocity_y = velocity.y;
      }
      break;
    case StringEffectState::CharacterScattering:
      effect.x += effect.velocity_x;
      effect.y += (effect.velocity_y += kCharacterScatterGravity);
      if (effect.time == 0) {
        effect.state = StringEffectState::Inactive;
      }
      break;
    case StringEffectState::PointValue:
      if (effect.time == 0) {
        effect.state = StringEffectState::Inactive;
      }
      effect.x += effect.velocity_x;
      effect.y += (effect.velocity_y += kPointValueGravity);
      break;
    case StringEffectState::GameOverEntering: {
      if (effect.time == 0) {
        effect.state = StringEffectState::GameOverHolding;
        effect.time = 35;
      }
      break;
    }
    case StringEffectState::MusicTitleEntering:
      if (effect.time == 0) {
        effect.state = StringEffectState::MusicTitleHolding;
        effect.time = 256;
      }
      break;
    case StringEffectState::MusicTitleHolding:
      if (effect.time == 0) {
        effect.state = StringEffectState::MusicTitleLeaving;
        effect.time = 128;
      }
      break;
    case StringEffectState::MusicTitleLeaving:
      effect.x += 1_px;
      if (effect.time == 0) {
        effect.state = StringEffectState::Inactive;
      }
      break;
    case StringEffectState::GameOverHolding:
    case StringEffectState::Inactive:
      break;
    }
    if (effect.state != StringEffectState::Inactive) {
      --effect.time;
    }
  }
}

void EffectManager::RenderMusicTitle(PixelPoint top_left, const Rect &subrect) {
  TextRenderer().Render(
      top_left, music_title_rect_, music_title_text_[1],
      [this](TextRenderSession &session) {
        const auto gradient = [](int y) -> uint8_t { return 255 + 8 - y * 8; };
        ui::DrawGradient(session, music_title_text_, FontId::Normal, true,
                         gradient);
      },
      subrect);
}

void EffectManager::DrawStrings() {
  static constexpr std::string_view kGameOver = "GAME OVER";

  for (const auto &effect : strings_) {
    switch (effect.state) {
    case StringEffectState::CharacterEntering:
    case StringEffectState::CharacterPaused:
    case StringEffectState::CharacterScattering:
      ui::DrawGlyph({effect.x.ToPixels(), effect.y.ToPixels()},
                    effect.character);
      break;
    case StringEffectState::PointValue: {
      const auto points = std::format("{}", effect.points);
      ui::DrawScore({effect.x.ToPixels(), effect.y.ToPixels()}, points);
      break;
    }
    case StringEffectState::GameOverEntering: {
      const int remaining = effect.time;
      for (int index = 0; index < 9; ++index) {
        const auto angle = static_cast<uint8_t>(effect.time * 3 + index * 26);
        const auto offset = math::RoundedPolarVector(
            math::AngleFromLegacy(angle), remaining * 4);
        const int x = effect.x.ToPixels() + offset.x;
        const int y = effect.y.ToPixels() + offset.y;
        ui::DrawGlyph({x, y}, kGameOver[index]);
      }
      break;
    }
    case StringEffectState::GameOverHolding: {
      const int remaining = effect.time;
      const int center_x = effect.x.ToPixels() + 8;
      const int center_y = effect.y.ToPixels() + 8;
      const int half_height = (35 - remaining) / 2;
      geometry::SetColor({0, 0, 0});
      geometry::SetAlphaNorm(static_cast<uint8_t>((35 - remaining) * 3));
      geometry::DrawBoxA(center_x - 170, center_y - half_height, center_x + 170,
                         center_y + half_height);
      for (int index = 0; index < 9; ++index) {
        ui::DrawGlyph({effect.x.ToPixels() + (index - 4) * (35 - remaining),
                       effect.y.ToPixels()},
                      kGameOver[index]);
      }
      break;
    }
    case StringEffectState::MusicTitleEntering:
    case StringEffectState::MusicTitleLeaving: {
      const auto phase = effect.state == StringEffectState::MusicTitleEntering
                             ? static_cast<uint8_t>(effect.time)
                             : static_cast<uint8_t>(128 - effect.time);
      const int amplitude =
          effect.state == StringEffectState::MusicTitleEntering ? 160 : 100;
      for (int column = 0; column < effect.extent.x; ++column) {
        const Rect source = Rect::FromLtwh(column, 0, 1, effect.extent.y);
        const int wave =
            math::RoundedPolarVector(math::AngleFromLegacy(phase), amplitude).y;
        const int y =
            effect.y.ToPixels() -
            math::RoundedPolarVector(static_cast<float>(phase + column) *
                                         math::kLegacyAngleStep,
                                     phase)
                .y;
        for (int duplicate = 0; duplicate < 2; ++duplicate) {
          const int x = effect.x.ToPixels() +
                        math::RoundedPolarVector(
                            static_cast<float>(phase + (column / 2)) *
                                math::kLegacyAngleStep,
                            wave)
                            .y +
                        column + duplicate;
          RenderMusicTitle({x, y}, source);
        }
      }
      break;
    }
    case StringEffectState::MusicTitleHolding: {
      geometry::SetColor({0, 0, 0});
      const auto alpha =
          math::RoundedPolarVector(static_cast<float>(effect.time - 32) *
                                       math::kLegacyAngleStep,
                                   80.0F)
              .y +
          80;
      geometry::SetAlphaNorm(alpha);
      for (int row = 0; row < 16; ++row) {
        const int inset =
            math::RoundedPolarVector(static_cast<float>(128 + row * 16) *
                                         math::kLegacyAngleStep,
                                     16.0F)
                .y;
        geometry::DrawBoxA(
            effect.x.ToPixels() + inset - 16, effect.y.ToPixels() + row,
            playfield::kRight - 16 - inset, effect.y.ToPixels() + row + 1);
      }
      RenderMusicTitle({effect.x.ToPixels(), effect.y.ToPixels()},
                       Rect::FromLtwh(0, 0, effect.extent.x, effect.extent.y));
      break;
    }
    case StringEffectState::Inactive:
      break;
    }
  }
}
