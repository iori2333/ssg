/*************************************************************************************************/
/*   ENEMY.C   敵の管理とか発生制御等 */
/*                                                                                               */
/*************************************************************************************************/

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include "GIAN07/game/entity.h"
#include "ecl/ecl_vm.h"
#include "enemy/BOSS.h"
#include "enemy/ENEMY.h"
#include "entity/ITEM.h"
#include "entity/LLASER.h"
#include "entity/MAID.h"
#include "entity/TAMA.h"
#include "game/GIAN.h"
#include "game/LOADER.h"
#include "game/cast.h"
#include "game/constants.h"
#include "game/coords.h"
#include "game/endian.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

/*
 * ECLコマンドのアドレス更新には ECL_CmdLen[ECLコマンド定数] を使用する
 *
 */

// 変数の実体 → enemy_manager.cpp の EnemyManager に移動
// 下記の参照が後方互換用:
// Enemy, EnemyInd, EnemyNow, ECL_Head, SCL_Head, SCL_Now, Anime,
// HomingX, HomingY, HomingFlag — enemy_manager.cpp で参照として定義

// 関数 //
static void EnemyDrawBomb(int x, int y, uint32_t count);

template <size_t N>
static void Indsort(std::array<uint16_t, N> &indices, uint16_t &count,
                    const std::array<EnemyData, N> &entities) {
  Indsort(indices, count, entities,
          [](const EnemyData &e) { return (e.flag & EF_DELETE); });
}

void UpdateHoming(const EnemyData *e) {
  const int temp = (Viv.y - e->y);

  if (temp < 0)
    return;

  if (temp < HomingFlag) {
    HomingFlag = temp;
    HomingX = e->x;
    HomingY = e->y;
  }
}

bool LaserHITCHK(const EnemyData *e, int ox, int oy, uint8_t d) {
  const int chkw = (min(e->g_height, e->g_width) + (3 * 64));

  const int tx = (e->x - ox);
  const int ty = (e->y - oy);

  const int l = (cosl(d, tx) + sinl(d, ty));
  const int w = abs(-sinl(d, tx) + cosl(d, ty));

  return ((l > 0) && (w < chkw));
}

/*
inline Debug(DWORD old,int id)
{
        RndBuf[id] += (random_ref-old);
}
*/

void EnemyData::Draw() const {
  constexpr auto sid = SURFACE_ID::ENEMY;

  // TODO: Remove once the structure itself uses WORLD_POINT.
  const WORLD_POINT center = {&x, &y};

  const auto &a = Anime[anm_ptn];
  const auto topleft = center.ToPixel(a.size); // 座標セット //

  // 描画モード選択 //
  const auto &src =
      ((a.mode == ANM_DEG) ? a.ptn[static_cast<uint8_t>(d - 64 + 8) >> 4]
                           : a.ptn[anm_c]);
  if (GrpSurface_Blit({topleft.x, topleft.y}, sid, src)) {
    if ((anm_ptn != anm_ptnEx) && IsDamaged) {
      const auto &a = Anime[anm_ptnEx];
      const auto topleft = center.ToPixel(a.size); // 座標セット //
      GrpSurface_Blit({topleft.x, topleft.y}, sid, a.ptn[0]);
    }
  }
}

