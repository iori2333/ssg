/*************************************************************************************************/
/*   TAMA.cpp   たまの発射などに関する関数群 */
/*                                                                                               */
/*************************************************************************************************/

#include "gian.h"
#include "level.h"
#include "bullet.h"
#include "bullet_manager.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

////グローバル変数 → bullet_manager.cpp の BulletManager に移動
// command, bullets, count_small, count_large → bullet_manager.cpp の参照 (クロスモジュール)
// indices_small, indices_large, max_small, max_large, speed
// → bullet_manager.h 経由で直接アクセス

////ローカルな関数////
// プライベートメソッドは bullet_manager.h で宣言済み
void _TamaEffectDraw(const Bullet *t); // 弾をエフェクトとして描画？

void BulletManager::Spawn() {
  int v;

  // NORMAL の場合は変更しない(ゲーム中に増減する難易度は考案中) //
  // おそらく switch 中に記述する事になるかと... //
  switch (Ranking.state.GameLevel) {
  case (GAME_EASY):
    SetEasy();
    break;

  case (GAME_HARD):
    SetHard();
    break;

  case (GAME_LUNATIC):
    SetLunatic();
    break;
  }

  // 数値は単純に　(speed /2) *rank/32 + speed/2
  v = SPEEDM(command.v); // 速度の基本値をセットする(GIAN.H)
  if ((command.type & 0x0f) == T_NORM)
    speed = (((v >> 1) * (Ranking.state.Rank)) >> (5 + 8)) + (v >> 1);
  else
    speed = v;

  TamaSetMain();
}

void BulletManager::SpawnEX() {
  speed = SPEEDM(command.v);

  TamaSetMain();
}

// 弾をセットする(ライン状に発射)
void BulletManager::SpawnLine() {
  // uint32_t temp;
  uint16_t *indnow, *indmax, *indp; // 上に同じ

  speed = SPEEDM(command.v);

  // "アクセスする領域" をセットする(小型弾 or 特殊弾)        //
  if ((command.c & 0xf0) == TAMA_SMALL)
    indnow = &count_small, indmax = &max_small, indp = &indices_small[count_small];
  else
    indnow = &count_large, indmax = &max_large, indp = &indices_large[count_large];

  // セットする弾数(連射を考慮に入れる)
  const uint16_t setmax =
      (command.n * ((command.cmd & TAMA_REN) ? command.ns : 1));

  // その他パラメータのセット //
  command.cmd = (command.cmd & 0xf0) | TC_WAY;

  for (const auto i : std::views::iota(0u, setmax)) {
    if ((*indnow) + 1 >= (*indmax))
      return; // セットできない場合

    *indnow = *indnow + 1;    // 弾数をインクリメント
    auto *t = &bullets[indp[i]]; // 弾ポインタをセット

    t->x = t->tx = command.x; // X座標のセット
    t->y = t->ty = command.y; // Y座標のセット

    t->a = command.a; // 注意：サイズは char

    // temp = random_ref;
    t->d = Dir(i); // 弾の発射角度
    // Debug(temp,31);

    t->d16 = (t->d << 8); // 角速度のある運動で使用

    // temp = random_ref;
    t->v = t->v0 = LineCmdNewSpeed(i); // 初速度のセット
    // Debug(temp,30);

    t->vx = cosl(t->d, t->v); // 速度のＸ成分セット
    t->vy = sinl(t->d, t->v); // 速度のＹ成分セット

    t->vd = command.vd;             // 角速度もしくはホーミング率
    t->c = command.c;               // 弾の色＆形状
    t->rep = command.rep;           // 繰り返し回数
    t->type = command.type;         // 弾の種類
    t->option = command.option;     // 弾の属性(バイブ、反射等)
    t->effect = command.cmd & 0xf0; // 弾のエフェクト
    t->count = 0;                   // カウンタの初期化
    t->flag = Flag();          // フラグの初期化
  }
}

