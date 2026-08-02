///
/// EnemyRenderer - presentation of shared enemy actors
///

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "enemy_renderer.h"

#include "enemy/actor/enemy_actor.h"
#include "enemy/boss/bit_formation.h"
#include "enemy/boss/boss.h"
#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/math_utils.h"
#include "util/object_pool.h"

void EnemyRenderer::DrawActor(const EnemyActor &actor) const {
  if (actor.animation >= animations_.size()) {
    return;
  }

  constexpr auto surface = SurfaceId::Enemy;
  const auto &animation = animations_[actor.animation];
  const WorldPoint center = WorldPoint::FromWorld(actor.x, actor.y);
  const auto topleft = center.ToPixel(animation.size);

  const auto frame = animation.mode == EnemyAnimationMode::Directional
                         ? static_cast<uint8_t>(actor.d - 64 + 8) >> 4
                         : actor.animation_frame;
  if (std::cmp_greater_equal(frame, kEnemyAnimationFrameCapacity) ||
      !GraphicsSurfaceBlit({topleft.x, topleft.y}, surface,
                           animation.ptn[frame])) {
    return;
  }

  if (actor.animation == actor.damage_animation || actor.damage_flash == 0 ||
      actor.damage_animation >= animations_.size()) {
    return;
  }

  const auto &damage_animation = animations_[actor.damage_animation];
  const auto damage_topleft = center.ToPixel(damage_animation.size);
  GraphicsSurfaceBlit({damage_topleft.x, damage_topleft.y}, surface,
                      damage_animation.ptn[0]);
}

void EnemyRenderer::DrawRegular(
    const util::ObjectPool<EnemyActor, kEnemyCapacity> &actors) const {
  for (const auto &actor : actors) {
    if (actor.state == EnemyActorState::Exploding) {
      DrawExplosion(actor);
    } else if (actor.HasFlag(EnemyActorFlags::Draw)) {
      DrawActor(actor);
    }
  }
}

void EnemyRenderer::DrawExplosion(const EnemyActor &actor) {
  const auto frame = actor.count / kEnemyExplosionSpeed;
  const PixelLtrb source = {static_cast<PixelCoord>(frame * 48), 296,
                            static_cast<PixelCoord>((frame + 1) * 48), 344};
  const PixelPoint center = WorldPoint::FromWorld(actor.x, actor.y).ToPixel();
  GraphicsSurfaceBlit({center.x - 24, center.y - 24}, SurfaceId::System,
                      source);
}

void EnemyRenderer::DrawBosses(
    const util::ObjectPool<BossActor, kBossCapacity> &bosses,
    const std::array<BitFormation, kBossCapacity> &formations) const {
  for (const auto &formation : formations) {
    DrawBossLinks(formation);
  }

  for (const auto &boss : bosses) {
    if (DrawBossSpecialState(boss)) {
      continue;
    }
    if (boss.HasFlag(EnemyActorFlags::Draw)) {
      DrawActor(boss);
    }
  }
}

void EnemyRenderer::DrawBossLinks(const BitFormation &formation) {
  const auto link_geometry = formation.LinkGeometry();
  if (link_geometry.count == 0) {
    return;
  }

  geometry::SetColor({4, 4, 5});
  for (std::size_t index = 0; index < link_geometry.count; ++index) {
    const auto &link = link_geometry.links[index];
    geometry::DrawLine(link.from.x, link.from.y, link.to.x, link.to.y);
  }
}

bool EnemyRenderer::DrawBossSpecialState(const BossActor &boss) const {
  constexpr auto surface = SurfaceId::Enemy;
  const auto center = WorldPoint::FromWorld(boss.x, boss.y).ToPixel();

  if (boss.mode == BossMode::BombSpirit &&
      static_cast<unsigned int>(player_.IsBombActive()) != 0U &&
      boss.HasFlag(EnemyActorFlags::Draw)) {
    const PixelLtrb spirit = PixelLtwh{
        160 + (boss.count / 2) % 4 * 40, 80, 40, 40};
    GraphicsBackendSetClip(kGameResolutionRect);
    GraphicsSurfaceBlit({center.x - 20, center.y - 20}, surface, spirit);
    GraphicsBackendSetClip({playfield::kLeft, playfield::kTop,
                            playfield::kRight + 1, playfield::kBottom + 1});
    return true;
  }

  if (boss.mode == BossMode::BombShield &&
      static_cast<unsigned int>(player_.IsBombActive()) != 0U &&
      boss.HasFlag(EnemyActorFlags::Draw)) {
    for (int layer = 0; layer <= 5; ++layer) {
      geometry::SetColor({static_cast<uint8_t>(5U - layer),
                          static_cast<uint8_t>(5U - layer), 5U});
      geometry::DrawCircle(
          {center.x, center.y},
          math::RoundedPolarVector(static_cast<float>(boss.count * 4) *
                                       math::kLegacyAngleStep,
                                   static_cast<float>(30 + layer * 4))
                  .y +
              80);
    }
  }

  switch (boss.mode) {
  case BossMode::ButterflyWings: {
    const auto offset = std::max((boss.mode_frame - 72) << 2, 0);
    GraphicsSurfaceBlit({center.x - 64 - offset, center.y - 92}, surface,
                        {0, 176, 128, 360});
    GraphicsSurfaceBlit({center.x - 64 + offset, center.y - 92}, surface,
                        {128, 176, 256, 360});
    break;
  }
  case BossMode::BirdWings:
    GraphicsSurfaceBlit({center.x - 94, center.y - 52}, surface,
                        {552, 0, 640, 104});
    GraphicsSurfaceBlit({center.x + 6, center.y - 52}, surface,
                        {552, 104, 640, 208});
    break;
  case BossMode::Normal:
  case BossMode::BombShield:
  case BossMode::BombSpirit:
    break;
  }
  return false;
}
