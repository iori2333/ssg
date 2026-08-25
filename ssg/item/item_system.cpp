///
/// ItemSystem - collectible item entities and pickup processing
///

#include <cmath>
#include <cstdint>

#include "item_system.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "effect/effect_manager.h"
#include "effect/effect_types.h"
#include "gameplay/playfield.h"
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/core/world_math.h"
#include "gfx/graphics.h"
#include "player/player.h"
#include "sys/log.h"
#include "util/math_utils.h"

namespace {

constexpr auto kCollectDistanceStep = WorldCoord::FromRaw(500);
constexpr auto kBombCollectDistanceStep = WorldCoord::FromRaw(700);

} // namespace

WorldCoord ItemSystem::HitRadius(ItemKind kind) {
  switch (kind) {
  case ItemKind::Bomb:
  case ItemKind::Extend:
    return kLargeItemHitRadius;
  default:
    return kItemHitRadius;
  }
}

void ItemSystem::Spawn(WorldCoord x, WorldCoord y, ItemKind kind) {
  if (kind == ItemKind::None) {
    return;
  }
  auto *ip = pool_.Alloc();
  if (ip == nullptr) {
    logging::Warning(logging::Channel::Gameplay,
                     "Item pool exhausted; item dropped");
    return;
  }

  constexpr auto angle = -math::kFullAngle / 4.0F;
  ip->x = x;
  ip->y = y;
  ip->kind = kind;
  ip->count = 0;
  ip->auto_collect = false;

  switch (kind) {
  case ItemKind::None:
    return;
  case ItemKind::Score: {
    const auto velocity = math::RoundedPolarVector(angle, 3_px);
    ip->vx = velocity.x;
    ip->vy = velocity.y;
  } break;

  case ItemKind::Extend: {
    const auto velocity = math::RoundedPolarVector(angle, 2_px);
    ip->vx = velocity.x;
    ip->vy = velocity.y;
  } break;

  case ItemKind::Bomb: {
    const auto velocity = math::RoundedPolarVector(angle, 2_px);
    ip->vx = velocity.x;
    ip->vy = velocity.y;
  } break;
  }
}

void ItemSystem::Update() {
  WorldCoord tx{};
  WorldCoord ty{};
  int l = 0;

  const int point =
      (((playfield::kWorldBottom - 10_px - player_.Y()).ToPixels() +
        (player_.GrazeCount() * 4)) *
       160);

  for (auto &ip : pool_) {
    if (!player_.IsBombActive()) {
      if (player_.Y() < kStarCollectLine ||
          player_.GrazeWaitTime() > kStarCollectGrazeWait || ip.auto_collect) {
        // Player above collect line or auto-collect already active
        ip.auto_collect = true;
        tx = (player_.X() - ip.x);
        ty = (player_.Y() - ip.y);
        const WorldCoord distance = math::RoundedLength({tx, ty});
        l = 1 + distance.Ratio(kCollectDistanceStep);
        ip.x += tx / l;
        ip.y += ty / l;
      } else {
        ip.x += ip.vx;
        ip.y += ip.vy;
      }
    } else {
      tx = (player_.X() - ip.x);
      ty = (player_.Y() - ip.y);
      const WorldCoord distance = math::RoundedLength({tx, ty});
      l = 1 + distance.Ratio(kBombCollectDistanceStep); // 512(3+6)
      ip.x += tx / l;
      ip.y += ty / l;
    }

    if (ip.vy < 6_px) {
      ip.vy += kItemGravity;
    }
    ip.count++;
    {
      if (math::WithinRadius({ip.x, ip.y}, {player_.X(), player_.Y()},
                             HitRadius(ip.kind))) {
        switch (ip.kind) {
        case ItemKind::Score: {
          audio_.PlaySfx(SfxId::Select, ip.x);
          player_.AddScore(point);
          effects_.SpawnPointValue(ip.x, ip.y, point);
          if (player_.GrazeCount() != 0U) {
            effects_.SpawnFragment(ip.x, ip.y, FragmentKind::RisingStar);
            effects_.SpawnFragment(ip.x, ip.y, FragmentKind::RisingStar);
          }

          const int star_amt = (player_.GrazeCount() != 0U) ? 2 : 1;
          const auto reward = player_.AddStar(star_amt);
          switch (reward) {
          case PlayerReward::Extend:
            effects_.SpawnString(180 + 64, 80, "E x t e n d  !");
            break;
          case PlayerReward::Bomb:
            effects_.SpawnString(120 + 64, 80, "B o m b   E x t e n d  !");
            break;
          case PlayerReward::None:
            break;
          }

          break;
        }

        case ItemKind::Extend:
          audio_.PlaySfx(SfxId::Select, ip.x);
          effects_.SpawnString(180 + 64, 80, "E x t e n d  !");
          player_.PickupExtend();
          break;

        case ItemKind::Bomb:
          audio_.PlaySfx(SfxId::Select, ip.x);
          effects_.SpawnString(120 + 64, 80, "B o m b   E x t e n d  !");
          player_.PickupBomb();
          break;
        case ItemKind::None:
          break;
        }
        ip.kind = ItemKind::None;
      }
    }

    // Do not delete upward
    if ((ip.x) < playfield::kWorldLeft - 8_px ||
        (ip.x) > playfield::kWorldRight + 8_px ||
        (ip.y) > playfield::kWorldBottom + 8_px) {
      ip.kind = ItemKind::None;
    }
  }

  pool_.Compact(
      [](const ItemData &item) { return item.kind == ItemKind::None; });
}

void ItemSystem::Draw() const {
  int j = 0;
  int x = 0;
  int y = 0;
  Rect src;

  for (const auto &ip : pool_) {
    const int ptn = ((ip.count >> 2) & 3);
    switch (ip.kind) {
    case ItemKind::Score:
      src = Rect::FromLtwh((384 + (ptn << 4)), (256 + 16), 16, 16);
      x = ip.x.ToPixels() - 8;
      y = ip.y.ToPixels() - 8;
      GraphicsSurfaceBlit({x, y}, SurfaceId::System, src);
      break;

    case ItemKind::Extend:
      for (j = 0; j < 8; j++) {
        src = Rect::FromLtwh((384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16);
        const auto offset = math::RoundedPolarVector(
            static_cast<float>(ip.count + static_cast<float>(j * 256 / 8)) *
                math::kLegacyAngleStep,
            12.0F);
        x = ip.x.ToPixels() - 8 + offset.x;
        y = ip.y.ToPixels() - 8 + offset.y;
        GraphicsSurfaceBlit({x, y}, SurfaceId::System, src);
      }

      break;

    case ItemKind::Bomb:
      for (j = 0; j < 8; j++) {
        src = Rect::FromLtwh((384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16);
        const auto offset = math::RoundedPolarVector(
            static_cast<float>((-2 * ip.count) +
                               static_cast<float>(j * 256 / 8)) *
                math::kLegacyAngleStep,
            12.0F);
        x = ip.x.ToPixels() - 8 + offset.x;
        y = ip.y.ToPixels() - 8 + offset.y;
        GraphicsSurfaceBlit({x, y}, SurfaceId::System, src);
      }

      break;

    case ItemKind::None:
      break;
    }
  }
}

void ItemSystem::Reset() { pool_.Reset(); }