// エキストラボス専用弾幕(角度が広くなると、遅くなる) //
void BulletManager::SpawnExtra01() {
  // uint32_t temp;
  uint16_t *indnow, *indmax, *indp; // 上に同じ

  speed = SPEEDM(command.v);

  // "アクセスする領域" をセットする(小型弾 or 特殊弾)        //
  if ((command.c & 0xf0) == TAMA_SMALL)
    indnow = &count_small, indmax = &max_small, indp = &indices_small[count_small];
  else
    indnow = &count_large, indmax = &max_large, indp = &indices_large[count_large];

  // セットする弾数(連射を考慮に入れる)
  const uint16_t setmax =
      (command.n * ((command.cmd & TAMA_REN) ? command.ns : 1));

  // その他パラメータのセット //
  // command.cmd = (command.cmd & 0xf0);

  for (const auto i : std::views::iota(0u, setmax)) {
    if ((*indnow) + 1 >= (*indmax))
      return; // セットできない場合

    *indnow = *indnow + 1;    // 弾数をインクリメント
    auto *t = &bullets[indp[i]]; // 弾ポインタをセット

    t->x = t->tx = command.x; // X座標のセット
    t->y = t->ty = command.y; // Y座標のセット

    t->a = command.a; // 注意：サイズは char

    t->d = Dir(i);   // 弾の発射角度
    t->d16 = (t->d << 8); // 角速度のある運動で使用

    t->v = t->v0 = SpeedEx(t->d); // 初速度のセット

    t->vx = cosl(t->d, t->v); // 速度のＸ成分セット
    t->vy = sinl(t->d, t->v); // 速度のＹ成分セット

    t->vd = command.vd;             // 角速度もしくはホーミング率
    t->c = command.c;               // 弾の色＆形状
    t->rep = command.rep;           // 繰り返し回数
    t->type = command.type;         // 弾の種類
    t->option = command.option;     // 弾の属性(バイブ、反射等)
    t->effect = command.cmd & 0xf0; // 弾のエフェクト
    t->count = 0;                   // カウンタの初期化
    t->flag = Flag();          // フラグの初期化
  }
}

int BulletManager::SpeedEx(uint8_t d) {
  int temp = 0;
  int delta;

  switch (command.v & 0xc0) {
  case (TAMASP_RND1):
    temp = rnd() % 16 - 8; /* DebugOut(u8"2"); */
    break;
  case (TAMASP_RND2):
    temp = rnd() % 32 - 16; /* DebugOut(u8"3"); */
    break;
  case (TAMASP_RND3):
    temp = rnd() % 64 - 32; /* DebugOut(u8"4"); */
    break;
  }

  // d と command.d の値の離れ具合により、速度を変化させる //
  delta = command.d - d;
  if (delta > 128)
    delta -= 256;
  if (delta < -128)
    delta += 256;

  return speed - (speed * abs(delta)) / 23 + temp;
}

void BulletManager::TamaSetMain() {
  // uint32_t temp;
  uint16_t *indnow, *indmax, *indp; // 上に同じ

  // "アクセスする領域" をセットする(小型弾 or 特殊弾)        //
  if ((command.c & 0xf0) == TAMA_SMALL)
    indnow = &count_small, indmax = &max_small, indp = &indices_small[count_small];
  else
    indnow = &count_large, indmax = &max_large, indp = &indices_large[count_large];

  // セットする弾数(連射を考慮に入れる)
  const uint16_t setmax =
      (command.n * ((command.cmd & TAMA_REN) ? command.ns : 1));

  for (const auto i : std::views::iota(0u, setmax)) {
    if ((*indnow) + 1 >= (*indmax))
      return; // セットできない場合

    *indnow = *indnow + 1; // 弾数をインクリメント
    auto *t = &bullets[indp[i]];

    t->x = t->tx = command.x; // X座標のセット
    t->y = t->ty = command.y; // Y座標のセット

    // temp = random_ref;
    t->v = t->v0 = NewSpeed(i); // 初速度のセット
    // Debug(temp,30);

    t->a = command.a; // 注意：サイズは char

    // temp = random_ref;
    t->d = Dir(i); // 弾の発射角度
    // Debug(temp,31);

    t->d16 = (t->d << 8); // 角速度のある運動で使用

    t->vx = cosl(t->d, t->v); // 速度のＸ成分セット
    t->vy = sinl(t->d, t->v); // 速度のＹ成分セット

    t->vd = command.vd;             // 角速度もしくはホーミング率
    t->c = command.c;               // 弾の色＆形状
    t->rep = command.rep;           // 繰り返し回数
    t->type = command.type;         // 弾の種類
    t->option = command.option;     // 弾の属性(バイブ、反射等)
    t->effect = command.cmd & 0xf0; // 弾のエフェクト
    t->count = 0;                   // カウンタの初期化
    t->flag = Flag();          // フラグの初期化
  }
}

