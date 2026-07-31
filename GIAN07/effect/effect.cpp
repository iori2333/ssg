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

#include "audio/sfx.h"
#include "gameplay/playfield.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "platform/text_backend.h"
#include "util/cast.h"
#include "util/math_utils.h"

void EffectManager::ResetCircles() {
  for (auto &effect : circles_) {
    effect.active = false;
  }
}

void EffectManager::SpawnCircle(int x, int y, CircleEffectKind kind) {
  if (kind == CircleEffectKind::None) {
    return;
  }
  const auto found = std::ranges::find(circles_, false, &CircleEffect::active);
  if (found == circles_.end()) {
    return;
  }

  *found = {.x = x >> 6, .y = y >> 6, .kind = kind, .active = true};
  switch (kind) {
  case CircleEffectKind::None:
    return;
  case CircleEffectKind::Star:
    PlaySfx(SfxId::Tamefast);
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

  GrpGeom->Lock();
  for (const auto &effect : circles_) {
    if (!effect.active) {
      continue;
    }
    switch (effect.kind) {
    case CircleEffectKind::None:
      break;
    case CircleEffectKind::Star: {
      const int age = static_cast<int>(effect.age);
      for (uint8_t layer = 0; layer < 4; ++layer) {
        const int radius = effect.radius - layer * 7;
        if (radius < 0) {
          continue;
        }
        GrpGeom->SetColor({5U, layer + 2U, layer + 2U});
        for (int point = 0; point < 5; ++point) {
          const int angle =
              effect.angle + kAngleSpeeds[layer] * age / 10 + point * 256 / 5;
          const int next_angle = effect.angle + kAngleSpeeds[layer] * age / 10 +
                                 (point + 2) * 256 / 5;
          const auto start = math::RoundedPolarVector(
              static_cast<float>(angle) * math::kLegacyAngleStep, radius);
          const auto end = math::RoundedPolarVector(
              static_cast<float>(next_angle) * math::kLegacyAngleStep, radius);
          GrpGeom->DrawLine(effect.x + start.x, effect.y + start.y,
                            effect.x + end.x, effect.y + end.y);
        }
      }
      break;
    }
    case CircleEffectKind::Converging:
    case CircleEffectKind::Diverging:
      for (uint8_t layer = 0; layer < 4; ++layer) {
        const int divisor =
            effect.kind == CircleEffectKind::Converging ? 8 : 12;
        const int radius =
            effect.radius - std::max(2, layer * effect.radius / divisor);
        if (radius < 0) {
          continue;
        }
        GrpGeom->SetColor({5U, layer + 2U, layer + 2U});
        GeomCircle({effect.x, effect.y}, radius);
      }
      break;
    }
  }
  GrpGeom->Unlock();
}

void EffectManager::InitializeTextRenderer() {
  music_title_rect_ = TextObj.Register(
      {.w = playfield::kRight + 1 - playfield::kLeft, .h = 20});
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
        .x = (x + static_cast<int>(index << 4) + 512) << 6,
        .y = y << 6,
        .velocity_x = -20_px,
        .time = 26,
        .state = StringEffectState::CharacterEntering,
        .character = text[index],
    };
  }
}