void enemy_move(void) {
  int i;

  if (BossNow == 0)
    HomingFlag = HOMING_DUMMY;

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];
    e->IsDamaged = 0;
    if (!(e->flag & EF_BOMB)) {
      // 通常の敵の処理 //
      if (EclVM::IsInitialized()) {
        auto &interp = EclVM::Instance();
        EclVM::CheckInterrupts(*e);
        interp.Execute(*e);
      }

      // 弾発射モードによる分岐 //
      if (e->t_rep && e->hp) {
        e->tama_c = (e->tama_c + 1) % (e->t_rep);
        if (e->tama_c == 0) {
          TamaCmd = e->t_cmd;
          TamaCmd.x += e->x;
          TamaCmd.y += e->y;
          tama_set();
        }
      }

      // サボテンヒットチェック //
      if (HITCHK(e->x, Viv.x, e->g_width) && HITCHK(e->y, Viv.y, e->g_height) &&
          Viv.muteki == 0) {
        // ここら辺で敵にダメージを与えるとおもしろいかも？ //
        if (e->flag & EF_HITSB)
          MaidDead();
      }

      // 範囲外チェック //
      if ((e->y < GY_MIN - (e->g_height)) || (e->y > GY_MAX + (e->g_height)) ||
          (e->x < GX_MIN - (e->g_width)) || (e->x > GX_MAX + (e->g_width))) {
        if ((e->flag & EF_CLIP) == 0) {
          if (e->LLaserRef)
            LLaserForceClose(e);
          e->flag = EF_DELETE;
        }
      }
    } else if (e->count >= 8 * ENEMY_BOMB_SPD - 1) {
      e->flag = EF_DELETE;
    }

    // ホーミングの準備 //
    if ((BossNow == 0) && (e->flag & EF_DAMAGE))
      UpdateHoming(e);

    // アニメーションの動作 //
    EnemyAnimeMove(e);

    e->count++;
  }

  Indsort(EnemyInd, EnemyNow, Enemy);
}

void enemy_draw(void) {
  int i;
  int x;
  int y;
  // HRESULT		ddrval;

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];

    // 敵を描画する(クリッピング＆幅、高さ処理を追加すること) //
    x = (e->x >> 6);
    y = (e->y >> 6);
    if (e->flag == EF_BOMB) {
      EnemyDrawBomb(x, y, e->count);
      continue;
    }

    if (e->flag & EF_DRAW) {
      e->Draw();
    }
  }
}

// 雑魚を消滅させる //
extern void enemy_clear(void) {
  int i;

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];
    if (e->flag == EF_BOMB)
      continue;

    if (e->flag & EF_DRAW) {
      e->flag = EF_BOMB;
      e->hp = 0;
      e->count = 0;
      if (e->LLaserRef)
        LLaserForceClose(e); // レーザーの強制クローズ
      Snd_SEPlay(SOUND_ID_BOMB, e->x);
    } else {
      // 描画しないタイプの敵の消去の仕方は他の場合と異なり、 //
      // 爆発アニメ・爆発音を再生しない                       //
      e->flag = EF_DELETE;
      e->hp = 0;
      e->count = 0;
      if (e->LLaserRef)
        LLaserForceClose(e); // レーザーの強制クローズ
                             // 爆発音の再生は行わない //
    }
  }

  Indsort(EnemyInd, EnemyNow, Enemy);
}

void enemyind_set(void) {
  int i;

  for (i = 0; std::cmp_less(i, ENEMY_MAX); i++) {
    // memset(Enemy+i,0,sizeof(Enemy));
    EnemyInd[i] = i;
  }

  EnemyNow = 0;
}

static bool EnemyDamageApply(EnemyData &e, int damage) {
  e.IsDamaged = ((e.count) & 1);
  if (std::cmp_less_equal(e.hp, damage)) {
    Snd_SEPlay(SOUND_ID_BOMB, e.x);
    if (e.LLaserRef) {
      LLaserForceClose(&e); // レーザーの強制クローズ
    }
    PowerUp(static_cast<uint8_t>(e.hp)); // パワーアップ
    e.hp = 0;
    e.count = 0;
    e.flag = EF_BOMB;
    score_add(e.score);
    if (e.item) {
      ItemSet(e.x, e.y, e.item);
    }
  } else {
    Snd_SEPlay(SOUND_ID_HIT, e.x);
    PowerUp(damage); // ここでもパワーアップ
    e.hp -= damage;
  }
  return true;
}

bool enemy_damage(int x, int y, int damage) {
  int i;

  if (BossDamage(x, y, damage)) {
    return true;
  }

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];
    if (HITCHK(x, e->x, e->g_width) && HITCHK(y, e->y, e->g_height) &&
        (e->flag & EF_DAMAGE)) {
      if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE)) {
        continue;
      }
      return EnemyDamageApply(*e, damage);
    }
  }

  return false;
}

