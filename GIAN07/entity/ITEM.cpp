/*                                                                           */
/*   Item.cpp   アイテムの処理                                               */
/*                                                                           */
/*                                                                           */

#include "entity/ITEM.h"
#include "entity/item_manager.h"
#include "game/GIAN.h"
#include "game/coord_util.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

// Item[], ItemInd[], ItemNow → item_manager.cpp に移動

// アイテムを発生させる //
void ItemSet(int x, int y, uint8_t type) {
  auto *ip = Items.pool.Alloc();
  if (!ip)
    return;

  constexpr uint8_t deg = -64; // rnd()%(128-110)+128+55;
  ip->x = x;
  ip->y = y;
  ip->type = type; // ITEM_SCORE;
  ip->count = 0;
  ip->auto_collect = false;

  switch (type) {
  case (ITEM_SCORE):
    ip->vx = cosl(deg, 64 * 3);
    ip->vy = sinl(deg, 64 * 3);
    break;

  case (ITEM_EXTEND):
    ip->vx = cosl(deg, 64 * 2);
    ip->vy = sinl(deg, 64 * 2);
    break;

  case (ITEM_BOMB):
    ip->vx = cosl(deg, 64 * 2);
    ip->vy = sinl(deg, 64 * 2);
    break;
  }
}

// アイテムを動かす //
void ItemMove(void) {
  int i, tx, ty, l;

  // 自機がこの高さより上にいる場合、アイテム自動回収
  constexpr int AUTO_COLLECT_Y = (120 * 64);

  // point = 100+(Viv.evade)*100;
  const uint32_t point = ((((SY_MAX - Viv.y) >> 6) + (Viv.evade * 4)) * 160);

  for (i = 0; i < ItemNow; i++) {
    auto *ip = &Item[ItemInd[i]];
    if (!Viv.bomb_time) {
      if (Viv.y < AUTO_COLLECT_Y || ip->auto_collect) {
        // 自機が回収ラインより上、または既に自動回収が発動済み
        ip->auto_collect = true;
        tx = (Viv.x - ip->x);
        ty = (Viv.y - ip->y);
        l = 1 + (isqrt(tx * tx + ty * ty) / 500);
        ip->x += tx / l;
        ip->y += ty / l;
      } else {
        ip->x += ip->vx;
        ip->y += ip->vy;
      }
    } else {
      tx = (Viv.x - ip->x);
      ty = (Viv.y - ip->y);
      l = 1 + (isqrt(tx * tx + ty * ty) / 700); // 512(3+6)
      ip->x += tx / l;
      ip->y += ty / l;
    }

    if (ip->vy < 64 * 6)
      ip->vy += ITEM_GRAVITY;
    ip->count++;
    if (HITCHK(ip->x, Viv.x, ITEM_HITX) && HITCHK(ip->y, Viv.y, ITEM_HITY)) {
      switch (ip->type) {
      case (ITEM_SCORE):
        Snd_SEPlay(SOUND_ID_SELECT, ip->x);
        // PlayRankAdd((SY_MAX-Viv.y)>>10);	// 道具回收不再增加 Rank
        score_add(point);
        StringEffect2(ip->x, ip->y, point);
        if (Viv.evade) {
          fragment_set(ip->x, ip->y, FRG_STAR3);
          fragment_set(ip->x, ip->y, FRG_STAR3);
        }
        break;

      case (ITEM_EXTEND):
        Snd_SEPlay(SOUND_ID_SELECT, ip->x);
        StringEffect(180 + 64, 80, "E x t e n d  !");
        Viv.left++;
        break;

      case (ITEM_BOMB):
        Snd_SEPlay(SOUND_ID_SELECT, ip->x);
        StringEffect(120 + 64, 80, "B o m b   E x t e n d  !");
        Viv.bomb++;
        break;
      }
      ip->type = ITEM_DELETE;
    }

    // 上方向では消去しない //
    if ((ip->x) < GX_MIN - 8 * 64 || (ip->x) > GX_MAX + 8 * 64 ||
        (ip->y) > GY_MAX + 8 * 64)
      ip->type = ITEM_DELETE;
  }

  Items.pool.Compact(
      [](const ITEM_DATA &i) { return (i.type == ITEM_DELETE); });
}

// アイテムを描画する //
void ItemDraw(void) {
  int i, j, x, y;
  PIXEL_LTRB src;

  for (i = 0; i < ItemNow; i++) {
    auto *ip = &Item[ItemInd[i]];
    const uint8_t ptn = ((ip->count >> 2) & 3);
    switch (ip->type) {
    case (ITEM_SCORE):
      src = PIXEL_LTWH{(384 + (ptn << 4)), (256 + 16), 16, 16};
      x = WorldToPixel(ip->x) - 8;
      y = WorldToPixel(ip->y) - 8;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      break;

    case (ITEM_EXTEND):
      for (j = 0; j < 8; j++) {
        src = PIXEL_LTWH{(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16};
        x = WorldToPixel(ip->x) - 8 + cosl(ip->count + j * 256 / 8, 12);
        y = WorldToPixel(ip->y) - 8 + sinl(ip->count + j * 256 / 8, 12);
        GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      }

      //	src = PIXEL_LTWH{
      //		(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16
      //	};
      //	x = WorldToPixel(ip->x) - 8;
      //	y = WorldToPixel(ip->y) - 8;
      //	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
      break;

    case (ITEM_BOMB):
      for (j = 0; j < 8; j++) {
        src = PIXEL_LTWH{(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16};
        x = WorldToPixel(ip->x) - 8 + cosl(-2 * ip->count + j * 256 / 8, 12);
        y = WorldToPixel(ip->y) - 8 + sinl(-2 * ip->count + j * 256 / 8, 12);
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

// アイテム配列の初期化 //
void ItemIndSet(void) { Items.pool.InitIndices(); }
