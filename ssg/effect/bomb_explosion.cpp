///
/// Expanding bomb explosion effect.
///

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "effect_manager.h"

#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/core/world_math.h"
#include "gfx/graphics.h"
#include "util/math_utils.h"

void EffectManager::ResetBombExplosions() {
  for (auto &effect : bomb_explosions_) {
    effect.active = false;
  }
}

void EffectManager::SpawnBombExplosion(WorldCoord x, WorldCoord y) {
  const auto found =
      std::ranges::find(bomb_explosions_, false, &BombExplosion::active);
  if (found == bomb_explosions_.end()) {
    return;
  }
  *found = {.x = x, .y = y, .active = true};
  InitializeBombExplosion(*found);
}

void EffectManager::UpdateBombExplosions() {
  for (auto &effect : bomb_explosions_) {
    if (!effect.active) {
      continue;
    }
    ++effect.age;
    UpdateBombExplosion(effect);
    if (effect.age > 224) {
      effect.active = false;
    }
  }
}

void EffectManager::DrawBombExplosions() const {
  for (const auto &effect : bomb_explosions_) {
    if (effect.active) {
      DrawBombExplosion(effect);
    }
  }
}

void EffectManager::InitializeBombExplosion(BombExplosion &effect) {
  for (std::size_t index = 0; index < effect.particles.size(); ++index) {
    effect.particles[index] = {
        .x = effect.x,
        .y = effect.y,
        .frame = static_cast<int>(index % 14),
    };
  }
}

void EffectManager::DrawBombExplosion(const BombExplosion &effect) {
  for (const auto &particle : effect.particles) {
    if (particle.frame > 14) {
      continue;
    }
    GraphicsSurfaceBlit(
        {particle.x.ToPixels() - 24, particle.y.ToPixels() - 24},
        SurfaceId::System,
        Rect::FromLtwh((particle.frame >> 1) * 48, 296, 48, 48));
  }
}

void EffectManager::UpdateBombExplosion(BombExplosion &effect) {
  const auto speed_angle =
      (static_cast<float>(effect.age) / 2.0F - 64.0F) * math::kLegacyAngleStep;
  const WorldCoord speed =
      math::RoundedPolarVector(speed_angle, 200_px).y + 200_px;
  int spawned = 0;
  for (auto &particle : effect.particles) {
    if (particle.frame > 14) {
      if (effect.age > 192) {
        continue;
      }
      const auto direction = math::RandomAngle();
      const WorldCoord velocity =
          WorldCoord::FromRaw(math::RandomInt() % 256 + 128);
      const auto movement = math::RoundedPolarVector(direction, velocity);
      particle.velocity_x = movement.x;
      particle.velocity_y = movement.y;

      const auto angle =
          static_cast<float>(effect.age * 2 + (spawned % 8) * 32) *
          math::kLegacyAngleStep;
      const WorldCoord radius =
          speed - math::RandomWorldBelow(speed.DivPow2Floor(2));
      const auto offset = math::RoundedPolarVector(angle, radius);
      particle.x = offset.x + effect.x;
      particle.y = offset.y + effect.y;
      particle.frame = 0;
      ++spawned;
      continue;
    }

    ++particle.frame;
    particle.x += particle.velocity_x;
    particle.y += particle.velocity_y;
    particle.velocity_x -= particle.velocity_x.DivPow2Floor(3);
    particle.velocity_y -= particle.velocity_y.DivPow2Floor(3);
  }
}