void BulletManager::Move() {
  // ヒットチェック後にサボテンの生死判定をしているのは、死んでいる時間 //
  // よりも生きている時間のほうが長いからなのですが...                  //

  // 小型弾の処理 //
  for (const auto i : std::views::iota(0u, count_small)) {
    auto *t = &bullets[indices_small[i]];
    if (t->effect == TE_NONE) {
      MoveByType(t);
      MoveByOption(t);
      if (((t->flag & TF_CLIP) == 0) &&
          ((t->x) < GX_MIN - 4 * 64 || (t->x) > GX_MAX + 4 * 64 ||
           (t->y) < GY_MIN - 4 * 64 || (t->y) > GY_MAX + 4 * 64))
        t->flag = TF_DELETE;
      t->count++;
      if (Players.viv.muteki)
        continue;
      if (HITCHK(t->x, Players.viv.x, TAMA_EVX_SMALL) &&
          HITCHK(t->y, Players.viv.y, TAMA_EVY_SMALL)) {
        TamaEvadeAdd(t);
      }
      if (HITCHK(t->x, Players.viv.x, TAMA_HITX) && HITCHK(t->y, Players.viv.y, TAMA_HITY)) {
        t->flag = TF_DELETE;
        MaidDead();
      }
    } else {
      MoveByEffect(t);
      t->count++;
    }
  }
  Indsort(indices_small, count_small, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  // 大型弾＆特殊弾の処理 //
  for (const auto i : std::views::iota(0u, count_large)) {
    auto *t = &bullets[indices_large[i]];
    if (t->effect == TE_NONE) {
      MoveByType(t);
      MoveByOption(t);
      if (((t->flag & TF_CLIP) == 0) &&
          ((t->x) < GX_MIN - 8 * 64 || (t->x) > GX_MAX + 8 * 64 ||
           (t->y) < GY_MIN - 8 * 64 || (t->y) > GY_MAX + 8 * 64))
        t->flag = TF_DELETE;
      t->count++;
      if (Players.viv.muteki)
        continue;
      if (HITCHK(t->x, Players.viv.x, TAMA_EVX_LARGE) &&
          HITCHK(t->y, Players.viv.y, TAMA_EVY_LARGE)) {
        TamaEvadeAdd(t);
      }
      if (HITCHK(t->x, Players.viv.x, TAMA_HITX) && HITCHK(t->y, Players.viv.y, TAMA_HITY)) {
        t->flag = TF_DELETE;
        MaidDead();
      }
    } else {
      MoveByEffect(t);
      t->count++;
    }
  }
  Indsort(indices_large, count_large, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });
}

void BulletManager::Draw() {
  //	HRESULT		ddrval;
  PIXEL_LTRB src;
  int x, y;
  int dx, dy;

  static const PIXEL_LTRB rcExtraTama[4] = {{128, 384, 128 + 32, 384 + 32},
                                            {128 + 32, 384, 128 + 56, 384 + 24},
                                            {128 + 56, 384, 128 + 72, 384 + 16},
                                            {128 + 72, 384, 128 + 80, 384 + 8}};

  static constexpr uint8_t sizeExtraTama[4] = {16, 12, 8, 4};

  // 大型弾＆特殊弾(16*16) の描画 //
  for (const auto i : std::views::iota(0u, count_large)) {
    auto *t = &bullets[indices_large[i]];

    x = (t->x >> 6) - 8; // -8 は座標の補正用です
    y = (t->y >> 6) - 8; // 上に同じ

    switch (t->effect) {
    case (TE_DELETE):
      src = PIXEL_LTWH{(384 + ((t->count / 6) << 4)), 104, 16, 16};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      continue;

    case (TE_CIRCLE1):
      _TamaEffectDraw(t);
      continue;

      // その他は知らぬ //
    }

    switch (t->c & 0xf0) {
    case (TAMA_LARGE): // 大型丸弾
      src.top = 8;
      src.left = ((t->c & 0x0f) << 4) + 384;
      src.bottom = 24;
      src.right = src.left + 16;
      break;

    case (TAMA_EXTRA): {
      const uint8_t d = (t->c & 3);
      src = rcExtraTama[d];
      x = (t->x >> 6) - sizeExtraTama[d];
      y = (t->y >> 6) - sizeExtraTama[d];
      GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY, src);
    }
      continue;

    case (TAMA_EXTRA2): {
      // The original code spelled this as
      //
      // 	d = (BYTE)(t->d+4)/8;
      //
      // which is a rather misleading way of expressing the following
      // operations:
      //
      // 1) Promote [t->d] to `int` as per C/C++'s arithmetic rules
      // 2) Add 4
      // 3) Take the least significant 8 bits to pretend that it
      //    actually was an 8-bit addition
      // 4) Divide the result by 8
      //
      // Merely removing the seemingly superfluous cast therefore
      // leads to a different result (and thus, a different sprite)
      // as the result of the integer addition is not truncated on
      // overflow. Spelling the truncation as `& 0xFF` doesn't look
      // any less superfluous. Cast::down_sign() is the best solution
      // here, as it enforces its argument to be both larger
      // (`int` > `uint8_t`) and signed.
      //
      // 256(-1) -> 32(-1) に変換
      const auto d = (Cast::down_sign<uint8_t>(t->d + 4) / 8);

      src.top = 320 + ((t->c & 3) << 4); // (c mod 4) * 16
      src.left = d * 16;
      src.bottom = src.top + 16;
      src.right = src.left + 16;
      // x   = (t->x>>6) - 8;	// サイズは１６で固定
      // y   = (t->y>>6) - 8;	// すなわち、そのままでＯＫ！
      GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY, src);
    }
      continue;

    case (TAMA_ANGLE):
      // default:		// 角度アニメーション系
      if (t->c != 32 + 5) {
        src.top = 24 + ((t->c & 0x0f) << 4);
        src.left = ((t->d + 8) & 0xf0) + 384;
        src.bottom = src.top + 16;
        src.right = src.left + 16;
      } else {
        // Same as above.
        //
        // 修正 8 ごとで 32 分割だからズラシは 4
        // d = (Cast::down_sign<uint8_t>(t->d + 8) / 8);
        const auto d = (Cast::down_sign<uint8_t>(t->d + 4) / 8);

        dx = (d % 8) * 32;
        dy = (d / 8) * 32;
        src.top = 304 + dy;
        src.left = 384 + dx;
        src.bottom = src.top + 32;
        src.right = src.left + 32;
        // 注意：すでに(x,y)から(8,8)が減算されているので、
        // ここでは(-16,-16)への補正のためにそれぞれ８を引く
        x -= 8; // ここであたり判定座標の
        y -= 8; // 補正を行うのだ
      }
      break;
    }

    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  // 小型弾(8*8) の描画 //
  for (const auto i : std::views::iota(0u, count_small)) {
    auto *t = &bullets[indices_small[i]];

    x = (t->x >> 6) - 4; // -4 は座標の補正用です
    y = (t->y >> 6) - 4; // 上に同じ

    switch (t->effect) {
    case (TE_DELETE):
      src = PIXEL_LTWH{(384 + ((t->count / 6) << 3)), 120, 8, 8};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      continue;

    case (TE_CIRCLE1):
      _TamaEffectDraw(t);
      continue;

      // その他は知らぬ //
    }

    if (t->c != 0x25) {
      src.top = 0;
      src.left = ((t->c) << 3) + 384; // 0; //(t->d+8)&0xf0;
      src.bottom = 8;
      src.right = src.left + 8;
    } else {
      src.top = 24 + ((t->c & 0x0f) << 4);
      src.left = ((t->d + 8) & 0xf0) + 384;
      src.bottom = src.top + 16;
      src.right = src.left + 16;
    }

    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }
}

