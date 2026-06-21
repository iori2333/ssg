///
/// Item - Item processing
///

#include "item.h"

#include "core/entity.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "gian.h"
#include "item_manager.h"
#include "game/graphics_backend.h"
#include "player.h"
#include <utility>

// entities[], indices[], count moved to item_manager.cpp

// Spawn an item
void ItemManager::Spawn(int x, int y, uint8_t type) {
  if (count + 1 >= ITEM_MAX) {
    return;
  }

  auto *ip = &entities[indices[count++]];

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

  // point = 100+(Players.GrazeCount())*100;
  const uint32_t point =
      ((((SY_MAX - Players.Y()) >> 6) + (Players.GrazeCount() * 4)) * 160);

  for (uint16_t i = 0; i < count; i++) {
    auto *ip = &entities[indices[i]];
    if (!Players.IsBombActive()) {
      if (Players.Y() < STAR_COLLECT_LINE ||
          Players.GrazeWaitTime() > STAR_COLLECT_EVADETIME ||
          ip->auto_collect) {
        // Player above collect line or auto-collect already active
        ip->auto_collect = true;
        tx = (Players.X() - ip->x);
        ty = (Players.Y() - ip->y);
        l = 1 + (isqrt((tx * tx) + (ty * ty)) / 500);
        ip->x += tx / l;
        ip->y += ty / l;
      } else {
        ip->x += ip->vx;
        ip->y += ip->vy;
      }
    } else {
      tx = (Players.X() - ip->x);
      ty = (Players.Y() - ip->y);
      l = 1 + (isqrt((tx * tx) + (ty * ty)) / 700); // 512(3+6)
      ip->x += tx / l;
      ip->y += ty / l;
    }

    if (ip->vy < 64 * 6) {
      ip->vy += ITEM_GRAVITY;
    }
    ip->count++;
    if (HITCHK(ip->x, Players.X(), ITEM_HITX) &&
        HITCHK(ip->y, Players.Y(), ITEM_HITY)) {
      switch (ip->type) {
      case ITEM_SCORE: {
        Snd_SEPlay(SOUND_ID_SELECT, ip->x);
        // Ranking.Add((SY_MAX-Players.Y())>>10);	// Item pickup no longer
        // increases Rank
        Players.AddScore(point);
        Effects.SpawnPointEffect(ip->x, ip->y, point);
        if (Players.GrazeCount() != 0U) {
          Effects.SpawnFragment(ip->x, ip->y, FRG_STAR3);
          Effects.SpawnFragment(ip->x, ip->y, FRG_STAR3);
        }

        const uint32_t star_amt = (Players.GrazeCount() != 0U) ? 2 : 1;
        const auto reward = Players.AddStar(star_amt);
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
        Snd_SEPlay(SOUND_ID_SELECT, ip->x);
        Effects.SpawnStringEffect(180 + 64, 80, "E x t e n d  !");
        Players.PickupExtend();
        break;

      case ITEM_BOMB:
        Snd_SEPlay(SOUND_ID_SELECT, ip->x);
        Effects.SpawnStringEffect(120 + 64, 80, "B o m b   E x t e n d  !");
        Players.PickupBomb();
        break;
      }
      ip->type = ITEM_DELETE;
    }

    // Do not delete upward
    if ((ip->x) < GX_MIN - (8 * 64) || (ip->x) > GX_MAX + (8 * 64) ||
        (ip->y) > GY_MAX + (8 * 64)) {
      ip->type = ITEM_DELETE;
    }
  }

  Indsort(indices, count, entities,
          [](const ItemData &i) { return (i.type == ITEM_DELETE); });
}

// Draw items
void ItemManager::Draw() {
  int i = 0;
  int j = 0;
  int x = 0;
  int y = 0;
  PIXEL_LTRB src;

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *ip = &entities[indices[i]];
    const uint8_t ptn = ((ip->count >> 2) & 3);
    switch (ip->type) {
    case ITEM_SCORE:
      src = PIXEL_LTWH{(384 + (ptn << 4)), (256 + 16), 16, 16};
      x = (ip->x >> 6) - 8;
      y = (ip->y >> 6) - 8;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      break;

    case ITEM_EXTEND:
      for (j = 0; j < 8; j++) {
        src = PIXEL_LTWH{(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16};
        x = (ip->x >> 6) - 8 + cosl(ip->count + (j * 256 / 8), 12);
        y = (ip->y >> 6) - 8 + sinl(ip->count + (j * 256 / 8), 12);
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      }

      //	src = PIXEL_LTWH{
      //		(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16
      //	};
      //	x = (ip->x>>6) - 8;
      //	y = (ip->y>>6) - 8;
      //	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
      break;

    case ITEM_BOMB:
      for (j = 0; j < 8; j++) {
        src = PIXEL_LTWH{(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16};
        x = (ip->x >> 6) - 8 + cosl((-2 * ip->count) + (j * 256 / 8), 12);
        y = (ip->y >> 6) - 8 + sinl((-2 * ip->count) + (j * 256 / 8), 12);
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      }

      //	src = PIXEL_LTWH{
      //		(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16
      //	};
      //	x = (ip->x>>6) - 8;
      //	y = (ip->y>>6) - 8;
      //	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
      break;

    default:
      break;
    }
  }
}

// Initialize item array
void ItemManager::SetIndices() {
  int i = 0;

  for (i = 0; i < ITEM_MAX; i++) {
    indices[i] = i;
    // memset(Item+i,0,sizeof(ITEM_DATA));
  }

  count = 0;
}