bool enemy_damage2(int x, int y, int damage) {
  int i;
  auto ret_val = BossDamage2(x, y, damage);

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];
    if (HITCHK(x, e->x, e->g_width) && (y > e->y) && (e->flag & EF_DAMAGE)) {
      if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE)) {
        continue;
      }
      ret_val = EnemyDamageApply(*e, damage);
    }
  }

  return ret_val;
}

// ナナメレーザーの当たり判定 //
extern void enemy_damage3(int x, int y, uint8_t d) {
  int i;
  // bool	ret_val = false;
  constexpr int damage = 8;

  BossDamage3(x, y, d);

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];
    if (LaserHITCHK(e, x, y, d) && (e->flag & EF_DAMAGE)) {
      if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE)) {
        continue;
      }
      EnemyDamageApply(*e, damage);
    }
  }
}

// すべての敵にダメージを与える /
extern void enemy_damage4(int damage) {
  int i;

  BossDamage4(damage);

  for (i = 0; std::cmp_less(i, EnemyNow); i++) {
    auto *e = &Enemy[EnemyInd[i]];
    if (e->flag & EF_DAMAGE) {
      if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE)) {
        continue;
      }
      EnemyDamageApply(*e, damage);
      // return true;
    }
  }

  // return false;
}

// 敵データを初期化する(x,y は x64 で指定のこと) //
void InitEnemyDataX64(EnemyData *e, int x, int y, uint32_t EclID) {
  e->x = x;
  e->y = y;

  e->cmd = U32LEAt(&ECL_Head[EclID]);

  e->call_addr = e->cmd;

  e->hp = 0xffffffff;
  e->amp = 0;
  e->anm_ptn = 0;
  e->anm_ptnEx = 0; // 追加 : 2000/11/27 (ダメージ中のアニメ)
  e->anm_sp = 0;
  e->anm_c = 0;
  e->count = 0;
  e->evscore = 0;
  e->d = 64;
  e->flag = EF_DAMAGE | EF_DRAW | EF_HITSB;

  e->IsDamaged = 0;

  e->tama_c = Cast::down<uint8_t>(rnd()); // & 0xff;
  e->t_rep = 0;                           // 弾の発射間隔(０：自動発射しない)
  e->g_width = 0;
  e->g_height = 0;

  e->item = ITEM_SCORE;

  e->rep_c = 0;
  e->cmd_c = 0;
  e->v = 64;
  e->vd = 0;
  e->vx = cosl(e->d, e->v);
  e->vy = sinl(e->d, e->v);

  e->LLaserRef = 0;

  e->t_cmd.c = 0;
  e->t_cmd.cmd = TC_WAY;
  e->t_cmd.d = 64;
  e->t_cmd.n = 1;
  e->t_cmd.option = TE_NONE;
  e->t_cmd.type = T_NORM;
  e->t_cmd.v = 3;
  e->t_cmd.x = 0;
  e->t_cmd.y = 0;

  e->t_cmd.dw = 16;
  e->t_cmd.ns = 1;
  e->t_cmd.rep = 0;
  e->t_cmd.vd = 0;

  e->l_cmd.l2 = 0;
  e->l_cmd.x = 0;
  e->l_cmd.y = 0;
  e->l_cmd.notr = 0xff;

  // 変数用レジスタの初期化 //
  e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
  e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;

  // 割り込みベクタの初期化 //
  if (EclVM::IsInitialized()) {
    EclVM::Instance().InitInterrupts(*e);
  }
}

// 強制的に ECL ブロック間を移動する //
void EnemyECL_LongJump(EnemyData *e, uint32_t EclID) {
  if (EclVM::IsInitialized()) {
    EclVM::Instance().LongJump(*e, EclID);
  }
}

// Backward-compatible wrappers for callers in BOSS.cpp, SCROLL.cpp, etc.
void parse_ECL(EnemyData *e) {
  if (EclVM::IsInitialized()) {
    EclVM::Instance().Execute(*e);
  }
}

