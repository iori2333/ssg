///
/// Short-lived particle fragments.
///

#include <cstdint>

#include "effect_manager.h"
#include "effect_types.h"

#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

void EffectManager::ResetFragments() {
  for (auto &fragment : fragments_) {
    fragment.remaining = 0;
  }
  next_fragment_ = 0;
}

void EffectManager::SpawnFragment(int x, int y, FragmentKind kind) {
  auto &fragment = fragments_[next_fragment_];
  fragment = {.x = x, .y = y, .kind = kind};

  float angle = 0.0f;
  int speed = 0;
  switch (kind) {
  case FragmentKind::Hit:
    angle = math::RandomAngle();
    speed = 1_px + math::RandomInt() % 3_px;
    fragment.remaining = 24;
    {
      const auto velocity = math::RoundedPolarVector(angle, speed);
      fragment.velocity_x = velocity.x;
      fragment.velocity_y = velocity.y;
    }
    break;
  case FragmentKind::Graze:
    angle = math::RandomAngle();
    speed = 4_px + math::RandomInt() % 3_px;
    fragment.remaining = 24;
    {
      const auto velocity = math::RoundedPolarVector(angle, speed);
      fragment.velocity_x = velocity.x;
      fragment.velocity_y = velocity.y;
    }
    break;
  case FragmentKind::Smoke:
    fragment.remaining = 24;
    break;
  case FragmentKind::SmallStar:
    angle = math::RandomAngle();
    speed = 5_px + math::RandomInt() % 3_px;
    fragment.remaining = 64;
    {
      const auto velocity = math::RoundedPolarVector(angle, speed);
      fragment.velocity_x = velocity.x;
      fragment.velocity_y = velocity.y;
    }
    break;
  case FragmentKind::LargeStar:
    angle = math::RandomAngle();
    speed = 4_px + math::RandomInt() % 3_px;
    fragment.remaining = 64;
    {
      const auto velocity = math::RoundedPolarVector(angle, speed);
      fragment.velocity_x = velocity.x;
      fragment.velocity_y = velocity.y;
    }
    break;
  case FragmentKind::RisingStar:
    angle = static_cast<float>(-112 + math::RandomInt() % 96) *
            math::kLegacyAngleStep;
    speed = 6_px + math::RandomInt() % 4_px;
    fragment.remaining = 64;
    {
      const auto velocity = math::RoundedPolarVector(angle, speed);
      fragment.velocity_x = velocity.x;
      fragment.velocity_y = velocity.y;
    }
    break;
  case FragmentKind::Heart:
    angle = math::RandomAngle();
    speed = 2_px + math::RandomInt() % 5_px;
    fragment.remaining = 105;
    {
      const auto velocity = math::RoundedPolarVector(angle, speed);
      fragment.velocity_x = velocity.x;
      fragment.velocity_y = velocity.y;
    }
    break;
  case FragmentKind::ExpandingCircle:
    fragment.remaining = 60;
    break;
  }

  next_fragment_ = (next_fragment_ + 1) % fragments_.size();
}

void EffectManager::UpdateFragments() {
  for (auto &fragment : fragments_) {
    if (fragment.remaining == 0) {
      continue;
    }
    fragment.x += fragment.velocity_x;
    fragment.y += fragment.velocity_y;
    --fragment.remaining;
  }
}

void EffectManager::DrawFragments() const {
  for (const auto &fragment : fragments_) {
    if (fragment.remaining == 0) {
      continue;
    }

    const int x = fragment.x >> 6;
    const int y = fragment.y >> 6;
    switch (fragment.kind) {
    case FragmentKind::Graze:
      GraphicsSurfaceBlit(
          {x - 4, y - 4}, SurfaceId::System,
          PixelLtwh{592 + ((24 - fragment.remaining) >> 2) * 8, 8, 8, 8});
      break;
    case FragmentKind::Hit:
      GraphicsSurfaceBlit(
          {x - 4, y - 4}, SurfaceId::System,
          PixelLtwh{592 + ((24 - fragment.remaining) >> 2) * 8, 16, 8, 8});
      break;
    case FragmentKind::Smoke:
      GraphicsSurfaceBlit(
          {x - 4, y - 4}, SurfaceId::System,
          PixelLtwh{592 + ((24 - fragment.remaining) >> 2) * 8, 0, 8, 8});
      break;
    case FragmentKind::SmallStar:
      GraphicsSurfaceBlit({x - 8, y - 8}, SurfaceId::System,
                          PixelLtwh{624, 432, 16, 16});
      break;
    case FragmentKind::LargeStar:
    case FragmentKind::RisingStar:
      GraphicsSurfaceBlit({x - 16, y - 16}, SurfaceId::System,
                          PixelLtwh{608, 448, 32, 32});
      break;
    case FragmentKind::Heart:
      GraphicsSurfaceBlit({x - 16, y - 16}, SurfaceId::System,
                          PixelLtwh{576, 448, 32, 32});
      break;
    case FragmentKind::ExpandingCircle:
      Geometry().SetColor({4, 0, 0});
      Geometry().SetAlphaOne();
      GeomFatCircleA(Geometry(), {x, y}, (60 - fragment.remaining) * 6, 5);
      break;
    }
  }
}