// 弾をエフェクトとして描画？ //
namespace { constexpr auto RCSET(int x, int y, int w) -> PIXEL_LTRB { return {x, y, x + w, y + w}; } }
void _TamaEffectDraw(const Bullet *t) {

  static constexpr PIXEL_LTRB Data[6][5] = {
      // [色][パターン]
      {
          // 赤 //
          RCSET(168, 344, 32),
          RCSET(232, 344, 28),
          RCSET(288, 344, 24),
          RCSET(336, 344, 20),
          RCSET(328, 416, 16),
      },
      {
          // 青 //
          RCSET(168, 344 + 32, 32),
          RCSET(232, 344 + 28, 28),
          RCSET(288, 344 + 24, 24),
          RCSET(336, 344 + 20, 20),
          RCSET(328 + 16, 416, 16),
      },
      {
          // 緑 //
          RCSET(168, 344 + 32 * 2, 32),
          RCSET(232, 344 + 28 * 2, 28),
          RCSET(288, 344 + 24 * 2, 24),
          RCSET(336, 344 + 20 * 2, 20),
          RCSET(328 + 16 * 2, 416, 16),
      },
      {
          // 紫 //
          RCSET(168 + 32, 344, 32),
          RCSET(232 + 28, 344, 28),
          RCSET(288 + 24, 344, 24),
          RCSET(336 + 20, 344, 20),
          RCSET(328, 416 + 16, 16),
      },
      {
          // 銀 //
          RCSET(168 + 32, 344 + 32, 32),
          RCSET(232 + 28, 344 + 28, 28),
          RCSET(288 + 24, 344 + 24, 24),
          RCSET(336 + 20, 344 + 20, 20),
          RCSET(328 + 16, 416 + 16, 16),
      },
      {
          // 橙 //
          RCSET(168 + 32, 344 + 32 * 2, 32),
          RCSET(232 + 28, 344 + 28 * 2, 28),
          RCSET(288 + 24, 344 + 24 * 2, 24),
          RCSET(336 + 20, 344 + 20 * 2, 20),
          RCSET(328 + 16 * 2, 416 + 16, 16),
      },
  };
#undef RCSET

  static int Width[5] = {32 / 2, 28 / 2, 24 / 2, 20 / 2, 16 / 2};
  static constexpr std::span<const PIXEL_LTRB, 5> Target[16 * 3] = {
      Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[0], Data[0],
      Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],

      Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[0], Data[0],
      Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],

      Data[0], Data[1], Data[5], Data[3], Data[4], Data[5], Data[0], Data[0],
      Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],
  };

  PIXEL_LTRB temp;
  const int ptn = ((t->count / 4) % 5);
  int x, y;

  x = (t->x >> 6) - Width[ptn];
  y = (t->y >> 6) - Width[ptn];

  // [色][パターン]
  // temp = Data[(t->c&0x0f)%6][ptn];
  if (t->c >= 16 * 3)
    temp = Target[3][ptn];
  else
    temp = Target[t->c][ptn];
  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, temp);
}

