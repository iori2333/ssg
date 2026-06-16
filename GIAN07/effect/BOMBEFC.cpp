/*
 *   BombEfc.cpp   : 爆発系エフェクト
 *
 */
#include "BOMBEFC.h"
#include "effect_manager.h"
#include "LOADER.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"
#include <algorithm>
#include <array>
#include <ranges>

// BombEfc[] → effect_manager.cpp の EffectManager に移動

// 秘密の関数（EffectManager の private メソッドとして宣言済み）

// 爆発系エフェクトの初期化 //
void EffectManager::InitBombEffects() {
  for (auto &it : Effects.bomb_effects) {
    it.bIsUsed = false;
  }
}

// 爆発系エフェクトをセットする //
void EffectManager::SpawnBombEffect(int x, int y, uint8_t type) {
  auto p =
      std::ranges::find_if(Effects.bomb_effects, [](const auto &p) { return !p.bIsUsed; });

  // 空いているオブジェクトが存在しない //
  if (p == std::end(Effects.bomb_effects)) {
    return;
  }

  // ターゲット捕捉 //
  p->x = x;
  p->y = y;
  p->type = type;
  p->count = 0;

  switch (type) {
  case EXBOMB_STD:
    Effects.InitBombEffectSTD(&*p);
    break;

  default:
    return;
  }

  // ここまできたら、無事にエフェクトがセットされた事になる //
  p->bIsUsed = true;
}

// 爆発系エフェクトを描画する
void EffectManager::DrawBombEffects() {
  for (auto &it : Effects.bomb_effects) {
    if (!it.bIsUsed) {
      continue;
    }
    switch (it.type) {
    case EXBOMB_STD:
      Effects.DrawBombEffectSTD(&it);
      break;

    default:
      return;
    }
  }
}

// 爆発系エフェクトを動作させる
void EffectManager::MoveBombEffects() {
  for (auto &it : Effects.bomb_effects) {
    if (!it.bIsUsed) {
      continue;
    }
    it.count++;
    switch (it.type) {
    case EXBOMB_STD:
      Effects.MoveBombEffectSTD(&it);
      if (it.count > ((64 * 3) + 32)) {
        it.bIsUsed = false;
      }
      break;

    default:
      return;
    }
  }
}

void EffectManager::InitBombEffectSTD(BombEfcCtrl *p) {
  int i, x, y;
  SpObj *target;

  x = p->x;
  y = p->y;

  target = p->Obj;
  for (i = 0; i < EXBOMB_OBJMAX; i++, target++) {
    target->x = x;
    target->y = y;
    target->d = i % (7 * 2);
  }
}

void EffectManager::DrawBombEffectSTD(BombEfcCtrl *p) {
  int x, y, dx;

  // Graphic 48 * 48 //
  for (const auto &it : p->Obj) {
    const auto *target = &it;
    if (target->d <= 7 * 2) {
      x = (target->x >> 6) - 24;
      y = (target->y >> 6) - 24;
      dx = (target->d >> 1) * 48;
      const PIXEL_LTRB src = PIXEL_LTWH{dx, 296, 48, 48};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    }
  }
}

void EffectManager::MoveBombEffectSTD(BombEfcCtrl *p) {
  int j = 0;
  int x, y, v, rv;

  x = p->x;
  y = p->y;
  const auto t = p->count;
  v = sinl(t / 2 - 64, 200 * 64) + (200 * 64);

  for (auto &it : p->Obj) {
    auto *target = &it;
    if (target->d > 7 * 2) {
      if (t > 64 * 3)
        continue;
      const uint8_t rd = rnd();
      rv = rnd() % 256 + 128;
      target->vx = cosl(rd, rv);
      target->vy = sinl(rd, rv);

      const uint8_t d = ((t * 2) + ((j % 8) << 5)); // + (rnd() % 8) - 4;
      // v = sinl(degx,64*90)+64*90;

      rv = v - rnd() % (v >> 2);
      target->x = cosl(d, rv) + x;
      target->y = sinl(d, rv) + y;
      target->d = 0;

      j++;
    } else {
      target->d = target->d + 1;
      target->x += target->vx;
      target->y += target->vy;
      target->vx -= (target->vx >> 3);
      target->vy -= (target->vy >> 3);
    }
  }
}