void CheckECLInterrupt(EnemyData *e) {
  if (EclVM::IsInitialized()) {
    EclVM::Instance().CheckInterrupts(*e);
  }
}

void InitECLInterrupt(EnemyData *e) {
  if (EclVM::IsInitialized()) {
    EclVM::Instance().InitInterrupts(*e);
  }
}

// 敵データを初期化する(x,y は非x64(ランダム可能) で指定のこと) //
void InitEnemyDataSTD(EnemyData *e, short x, short y, uint32_t EclID) {
  int EnemyX = x;
  int EnemyY = y;
  /*
          e->x   = I16LEAt(&p[0]);	// PixelToWorld(I16LEAt(&p[0]));
          e->y   = I16LEAt(&p[2]);	// PixelToWorld(I16LEAt(&p[2]));
  */

  // ランダム配置に対応するぞ //
  EnemyX = (EnemyX == X_RNDV) ? GX_RND() : (EnemyX << 6);
  EnemyY = (EnemyY == Y_RNDV) ? GY_RND() : (EnemyY << 6);

  InitEnemyDataX64(e, EnemyX, EnemyY, EclID);
}

void EnemyAnimeMove(EnemyData *e) {
  const auto *a = &Anime[e->anm_ptn];

  switch (a->mode) {
  case (ANM_NORM):
    if (e->anm_sp > 0 && (e->count % e->anm_sp == 0))
      e->anm_c = (e->anm_c + 1) % (a->n);
    else if (e->anm_sp < 0 && (e->count % (-e->anm_sp) == 0))
      e->anm_c = (e->anm_c + a->n - 1) % (a->n);
    break;

  // 逆方向は不可としておきましょうか... //
  case (ANM_STOP):
    if (e->anm_sp > 0 && (e->count % e->anm_sp == 0)) {
      if (e->anm_c < (a->n - 1))
        e->anm_c++;
    }
    break;

  default:
    break;
  }
}

static void EnemyDrawBomb(int x, int y, uint32_t count) {
  PIXEL_LTRB src;

  /*
          switch(count/ENEMY_BOMB_SPD){
                  case(0):case(1):
                          src = PIXEL_LTWH{ 520, 104,  8,  8 };
                          x-=4;y-=4;
                  break;

                  case(2):case(3):
                          src = PIXEL_LTWH{ 528, 104, 16, 16 };
                          x-=8;y-=8;
                  break;

                  case(4):case(5):
                          src = PIXEL_LTWH{ 544, 104, 24, 24 };
                          x-=12;y-=12;
                  break;

                  case(6):
                          src = PIXEL_LTWH{ 568, 104, 32, 32 };
                          x-=16;y-=16;
                  break;

                  case(7):
                          src = PIXEL_LTWH{ 600, 104, 48, 48 };
                          x-=24;y-=24;
                  break;
          }
  */
  src.top = 296;
  src.left = (count / ENEMY_BOMB_SPD) * 48;
  src.bottom = 296 + 48;
  src.right = src.left + 48;

  x -= 24;
  y -= 24;

  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
}

// parse_ECL, CheckECLInterrupt, InitECLInterrupt, and ID2Value
// have been moved to ecl_interpreter.cpp (EclVM class).
//
// Backward-compatible wrappers follow below. These are called by
// InitEnemyDataX64/STD and enemy_move() via EclVM::Instance().
//
// The original code was:
//
// void parse_ECL(EnemyData *e) { ... }
// void CheckECLInterrupt(EnemyData *e) { ... }
// void InitECLInterrupt(EnemyData *e) { ... }
// void EnemyECL_LongJump(EnemyData *e, uint32_t EclID) { ... }

// ==================================================================
// NOTE: All ECL execution code formerly in this file (parse_ECL,
// CheckECLInterrupt, InitECLInterrupt, ID2Value) has been moved to
// ecl_interpreter.cpp. See EclVM class.
//
// The EnemyECL_LongJump() wrapper and the enemy_move() function now
// delegate to EclVM::Instance() for ECL execution.
// ==================================================================