void EffectManager::SpawnPointValue(int x, int y, uint32_t points) {
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
  PIXEL_SIZE extent{};
  for (const auto text : music_title_text_) {
    const auto text_extent = TextObj.TextExtent(FONT_ID::NORMAL, text);
    extent.w += text_extent.w;
    extent.h = text_extent.h;
  }
  const int x = std::max(640 - 128 - 32 - extent.w, 128);
  *found = {.x = PixelToWorld(x),
            .y = PixelToWorld(y),
            .velocity_x = extent.w,
            .velocity_y = extent.h,
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
        const uint8_t angle =
            static_cast<uint8_t>(128 + math::RandomInt() % 128);
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
      effect.y += (effect.velocity_y += 16);
      if (effect.time == 0) {
        effect.state = StringEffectState::Inactive;
      }
      break;
    case StringEffectState::PointValue:
      if (effect.time == 0) {
        effect.state = StringEffectState::Inactive;
      }
      effect.x += effect.velocity_x;
      effect.y += (effect.velocity_y += 3);
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
      effect.x += 64;
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

void EffectManager::RenderMusicTitle(WINDOW_POINT top_left,
                                     const PIXEL_LTWH &subrect) {
  TextObj.Render(
      top_left, music_title_rect_, music_title_text_[1],
      [this](TEXTRENDER_SESSION &session) {
        const auto gradient = [](PIXEL_COORD y) -> uint8_t {
          return 255 + 8 - y * 8;
        };
        DrawGrdFont(session, music_title_text_, FONT_ID::NORMAL, true,
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
      GrpPutc(effect.x >> 6, effect.y >> 6, effect.character);
      break;
    case StringEffectState::PointValue: {
      const auto points = std::format("{}", effect.points);
      GrpPutScore(effect.x >> 6, effect.y >> 6, points.c_str());
      break;
    }
    case StringEffectState::GameOverEntering: {
      const int remaining = static_cast<int>(effect.time);
      for (int index = 0; index < 9; ++index) {
        const auto angle = Cast::down<uint8_t>(effect.time * 3 + index * 26);
        const auto offset = math::RoundedPolarVector(
            math::AngleFromLegacy(angle), remaining * 4);
        const int x = (effect.x >> 6) + offset.x;
        const int y = (effect.y >> 6) + offset.y;
        GrpPutc(x, y, kGameOver[index]);
      }
      break;
    }
    case StringEffectState::GameOverHolding: {
      const int remaining = static_cast<int>(effect.time);
      const int center_x = (effect.x >> 6) + 8;
      const int center_y = (effect.y >> 6) + 8;
      const int half_height = (35 - remaining) / 2;
      GrpGeom->Lock();
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->SetAlphaNorm(Cast::down_sign<uint8_t>((35 - remaining) * 3));
      GrpGeom->DrawBoxA(center_x - 170, center_y - half_height, center_x + 170,
                        center_y + half_height);
      GrpGeom->Unlock();
      for (int index = 0; index < 9; ++index) {
        GrpPutc((effect.x >> 6) + (index - 4) * (35 - remaining), effect.y >> 6,
                kGameOver[index]);
      }
      break;
    }
    case StringEffectState::MusicTitleEntering:
    case StringEffectState::MusicTitleLeaving: {
      const auto phase = effect.state == StringEffectState::MusicTitleEntering
                             ? Cast::down<uint8_t>(effect.time)
                             : Cast::down<uint8_t>(128 - effect.time);
      const int amplitude =
          effect.state == StringEffectState::MusicTitleEntering ? 160 : 100;
      for (int column = 0; column < effect.velocity_x; ++column) {
        const PIXEL_LTWH source = {column, 0, 1, effect.velocity_y};
        const int wave =
            math::RoundedPolarVector(math::AngleFromLegacy(phase), amplitude).y;
        const int y = (effect.y >> 6) - math::RoundedPolarVector(
                                            static_cast<float>(phase + column) *
                                                math::kLegacyAngleStep,
                                            phase)
                                            .y;
        for (int duplicate = 0; duplicate < 2; ++duplicate) {
          const int x =
              (effect.x >> 6) +
              math::RoundedPolarVector(static_cast<float>(phase + column / 2) *
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
      GrpGeom->Lock();
      GrpGeom->SetColor({0, 0, 0});
      const auto alpha =
          math::RoundedPolarVector(static_cast<float>(effect.time - 32) *
                                       math::kLegacyAngleStep,
                                   80.0f)
              .y +
          80;
      GrpGeom->SetAlphaNorm(alpha);
      for (int row = 0; row < 16; ++row) {
        const int inset =
            math::RoundedPolarVector(static_cast<float>(128 + row * 16) *
                                         math::kLegacyAngleStep,
                                     16.0f)
                .y;
        GrpGeom->DrawBoxA((effect.x >> 6) + inset - 16, (effect.y >> 6) + row,
                          playfield::kRight - 16 - inset,
                          (effect.y >> 6) + row + 1);
      }
      GrpGeom->Unlock();
      RenderMusicTitle({effect.x >> 6, effect.y >> 6},
                       {0, 0, effect.velocity_x, effect.velocity_y});
      break;
    }
    case StringEffectState::Inactive:
      break;
    }
  }
}
