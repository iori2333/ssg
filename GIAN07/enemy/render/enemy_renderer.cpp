///
/// EnemyRenderer - presentation of shared enemy actors
///

#include <algorithm>

#include "enemy_renderer.h"

#include "core/gian.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

void EnemyRenderer::DrawActor(const EnemyActor &actor) const {
  if (actor.animation >= animations_->size()) {
    return;
  }

  constexpr auto surface = SURFACE_ID::ENEMY;
  const auto &animation = (*animations_)[actor.animation];
  const WORLD_POINT center = WORLD_POINT::FromWorld(actor.x, actor.y);
  const auto topleft = center.ToPixel(animation.size);

  const auto frame = animation.mode == ANM_DEG
                         ? static_cast<uint8_t>(actor.d - 64 + 8) >> 4
                         : actor.animation_frame;
  if (frame >= ENEMY_ANIMATION_FRAME_MAX ||
      !GrpSurface_Blit({topleft.x, topleft.y}, surface, animation.ptn[frame])) {
    return;
  }

  if (actor.animation == actor.damage_animation || actor.damage_flash == 0 ||
      actor.damage_animation >= animations_->size()) {
    return;
  }

  const auto &damage_animation = (*animations_)[actor.damage_animation];
  const auto damage_topleft = center.ToPixel(damage_animation.size);
  GrpSurface_Blit({damage_topleft.x, damage_topleft.y}, surface,
                  damage_animation.ptn[0]);
}

void EnemyRenderer::DrawRegular(
    const ObjectPool<EnemyActor, ENEMY_MAX> &actors) const {
  for (const auto &actor : actors) {
    if (actor.state == EnemyActorState::Exploding) {
      DrawExplosion(actor);
    } else if ((actor.flag & EF_DRAW) != 0) {
      DrawActor(actor);
    }
  }
}

void EnemyRenderer::DrawExplosion(const EnemyActor &actor) const {
  const auto frame = actor.count / ENEMY_BOMB_SPD;
  const PIXEL_LTRB source = {static_cast<PIXEL_COORD>(frame * 48), 296,
                             static_cast<PIXEL_COORD>((frame + 1) * 48), 344};
  const PIXEL_POINT center = WORLD_POINT::FromWorld(actor.x, actor.y).ToPixel();
  GrpSurface_Blit({center.x - 24, center.y - 24}, SURFACE_ID::SYSTEM, source);
}

void EnemyRenderer::DrawBosses(
    const ObjectPool<BossData, BOSS_MAX> &bosses,
    const std::array<BitFormation, BOSS_MAX> &formations) const {
  for (const auto &formation : formations) {
    DrawBossLinks(formation);
  }

  for (const auto &boss : bosses) {
    if (DrawBossSpecialState(boss)) {
      continue;
    }
    if ((boss.actor.flag & EF_DRAW) != 0) {
      DrawActor(boss.actor);
    }
  }
}

void EnemyRenderer::DrawBossLinks(const BitFormation &formation) const {
  const auto geometry = formation.LinkGeometry();
  if (geometry.count == 0) {
    return;
  }

  GrpGeom->Lock();
  GrpGeom->SetColor({4, 4, 5});
  for (std::size_t index = 0; index < geometry.count; ++index) {
    const auto &link = geometry.links[index];
    GrpGeom->DrawLine(link.from.x, link.from.y, link.to.x, link.to.y);
  }
  GrpGeom->Unlock();
}

bool EnemyRenderer::DrawBossSpecialState(const BossData &boss) const {
  constexpr auto surface = SURFACE_ID::ENEMY;
  const auto &actor = boss.actor;
  const auto center = WORLD_POINT::FromWorld(actor.x, actor.y).ToPixel();

  if (boss.state == BossState::BombSpirit && player_->IsBombActive() != 0U &&
      (actor.flag & EF_DRAW) != 0) {
    const PIXEL_LTRB spirit = PIXEL_LTWH{
        160 + (Cast::sign<int32_t>(actor.count / 2) % 4) * 40, 80, 40, 40};
    GrpBackend_SetClip(GRP_RES_RECT);
    GrpSurface_Blit({center.x - 20, center.y - 20}, surface, spirit);
    GrpBackend_SetClip({X_MIN, Y_MIN, X_MAX + 1, Y_MAX + 1});
    return true;
  }

  if (boss.state == BossState::BombShield && player_->IsBombActive() != 0U &&
      (actor.flag & EF_DRAW) != 0) {
    GrpGeom->Lock();
    for (uint8_t layer = 0; layer <= 5; ++layer) {
      GrpGeom->SetColor({5U - layer, 5U - layer, 5U});
      GeomCircle({center.x, center.y},
                 sinl(actor.count * 4, 30 + layer * 4) + 80);
    }
    GrpGeom->Unlock();
  }

  switch (boss.state) {
  case BossState::ButterflyWings: {
    const auto offset =
        std::max((static_cast<int>(boss.state_frame) - 72) << 2, 0);
    GrpSurface_Blit({center.x - 64 - offset, center.y - 92}, surface,
                    {0, 176, 128, 360});
    GrpSurface_Blit({center.x - 64 + offset, center.y - 92}, surface,
                    {128, 176, 256, 360});
    break;
  }
  case BossState::BirdWings:
    GrpSurface_Blit({center.x - 94, center.y - 52}, surface,
                    {552, 0, 640, 104});
    GrpSurface_Blit({center.x + 6, center.y - 52}, surface,
                    {552, 104, 640, 208});
    break;
  case BossState::Normal:
  case BossState::BombShield:
  case BossState::BombSpirit:
    break;
  }
  return false;
}
