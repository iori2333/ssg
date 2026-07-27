///
/// Expanding bomb explosion effect.
///

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "effect_manager.h"

#include "gfx/coords.h"
#include "gfx/graphics_backend.h"
#include "util/ut_math.h"

void EffectManager::ResetBombExplosions() {
  for (auto &effect : bomb_explosions_) {
    effect.active = false;
  }
}

void EffectManager::SpawnBombExplosion(int x, int y) {
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
        .frame = static_cast<uint8_t>(index % 14),
    };
  }
}

void EffectManager::DrawBombExplosion(const BombExplosion &effect) {
  for (const auto &particle : effect.particles) {
    if (particle.frame > 14) {
      continue;
    }
    GrpSurface_Blit({(particle.x >> 6) - 24, (particle.y >> 6) - 24},
                    SURFACE_ID::SYSTEM,
                    PIXEL_LTWH{(particle.frame >> 1) * 48, 296, 48, 48});
  }
}

void EffectManager::UpdateBombExplosion(BombExplosion &effect) {
  const int speed = sinl(effect.age / 2 - 64, 200_px) + 200_px;
  int spawned = 0;
  for (auto &particle : effect.particles) {
    if (particle.frame > 14) {
      if (effect.age > 192) {
        continue;
      }
      const uint8_t direction = static_cast<uint8_t>(rnd());
      const int velocity = rnd() % 256 + 128;
      particle.velocity_x = cosl(direction, velocity);
      particle.velocity_y = sinl(direction, velocity);

      const uint8_t angle =
          static_cast<uint8_t>(effect.age * 2 + (spawned % 8) * 32);
      const int radius = speed - rnd() % (speed >> 2);
      particle.x = cosl(angle, radius) + effect.x;
      particle.y = sinl(angle, radius) + effect.y;
      particle.frame = 0;
      ++spawned;
      continue;
    }

    ++particle.frame;
    particle.x += particle.velocity_x;
    particle.y += particle.velocity_y;
    particle.velocity_x -= particle.velocity_x >> 3;
    particle.velocity_y -= particle.velocity_y >> 3;
  }
}