void BulletManager::Clear() {
  for (const auto i : std::views::iota(0u, count_small)) {
    auto &t = bullets[indices_small[i]];
    if (t.effect != TE_DELETE) {
      t.effect = TE_DELETE;
      t.count = 0;
      // t.c      = 0x25;
      t.d = 0;
    }
  }

  for (const auto i : std::views::iota(0u, count_large)) {
    auto &t = bullets[indices_large[i]];
    if (t.effect != TE_DELETE) {
      t.effect = TE_DELETE;
      t.count = 0;
      // t.c      = 0x25;
      t.d = 0;
    }
  }
}

// 弾を得点化する(Ret : 得点)
uint32_t BulletManager::ScoreToItems() {
  uint32_t sum = 0;
  uint32_t Score;

  Score = TAMA1_POINT + Players.viv.evade * 100;
  for (const auto i : std::views::iota(0u, count_small)) {
    auto *t = &bullets[indices_small[i]];
    if (t->effect != TE_DELETE) {
      Effects.SpawnPointEffect(t->x - 64 * 4, t->y - 64 * 4, Score);
      sum += Score;
      t->flag = TF_DELETE;
      t->count = 0;
      t->c = 0x25;
      t->d = 0;
    }
  }
  Indsort(indices_small, count_small, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  Score = TAMA2_POINT + Players.viv.evade * 100;
  for (const auto i : std::views::iota(0u, count_large)) {
    auto *t = &bullets[indices_large[i]];
    if (t->effect != TE_DELETE) {
      Effects.SpawnPointEffect(t->x - 64 * 8, t->y - 64 * 8, Score);
      sum += Score;
      t->flag = TF_DELETE;
      t->count = 0;
      t->c = 0x25;
      t->d = 0;
    }
  }
  Indsort(indices_large, count_large, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  return sum;
}

// 弾をアイテム化する //
void BulletManager::ToItems(uint8_t n) {
  // uint32_t sum = 0;
  // uint32_t Score;

  //	Score = TAMA1_POINT + Players.viv.evade * 100;

  if (n == 0) {
    Clear();
    return;
  }

  for (const auto i : std::views::iota(0u, count_small)) {
    auto *t = &bullets[indices_small[i]];
    if (t->effect != TE_DELETE) {
      t->count = 0;
      t->d = 0;

      if (rnd() % n == 0) {
        Items.Spawn(t->x, t->y, ITEM_SCORE);
        t->flag = TF_DELETE;
        t->c = 0x25;
      } else {
        t->effect = TE_DELETE;
        t->count = 0;
        t->d = 0;
      }
    }
  }
  Indsort(indices_small, count_small, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  //	Score = TAMA2_POINT + Players.viv.evade * 100;
  for (const auto i : std::views::iota(0u, count_large)) {
    auto *t = &bullets[indices_large[i]];
    if (t->effect != TE_DELETE) {
      t->count = 0;
      t->d = 0;

      if (rnd() % n == 0) {
        Items.Spawn(t->x, t->y, ITEM_SCORE);
        t->flag = TF_DELETE;
        t->c = 0x25;
      } else {
        t->effect = TE_DELETE;
        t->count = 0;
        t->d = 0;
      }
    }
  }
  Indsort(indices_large, count_large, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  //	return sum;
}

void BulletManager::SetIndices(uint16_t tama1) {
  int i;

  if (tama1 >= TAMA_MAX)
    tama1 = TAMA_MAX - 1;

  // 弾の最大数のセット //
  max_small = tama1;
  max_large = TAMA_MAX - tama1;

  // 弾のインデックス用配列の初期化 //
  for (i = 0; i < tama1; i++)
    indices_small[i] = i;
  for (i = tama1; i < TAMA_MAX; i++)
    indices_large[i - tama1] = i;

  // memset(bullets,0,sizeof(TAMA_DATA)*TAMA_MAX);

  count_small = count_large = 0;
}

void BulletManager::SetEasy() {
  switch (command.cmd & 0x03) {
  case (TC_WAY):
    if (command.n >= 3)
      command.n -= 2;                // 奇数・偶数は変化させない
    command.dw += (command.dw >> 2); // 幅を広げる
    break;

  case (TC_ALL):
  case (TC_RND):
    command.n >>= 1; // 弾数／２
    break;
  }

  if (command.ns >= 2)
    command.ns--; // 連射数_減少
}

void BulletManager::SetHard() {
  switch (command.cmd & 0x03) {
  case (TC_WAY):
    command.n += 2;                  // 奇数・偶数は変化させない
    command.dw -= (command.dw >> 3); // 幅を狭める
    break;

  case (TC_ALL):
    command.n += (((command.n >> 2) > 6) ? 6 : (command.n >> 2));
    break;

  case (TC_RND):
    command.n += (command.n >> 1); // 弾数５０％アップ
    break;
  }

  command.ns++; // 連射数_増加
}

void BulletManager::SetLunatic() {
  switch (command.cmd & 0x03) {
  case (TC_WAY):
    command.n += 4;                 // 奇数・偶数は変化させない
    command.dw -= (command.dw / 3); // 幅を狭める
    break;

  case (TC_ALL):
    command.n += (((command.n / 3) > 12) ? 12 : (command.n / 3));
    break;

  case (TC_RND):
    command.n <<= 1; // 弾数２倍
    break;
  }

  command.ns += 2; // 連射数_増加
}

uint8_t BulletManager::Dir(uint16_t i) {
  uint8_t deg = ((command.cmd & TAMA_ZSET)
                     ? atan8((Players.viv.x - command.x), (Players.viv.y - command.y))
                     : 0);

  deg += command.d;  // 基本角のセット完了
  i = i % command.n; // 連射弾対策

  switch (command.cmd & 0x03) {
  case (TC_WAY):
    i++;
    if (command.n & 1)
      return deg + (i >> 1) * command.dw * (1 - ((i & 1) << 1));
    else
      return deg - (command.dw >> 1) +
             (i >> 1) * command.dw * (1 - ((i & 1) << 1));

  case (TC_ALL):
    return deg + (i << 8) / command.n;

  case (TC_RND):
    // DebugOut(u8"1");
    return deg + rnd() % command.dw - (command.dw >> 1);

  default:
    return 0; // 絶対無いけれど、warning がうるさいので...
  }
}

int BulletManager::NewSpeed(uint16_t i) {
  int temp = 0; // ランダム要素の設定用
  const int vret =
      speed; // SPEEDM(command.v);	// 速度の基本値をセットする(GIAN.H)

  // 速度ランダムは基本値のｎ％変化とするべきかもしれないが... //
  switch (command.v & 0xc0) {
  case (TAMASP_RND1):
    temp = rnd() % 16 - 8; /* DebugOut(u8"2"); */
    break;
  case (TAMASP_RND2):
    temp = rnd() % 32 - 16; /* DebugOut(u8"3"); */
    break;
  case (TAMASP_RND3):
    temp = rnd() % 64 - 32; /* DebugOut(u8"4"); */
    break;
  }

  if (command.cmd & TAMA_REN)
    return vret + (vret >> 3) * (i / command.n) + temp;
  else
    return vret + temp;
}

int BulletManager::LineCmdNewSpeed(uint16_t i) {
  int vret = speed; // 速度の基本値をセットする(GIAN.H)

  i = (i % command.n) + 1; // 連射弾対策

  // 中心からの角度
  const auto deg_factor = ((i >> 1) * command.dw * (1 - ((i & 1) << 1)));
  const uint8_t deg =
      ((command.n & 1) ? deg_factor : -(command.dw >> 1) + deg_factor);

  vret = cosDiv(deg, vret);

  if (command.cmd & TAMA_REN)
    return vret + (vret >> 3) * (i - 1);
  else
    return vret;
}

int BulletManager::Speed(uint16_t i) {
  int temp = 0;                       // ランダム要素の設定用
  const int vret = SPEEDM(command.v); // 速度の基本値をセットする(GIAN.H)

  // 速度ランダムは基本値のｎ％変化とするべきかもしれないが... //
  switch (command.v & 0xc0) {
  case (TAMASP_RND1):
    temp = rnd() % 16 - 8; /* DebugOut(u8"2"); */
    break;
  case (TAMASP_RND2):
    temp = rnd() % 32 - 16; /* DebugOut(u8"3"); */
    break;
  case (TAMASP_RND3):
    temp = rnd() % 64 - 32; /* DebugOut(u8"4"); */
    break;
  }

  if (command.cmd & TAMA_REN)
    return vret + (vret >> 3) * (i / command.n) + temp;
  else
    return vret + temp;
}

uint8_t BulletManager::Flag() {
  switch (command.type) {
  case (T_HOMING):
  case (T_HOMING_M):
  case (T_ROLL):
  case (T_ROLL_A):
  case (T_ROLL_R):
  case (T_SBHOMING):
    return TF_CLIP;

  default:
    return TF_NONE;
  }
}

void BulletManager::MoveByType(Bullet *t) {
  short deg_t;
  // ENEMY_DATA	*e;

  // (x,y)に直接アクセスするのではなく、(tx,ty)にアクセスする事！ //
  switch (t->type & 0x0f) {
  case (T_NORM): // 通常弾
    // MMX_ADD32(&t->tx,&t->vx);
    t->tx += t->vx;
    t->ty += t->vy;
    return;

  case (T_NORM_A): // 加速弾
    t->v += t->a;
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // 上位ビットは一応保存する
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case (T_HOMING): // ｎ回ホーミング
    t->v += t->a;
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if ((t->a > 0) && (t->v >= t->v0)) {
      t->a = -(t->a);
      if (--(t->rep) == 0) {
        t->type = (t->type & 0xf0) | T_NORM;
        t->flag &= (~TF_CLIP);
        t->vx = cosl(t->d, t->v);
        t->vy = sinl(t->d, t->v);
      }
    }
    if ((t->a < 0) && (t->v <= 0)) {
      t->a = -(t->a);
      t->d = atan8((Players.viv.x) - (t->x), (Players.viv.y) - (t->y));
    }
    return;

  case (T_HOMING_M): // ｎ％ホーミング(ミサイル系？)
    // 最適化はしておりませんな... //
    if ((t->count > 19) && (t->count % 2 == 0)) {
      deg_t = atan8((Players.viv.x) - (t->x), (Players.viv.y) - (t->y)) - (t->d);
      if (deg_t < -128)
        deg_t += 256;
      if (deg_t > 128)
        deg_t -= 256;
      t->d = t->d + deg_t * (t->vd) / 255;
    }
    t->v += t->a;
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // 上位ビットは一応保存する
      t->flag &= (~TF_CLIP);
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case (T_ROLL): // 回転弾
    t->d += Cast::sign<uint8_t>(t->vd);
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // 上位ビットは一応保存する
      t->flag &= (~TF_CLIP);
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case (T_ROLL_A): // 回転弾(加速) 最初の加速度は"負"にして下さい！
    t->v += t->a;
    if (t->a > 0) {
      t->d += Cast::sign<uint8_t>(t->vd);
    }
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if ((t->a < 0) && (t->v <= 0))
      t->a = -(t->a);
    if ((t->a > 0) && (t->v >= t->v0)) {
      t->a = -(t->a);
      if (--(t->rep) == 0) {
        t->type = (t->type & 0xf0) | T_NORM;
        t->flag &= (~TF_CLIP);
        t->vx = cosl(t->d, t->v);
        t->vy = sinl(t->d, t->v);
      }
    }
    return;

  case (T_ROLL_R): // 回転弾(反転) 上と同じで加速度に注意！
    t->v += t->a;
    t->d += Cast::sign<uint8_t>(t->vd);
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if ((t->a < 0) && (t->v <= 0)) {
      t->d += 128;
      t->a = -(t->a);
    }
    if ((t->a > 0) && (t->v >= t->v0)) {
      t->a = -(t->a);
      if (--(t->rep) == 0) {
        t->type = (t->type & 0xf0) | T_NORM;
        t->flag &= (~TF_CLIP);
        t->vx = cosl(t->d, t->v);
        t->vy = sinl(t->d, t->v);
      }
    }
    return;

  case (T_GRAVITY): // 落下弾(上昇弾にもできるが...)
    t->vy += t->a;
    // MMX_ADD32(&t->tx,&t->vx);
    t->tx += t->vx;
    t->ty += t->vy;
    return;

  case (T_CHANGE): // 角度強制変更弾
    // MMX_ADD32(&t->tx,&t->vx);
    t->tx += t->vx;
    t->ty += t->vy;
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // 上位ビットは一応保存する
      t->d = Cast::sign<uint8_t>(t->vd);
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case (T_SBHOMING): // サボテン用ホーミング(煙を吐き出すぞ！)
    if (t->count & 1)
      Effects.SpawnFragment(t->x, t->y, FRG_SMOKE);
    t->tx += t->vx;
    t->ty += t->vy;
    if ((t->count < 130 - 60) && Enemies.homing_flag != HOMING_DUMMY) {
      deg_t = atan8(Enemies.homing_x - (t->x), Enemies.homing_y - (t->y)) - (t->d);
    } else if (t->count < 130 - 60) {
      deg_t = atan8(0, (-20 * 64) - (t->y)) - (t->d);
    } else {
      t->flag = TF_NONE;
      deg_t = 0;
    }

    if (deg_t < -128)
      deg_t += 256;
    if (deg_t > 128)
      deg_t -= 256;
    // if(deg_t>-2 && deg_t<2){
    if (deg_t == 0) {
      if (t->vd)
        t->vd--;
      t->v += t->a;
    } else {
      // pbg quirk: Was probably intended to compare the unsigned
      // representation against 0xFA? Since `t->vd` is a `int8_t`,
      // this is always true. Visual Studio 2022 optimizes this
      // comparison away in Release mode, Clang throws a warning.
      //
      // 	if(t->vd<250)
      t->vd++;
      t->v -= t->a;
    }
    t->d += (deg_t * (Cast::sign<uint8_t>(t->vd)) / 255);
    t->vx = cosl(t->d, t->v);
    t->vy = sinl(t->d, t->v);
    return;

  case (T_SBHBOMB): // サボテン用ホーミングボム
    // ちゅうい : この case はダミーです決して実行されてはいけません //
    if (t->count >= 49)
      t->flag = TF_DELETE;
    return;
  }
}

void BulletManager::MoveByOption(Bullet *t) {
  int op_temp = 0;

  // 分裂はとボムは消去要請フラグを立てる必要がある //
  // (x,y)に(tx,ty)の演算結果を利用して、値を代入する //
  switch (t->option & 0xf0) {
  case (TOP_NONE): // オプション無し
    t->x = t->tx;
    t->y = t->ty;
    return;

  case (TOP_WAVE): // 波
    op_temp = sinl(Cast::down_sign<uint8_t>(t->count << 2),
                   ((t->option & 0x0f) << 7));
    t->x = t->tx - sinl(t->d, op_temp);
    t->y = t->ty + cosl(t->d, op_temp);
    return;

  case (TOP_ROLL): { // 回転
    const auto angle = Cast::down_sign<uint8_t>(t->d + (t->count << 1));
    op_temp = (t->option & 0x0f) << 8;
    t->x = (t->tx + cosl(angle, op_temp));
    t->y = (t->ty + sinl(angle, op_temp));
  }
    return;

  case (TOP_PURU): // ぷるぷる
    return;

  case (TOP_REFX): // 反射Ｘ
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      t->d = 128 - t->d;
      t->vx = -(t->vx);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0)
        t->option = TOP_NONE;
      else
        t->option = TOP_REFX | (op_temp - 1);
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;

  case (TOP_REFY): // 反射Ｙ
    if ((t->ty) < GY_MIN) {
      t->d = -t->d;
      t->vy = -(t->vy);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0)
        t->option = TOP_NONE;
      else
        t->option = TOP_REFY | (op_temp - 1);
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;

  case (TOP_REFXY): // 反射ＸＹ
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      t->d = 128 - t->d;
      t->vx = -(t->vx);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0)
        t->option = TOP_NONE;
      else
        t->option = TOP_REFXY | (op_temp - 1);
    } else if ((t->ty) < GY_MIN) {
      t->d = -t->d;
      t->vy = -(t->vy);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0)
        t->option = TOP_NONE;
      else
        t->option = TOP_REFXY | (op_temp - 1);
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;

  case (TOP_DIV): // 分裂
    t->x = t->tx;
    t->y = t->ty;
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      op_temp = 1;
      command.d = 128 - (t->d);
    } else if ((t->ty) < GY_MIN) {
      op_temp = 1;
      command.d = -(t->d);
    }

    if (op_temp == 1) {
      command.x = t->tx + cosl(command.d, t->v);
      command.y = t->ty + sinl(command.d, t->v);
      t->flag = TF_DELETE; // 消滅エフェクトに変更すべきか？
      command.ns = 2;
      command.c = (t->c) & 0x0f;
      command.cmd = (t->option & 0x0f) | TE_CIRCLE1;
      switch (command.cmd & 0x03) {
      case (TC_WAY):
        command.n = 3;
        command.dw = 16;
        command.v = 13 - 2;
        break;
      case (TC_ALL):
        command.n = 10;
        command.v = 13;
        command.d = Cast::down<uint8_t>(rnd());
        if (command.cmd & TAMA_REN)
          command.v -= 2;
        break;
      case (TC_RND):
        command.n = 4;
        command.dw = 128 - 32;        // 128以上だと画面外に...
        command.v = 13 | TAMASP_RND2; // 速度ランダムあり
        break;
      }
      if (command.cmd & TAMA_ZSET)
        command.d = 0, command.dw -= 6;
      command.type = T_NORM;
      command.option = TOP_NONE;
      Snd_SEPlay(12, command.x);
      Spawn(); // 難易度で変化させるところがポイント
    }
    return;

  case (TOP_BOMB): // ボム
    return;
  }
}

void BulletManager::MoveByEffect(Bullet *t) {
  // TE_NONE:エフェクト無しはこの関数にこないので記述しても意味無し //
  // TE_DELETE:消去要請フラグを立てる事を忘れないように！ //
  switch (t->effect & 0xf0) {
  case (TE_ROLL1):
    return;

  case (TE_ROLL2):
    return;

  case (TE_WARN):
    return;

  case (TE_ROCK):
    return;

  case (TE_CIRCLE1):
    t->x = (t->tx += (t->vx >> 1));
    t->y = (t->ty += (t->vy >> 1));

    if (t->count >= 5 * 4 - 1) {
      t->effect = 0;
    }
    return;

  case (TE_CIRCLE2):
    return;

  case (TE_DELETE):
    t->x += (t->vx >> 1);
    t->y += (t->vy >> 1);
    // t->d+=4;
    if (t->count >= 47) {
      t->flag = TF_DELETE;
    }
    return;
  }
}
