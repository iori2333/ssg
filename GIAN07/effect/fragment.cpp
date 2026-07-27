///
/// Short-lived particle fragments.
///

#include <cstdint>

#include "effect_manager.h"
#include "effect_types.h"

#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/ut_math.h"

void EffectManager::ResetFragments() {
  for (auto &fragment : fragments_) {
    fragment.remaining = 0;
  }
  next_fragment_ = 0;
}

void EffectManager::SpawnFragment(int x, int y, FragmentKind kind) {
  auto &fragment = fragments_[next_fragment_];
  fragment = {.x = x, .y = y, .kind = kind};

  uint8_t angle = 0;
  int speed = 0;
  switch (kind) {
  case FragmentKind::Hit:
    angle = static_cast<uint8_t>(rnd());
    speed = 1_px + rnd() % 3_px;
    fragment.remaining = 24;
    fragment.velocity_x = cosl(angle, speed);
    fragment.velocity_y = sinl(angle, speed);
    break;
  case FragmentKind::Graze:
    angle = static_cast<uint8_t>(rnd());
    speed = 4_px + rnd() % 3_px;
    fragment.remaining = 24;
    fragment.velocity_x = cosl(angle, speed);
    fragment.velocity_y = sinl(angle, speed);
    break;
  case FragmentKind::Smoke:
    fragment.remaining = 24;
    break;
  case FragmentKind::SmallStar:
    angle = static_cast<uint8_t>(rnd());
    speed = 5_px + rnd() % 3_px;
    fragment.remaining = 64;
    fragment.velocity_x = cosl(angle, speed);
    fragment.velocity_y = sinl(angle, speed);
    break;
  case FragmentKind::LargeStar:
    angle = static_cast<uint8_t>(rnd());
    speed = 4_px + rnd() % 3_px;
    fragment.remaining = 64;
    fragment.velocity_x = cosl(angle, speed);
    fragment.velocity_y = sinl(angle, speed);
    break;
  case FragmentKind::RisingStar:
    angle = static_cast<uint8_t>(-112 + rnd() % 96);
    speed = 6_px + rnd() % 4_px;
    fragment.remaining = 64;
    fragment.velocity_x = cosl(angle, speed);
    fragment.velocity_y = sinl(angle, speed);
    break;
  case FragmentKind::Heart:
    angle = static_cast<uint8_t>(rnd());
    speed = 2_px + rnd() % 5_px;
    fragment.remaining = 105;
    fragment.velocity_x = cosl(angle, speed);
    fragment.velocity_y = sinl(angle, speed);
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
      GrpSurface_Blit(
          {x - 4, y - 4}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{592 + ((24 - fragment.remaining) >> 2) * 8, 8, 8, 8});
      break;
    case FragmentKind::Hit:
      GrpSurface_Blit(
          {x - 4, y - 4}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{592 + ((24 - fragment.remaining) >> 2) * 8, 16, 8, 8});
      break;
    case FragmentKind::Smoke:
      GrpSurface_Blit(
          {x - 4, y - 4}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{592 + ((24 - fragment.remaining) >> 2) * 8, 0, 8, 8});
      break;
    case FragmentKind::SmallStar:
      GrpSurface_Blit({x - 8, y - 8}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{624, 432, 16, 16});
      break;
    case FragmentKind::LargeStar:
    case FragmentKind::RisingStar:
      GrpSurface_Blit({x - 16, y - 16}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{608, 448, 32, 32});
      break;
    case FragmentKind::Heart:
      GrpSurface_Blit({x - 16, y - 16}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{576, 448, 32, 32});
      break;
    case FragmentKind::ExpandingCircle:
      if (auto *geometry = GrpGeom_Poly()) {
        geometry->Lock();
        geometry->SetColor({4, 0, 0});
        geometry->SetAlphaOne();
        GeomFatCircleA(*geometry, {x, y}, (60 - fragment.remaining) * 6, 5);
        geometry->Unlock();
      }
      break;
    }
  }
}
