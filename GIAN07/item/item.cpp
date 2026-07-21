///
/// Item - Item processing
///

#include "item.h"
#include "item_manager.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "effect/effect_manager.h"
#include "effect/fragment.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/ut_math.h"

int GetItemHitRadius(uint8_t type) {
  switch (type) {
  case ITEM_BOMB:
  case ITEM_EXTEND:
    return ITEM_HIT_RADIUS_LARGE;
  default:
    return ITEM_HIT_RADIUS;
  }
}

// Spawn an item
void ItemManager::Spawn(int x, int y, uint8_t type) {
  auto *ip = pool.Alloc();
  if (!ip) {
    return;
  }

  constexpr uint8_t deg = -64; // rnd()%(128-110)+128+55;
  ip->x = x;
  ip->y = y;
  ip->type = type; // ITEM_SCORE;
  ip->count = 0;
  ip->auto_collect = false;

  switch (type) {
  case ITEM_SCORE:
    ip->vx = cosl(deg, 64 * 3);
    ip->vy = sinl(deg, 64 * 3);
    break;

  case ITEM_EXTEND:
    ip->vx = cosl(deg, 64 * 2);
    ip->vy = sinl(deg, 64 * 2);
    break;

  case ITEM_BOMB:
    ip->vx = cosl(deg, 64 * 2);
    ip->vy = sinl(deg, 64 * 2);
    break;
  }
}

// Move items
void ItemManager::Move() {
  int tx = 0;
  int ty = 0;
  int l = 0;

  // Auto-collect items when player is above this height

  // point = 100+(player_->GrazeCount())*100;
  const uint32_t point =
      ((((SY_MAX - player_->Y()) >> 6) + (player_->GrazeCount() * 4)) * 160);

  for (auto &ip : pool) {
    if (!player_->IsBombActive()) {
      if (player_->Y() < STAR_COLLECT_LINE ||
          player_->GrazeWaitTime() > STAR_COLLECT_EVADETIME ||
          ip.auto_collect) {
        // Player above collect line or auto-collect already active
        ip.auto_collect = true;
        tx = (player_->X() - ip.x);
        ty = (player_->Y() - ip.y);
        l = 1 + (isqrt((tx * tx) + (ty * ty)) / 500);
        ip.x += tx / l;
        ip.y += ty / l;
      } else {
        ip.x += ip.vx;
        ip.y += ip.vy;
      }
    } else {
      tx = (player_->X() - ip.x);
      ty = (player_->Y() - ip.y);
      l = 1 + (isqrt((tx * tx) + (ty * ty)) / 700); // 512(3+6)
      ip.x += tx / l;
      ip.y += ty / l;
    }

    if (ip.vy < 64 * 6) {
      ip.vy += ITEM_GRAVITY;
    }
    ip.count++;
    {
      const int64_t dx = static_cast<int64_t>(ip.x) - player_->X();
      const int64_t dy = static_cast<int64_t>(ip.y) - player_->Y();
      const int r = GetItemHitRadius(ip.type);
      if ((dx * dx + dy * dy) < (static_cast<int64_t>(r) * r)) {
        switch (ip.type) {
        case ITEM_SCORE: {
          Snd_SEPlay(SfxId::Select, ip.x);
          // Ranking.Add((SY_MAX-player_->Y())>>10);	// Item pickup no longer
          // increases Rank
          player_->AddScore(point);
          Effects.SpawnPointEffect(ip.x, ip.y, point);
          if (player_->GrazeCount() != 0U) {
            Effects.SpawnFragment(ip.x, ip.y, FRG_STAR3);
            Effects.SpawnFragment(ip.x, ip.y, FRG_STAR3);
          }

          const uint32_t star_amt = (player_->GrazeCount() != 0U) ? 2 : 1;
          const auto reward = player_->AddStar(star_amt);
          switch (reward) {
          case PlayerReward::EXTEND:
            Effects.SpawnStringEffect(180 + 64, 80, "E x t e n d  !");
            break;
          case PlayerReward::BOMB:
            Effects.SpawnStringEffect(120 + 64, 80, "B o m b   E x t e n d  !");
            break;
          case PlayerReward::NONE:
            break;
          }

          break;
        }

        case ITEM_EXTEND:
          Snd_SEPlay(SfxId::Select, ip.x);
          Effects.SpawnStringEffect(180 + 64, 80, "E x t e n d  !");
          player_->PickupExtend();
          break;

        case ITEM_BOMB:
          Snd_SEPlay(SfxId::Select, ip.x);
          Effects.SpawnStringEffect(120 + 64, 80, "B o m b   E x t e n d  !");
          player_->PickupBomb();
          break;
        }
        ip.type = ITEM_DELETE;
      }
    }

    // Do not delete upward
    if ((ip.x) < GX_MIN - (8 * 64) || (ip.x) > GX_MAX + (8 * 64) ||
        (ip.y) > GY_MAX + (8 * 64)) {
      ip.type = ITEM_DELETE;
    }
  }

  pool.Compact(
      [](const ItemData &i) { return (i.type == ITEM_DELETE); });
}

// Draw items
void ItemManager::Draw() {
  int j = 0;
  int x = 0;
  int y = 0;
  PIXEL_LTRB src;

  for (auto &ip : pool) {
    const uint8_t ptn = ((ip.count >> 2) & 3);
    switch (ip.type) {
    case ITEM_SCORE:
      src = PIXEL_LTWH{(384 + (ptn << 4)), (256 + 16), 16, 16};
      x = (ip.x >> 6) - 8;
      y = (ip.y >> 6) - 8;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      break;

    case ITEM_EXTEND:
      for (j = 0; j < 8; j++) {
        src = PIXEL_LTWH{(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16};
        x = (ip.x >> 6) - 8 + cosl(ip.count + (j * 256 / 8), 12);
        y = (ip.y >> 6) - 8 + sinl(ip.count + (j * 256 / 8), 12);
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      }

      //	src = PIXEL_LTWH{
      //		(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16
      //	};
      //	x = (ip.x>>6) - 8;
      //	y = (ip.y>>6) - 8;
      //	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
      break;

    case ITEM_BOMB:
      for (j = 0; j < 8; j++) {
        src = PIXEL_LTWH{(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16};
        x = (ip.x >> 6) - 8 + cosl((-2 * ip.count) + (j * 256 / 8), 12);
        y = (ip.y >> 6) - 8 + sinl((-2 * ip.count) + (j * 256 / 8), 12);
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      }

      //	src = PIXEL_LTWH{
      //		(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16
      //	};
      //	x = (ip.x>>6) - 8;
      //	y = (ip.y>>6) - 8;
      //	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
      break;

    default:
      break;
    }
  }
}

// Initialize item pool
void ItemManager::Init() { pool.Init(); }
