/*                                                                           */
/*   Boss.cpp   ボスの処理(中ボス含む)                                       */
/*                                                                           */
/*                                                                           */

#include "BOMBEFC.h" // 爆発エフェクト処理
#include "boss.h"
#include "boss_manager.h"
#include "EnemyExCtrl.h"
#include "GEOMETRY.h"
#include "gian.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

///// [ 定数 ] /////

// BOSS_MAX, BOSSHPG_HEIGHT → BOSS.h に移動

// ボスの状態 //
static constexpr auto BEXST_NORM = 0x00; // 通常のＥＣＬで動作中
static constexpr auto BEXST_DEAD = 0x01; // 死亡中<-こいつは多分使っていないぞ(2000/10/31)
static constexpr auto BEXST_WING01 = 0x02; // 蝶の羽
static constexpr auto BEXST_WING02 = 0x03; // 天使の羽
static constexpr auto BEXST_SHILD1 = 0x04; // シールド１
static constexpr auto BEXST_SHILD2 = 0x05; // シールド２

// 体力ゲージ編 //
static constexpr auto BOSSHPG_WIDTH = 256; // 体力ゲージの幅
// BOSSHPG_HEIGHT → BOSS.h に移動
static constexpr auto BOSSHPG_START_X = X_MAX; // 体力ゲージの初期Ｘ
static constexpr auto BOSSHPG_END_X = 260; // 体力ゲージの最終Ｘ

static constexpr auto BHPG_DEAD = 0x00; // 体力ゲージは使用されていない
static constexpr auto BHPG_OPEN1 = 0x01; // 体力ゲージを開く(第一エフェクト中)
static constexpr auto BHPG_OPEN2 = 0x02; // 体力ゲージを開く(体力上昇中)
static constexpr auto BHPG_NORM = 0x03; // 体力ゲージの準備ができている
static constexpr auto BHPG_CLOSE = 0x04; // 体力ゲージを閉じる
static constexpr auto BHPG_OPEN3 = 0x05; // 体力ゲージを更新する

///// [構造体] /////

// BOSSHPG_INFO → BOSS.h に移動

///// [ 変数 ] /////

// bosses[], count, hpg → boss_manager.cpp の BossManager に移動

// 秘密の関数 //
static void HPG_Open(uint32_t max);    // ボスの体力ゲージをオープンする
static void HPG_Move(uint32_t now);    // ボスの体力ゲージを増減する
static void HPG_Close(void);           // ボスの体力ゲージをクローズする
static void HPG_Update(uint32_t next); // ボスの体力ゲージを上昇させる

static int PutBoss(int x, int y, uint32_t id); // ボスをセットする
static void STDMove(BossData *b);         // ノーマルECL互換の移動

// ボスデータ配列を初期化する(中断、ステージクリア時に使用) //
void BossManager::Init(void) {
  for (auto &it : bosses) {
    it.IsUsed = false; // 要はこれをゼロにすれば良い

    // 特殊変数の初期化.... //
    it.ExMove = BossManager::STDMove; // 特殊移動関数
    it.ExCount = 0;
    it.ExState = BEXST_NORM; // 特殊状態変数
    it.Hit = nullptr;        // 特殊当たり判定
  }

  // ボスが死んでいるのだから、体力ゲージはもちろん表示しない //
  hpg.State = BHPG_DEAD;

  // ボスの数を０にする //
  count = 0;

  // 蛇管理を初期化(やや謎) //
  SnakyInit();

  // ビット管理も初期化 //
  BitInit();
}

// ボスをセットする //
void BossManager::Set(int x, int y, uint32_t BossID) {
  int n;
  uint32_t HP_Sum = 0;

  // ｘ６４座標に変換してボスをセットする //
  n = PutBoss(x << 6, y << 6, BossID);

  if (n == BOSS_MAX)
    return; // ここに来たらバグ

  bosses[n].ExCount = 0;
  bosses[n].ExMove = BossManager::STDMove;
  bosses[n].ExState = BEXST_NORM;
  bosses[n].IsUsed = true;

  Enemies.Execute(&(bosses[n].Edat));
  // ObjectLockOn(&(bosses[n].Edat.x),&(bosses[n].Edat.y),bosses[n].Edat.g_width,bosses[n].Edat.g_height);

  for (const auto &it : bosses) {
    if (it.IsUsed) {
      HP_Sum += it.Edat.hp;
    }
  }

  HPG_Open(HP_Sum);
  count++;
}

// ボスをセットする(ＥＣＬ用) //
void BossManager::SetEx(int x, int y, uint32_t BossID) {
  int n;
  uint32_t HP_Sum = 0;

  // ｘ６４座標に変換してボスをセットする //
  n = PutBoss(x << 6, y << 6, BossID);

  if (n == BOSS_MAX)
    return; // ここに来たらバグ

  bosses[n].ExCount = 0;
  bosses[n].ExMove = BossManager::STDMove;
  bosses[n].ExState = BEXST_NORM;
  bosses[n].IsUsed = true;

  Enemies.Execute(&(bosses[n].Edat));
  // ObjectLockOn(&(bosses[n].Edat.x),&(bosses[n].Edat.y),bosses[n].Edat.g_width,bosses[n].Edat.g_height);

  for (const auto &it : bosses) {
    if (it.IsUsed) {
      HP_Sum += it.Edat.hp;
    }
  }

  HPG_Update(HP_Sum);
  count++;
}

// ボスを動かす //
void BossManager::Move(void) {
  uint32_t HP_Sum = 0;
  EnemyData *e;

  Enemies.homing_flag = HOMING_DUMMY;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);
      e->IsDamaged = 0;
      b->ExMove(b);

      // サボテンヒットチェック //
      if (HITCHK(e->x, Players.viv.x, e->g_width) && HITCHK(e->y, Players.viv.y, e->g_height) &&
          Players.viv.muteki == 0) {
        // ここら辺で敵にダメージを与えるとおもしろいかも？ //
        if (e->flag & EF_HITSB)
          MaidDead();
      }

      // ホーミングの準備 //
      if (e->flag & EF_DAMAGE)
        Enemies.UpdateHoming(e);

      // 体力の総和を表示する //
      HP_Sum += b->Edat.hp;

      // アニメーションの動作 //
      Enemies.UpdateAnimation(e);

      e->count++;
    }
  }

  SnakyMove();
  BitMove();
  HPG_Move(HP_Sum);
}

// ボスを描画する
void BossManager::Draw(void) {
  constexpr auto sid = SURFACE_ID::ENEMY;
  int x, y;
  int w, h, t;
  EnemyData *e;
  PIXEL_LTRB wing;

  BitLineDraw();

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);

      x = (e->x >> 6);
      y = (e->y >> 6);

      // 霊魂状態 //
      if (b->ExState == BEXST_SHILD2 && Players.viv.bomb_time && (e->flag & EF_DRAW)) {
        wing = PIXEL_LTWH{(160 + (Cast::sign<int32_t>(e->count / 2) % 4) * 40),
                          80, 40, 40};

        // pbg quirk: Blitted without clipping?! I'd consider this a
        // bug if it wasn't explicitly commented as such. Fine then...
        GrpBackend_SetClip(GRP_RES_RECT);

        // クリッピングなし
        GrpSurface_Blit({(x - 20), (y - 20)}, sid, wing);

        GrpBackend_SetClip({X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)});
        continue;
      }

      // バリア状態 //
      if (b->ExState == BEXST_SHILD1 && Players.viv.bomb_time && (e->flag & EF_DRAW)) {
        GrpGeom->Lock();
        for (uint8_t j = 0; j <= 5; j++) {
          GrpGeom->SetColor({(5u - j), (5u - j), 5u});
          GeomCircle({x, y}, (sinl((e->count * 4), (30 + (j * 4))) + 80));
        }
        GrpGeom->Unlock();
      }

      switch (b->ExState) {
      case (BEXST_WING01):
        t = (b->ExCount - 64 - 8) << 2;
        if (t < 0)
          t = 0;
        w = 64;
        h = 92;
        wing = {0, 176, 128, 360};
        GrpSurface_Blit({(x - w - t), (y - h)}, sid, wing);
        wing = {128, 176, 256, 360};
        GrpSurface_Blit({(x - w + t), (y - h)}, sid, wing);
        break;

      case (BEXST_WING02):
        w = 44;
        h = 52;
        wing = {552, 0, 640, 104};
        GrpSurface_Blit({(x - w - 50), (y - h)}, sid, wing);
        wing = {552, 104, 640, 208};
        GrpSurface_Blit({(x - w + 50), (y - h)}, sid, wing);
        break;
      }

      if (e->flag & EF_DRAW) {
        e->Draw();
      }
    }
  }
}

// ボス用・敵弾クリアの前処理関数 //
void BossManager::ClearCmd(void) { BitDelete(); }

// ボスの体力ゲージをオープンする //
void BossManager::HPG_Open(uint32_t max) {
  int i;

  hpg.Max = max;    // 最大値
  hpg.Now = 0;      // 最初のエフェクトで上昇して行くので
  hpg.Next = max;   // 次の体力値
  hpg.Update = max; // 更新用の値

  hpg.State = BHPG_OPEN1;
  hpg.Count = 0;

  // 表示用初期Ｘを指定する(乱数を使用するが...) //
  for (i = 0; i < BOSSHPG_HEIGHT; i++) {
    hpg.XTemp[i] = BOSSHPG_START_X + i * 20;
  }
}

// ボスの体力ゲージを上昇させる //
void BossManager::HPG_Update(uint32_t next) {
  //	hpg.Max  = max;		// 最大値
  //	hpg.Now  = 0;		// 最初のエフェクトで上昇して行くので
  hpg.Update = next; // 次の体力値

  hpg.State = BHPG_OPEN3;
  //	hpg.Count = 0;
}

// ボスの体力ゲージを増減する //
void BossManager::HPG_Move(uint32_t now) {
  int i;
  int ChkCount = 0;

  hpg.Next = now;

  switch (hpg.State) {
  case (BHPG_OPEN1): {
    for (auto &it : hpg.XTemp) {
      it -= 6;
      if (it <= BOSSHPG_END_X) {
        it = BOSSHPG_END_X;
        ChkCount++;
      }
    }

    if (ChkCount == BOSSHPG_HEIGHT)
      hpg.State = BHPG_OPEN2;
  } break;

  case (BHPG_OPEN2):
    hpg.Now += ((hpg.Max >> 7) + 1);
    if (hpg.Now >= hpg.Max) {
      hpg.Now = hpg.Max;
      hpg.State = BHPG_NORM;
    }
    break;

  case (BHPG_OPEN3):
    hpg.Now += ((hpg.Max >> 7) + 1);
    if (hpg.Now >= hpg.Update) {
      hpg.Now = hpg.Update;
      hpg.State = BHPG_NORM;
    }
    break;

  case (BHPG_NORM):
    if (hpg.Now > hpg.Next) {
      // temp = max(hpg.Max>>10,1);
      // temp = max((30*8*3)/max(hpg.Max,1),3);
      const auto temp =
          (std::max)(((std::max)(hpg.Max, 1u) / (30 * 8 * 4)), 3u);
      if (hpg.Now - hpg.Next > temp)
        hpg.Now -= temp;
      else
        hpg.Now = hpg.Next;
    }
    if (hpg.Now == 0)
      HPG_Close();
    break;

  case (BHPG_CLOSE):
    hpg.XTemp[BOSSHPG_HEIGHT - 1] += 6;
    for (i = BOSSHPG_HEIGHT - 2; i >= 0; i--) {
      hpg.XTemp[i] = max(hpg.XTemp[i], hpg.XTemp[i + 1] - 20);
    }
    if (hpg.XTemp[0] >= BOSSHPG_START_X)
      HPG_Close();
    break;

  case (BHPG_DEAD):
    // もちろん何もしない //
    return;
  }

  hpg.Count++;
}

// ボスの体力ゲージをクローズする //
void BossManager::HPG_Close(void) {
  // 後で変更のこと //
  hpg.State = BHPG_CLOSE;
}

// ボスの体力ゲージを描画する //
void BossManager::DrawHPG(void) {
  PIXEL_LTRB src;
  int i;

  switch (hpg.State) {
  case (BHPG_OPEN1):
  case (BHPG_CLOSE):
    // エフェクト付き枠の描画 //
    for (i = 0; i < BOSSHPG_HEIGHT; i++) {
      src = {0, (104 + i), BOSSHPG_WIDTH, (104 + i + 1)};
      GrpSurface_Blit({hpg.XTemp[i], (16 + i)}, SURFACE_ID::SYSTEM, src);
    }
    break;

  case (BHPG_OPEN2):
  case (BHPG_NORM):
  case (BHPG_OPEN3): {
    // 体力ゲージの描画 //
    constexpr WINDOW_COORD left = (BOSSHPG_END_X + 3);
    constexpr WINDOW_COORD top = (16 + 3);
    constexpr WINDOW_COORD bottom = (top + 11);
    const auto x1 = (left + ((hpg.Next * 30 * 8) / hpg.Max));
    const auto x2 = (left + ((hpg.Now * 30 * 8) / hpg.Max));
    constexpr uint8_t alpha = (128 + 64);
    constexpr RGB216 col = {0, 1, 5};

    GrpGeom->Lock();
    GrpGeom->SetAlphaNorm(alpha);
    if (auto *gp = GrpGeom_Poly()) {
      VERTEX_XY Src[4] = {
          {0, top},
          {left, top},
          {left, bottom},
          {0, bottom},
      };
      if (x1 < x2) {
        Src[0].x = Src[3].x = x1;
        GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
        // gp->DrawBoxA(left, top, x1, bottom);
        gp->SetColor({5, 0, 0});
        gp->DrawBoxA(x1, top, x2, bottom);
      } else {
        Src[0].x = Src[3].x = x2;
        GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
        // gp->DrawBoxA(left, top, x2, bottom);
      }
    } else if (auto *gf = GrpGeom_FB()) {
      constexpr auto line_top = (top + 5);
      constexpr auto line_bottom = (bottom - 4);
      gf->SetColor(col);
      if (x1 < x2) {
        gf->DrawBoxA(left, top, x2, line_top);
        gf->DrawBoxA(left, line_bottom, x2, bottom);
        gf->SetColor({5, 5, 5});
        gf->DrawBoxA(left, line_top, x1, line_bottom);
        gf->SetColor({5, 0, 0});
        gf->DrawBoxA(x1, top, x2, bottom);
      } else {
        gf->DrawBoxA(left, top, x2, line_top);
        gf->DrawBoxA(left, line_bottom, x2, bottom);
        gf->SetColor({5, 5, 5});
        gf->DrawBoxA(left, line_top, x2, line_bottom);
      }
    }

    GrpGeom->Unlock();

    // 枠の描画 //
    src = {0, 104, BOSSHPG_WIDTH, 128};
    GrpSurface_Blit({BOSSHPG_END_X, 16}, SURFACE_ID::SYSTEM, src);
  } break;

  case (BHPG_DEAD):
    // もちろん何もしない //
    break;
  }
}

// 現在出現しているボス全てのＨＰを０にする //
void BossManager::KillAll(void) {
  // 破壊後の破片放出などの処理は、ダメージを与えて破壊するのと同等の関数を //
  // 使用するが、当然のごとく、得点＆経験値？は入手できない                 //
  // レーザークローズも忘れずに！！                                         //

  EnemyData *e;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);
      SnakyDelete(b);
      Effects.SpawnFragment(e->x, e->y, FRG_FATCIRCLE);
      Effects.SpawnBombEffect(e->x, e->y, EXBOMB_STD);
      Snd_SEPlay(SOUND_ID_BOSSBOMB, e->x);
      if (e->LLaserRef)
        Lasers.ForceCloseLong(e); // レーザーの強制クローズ
      e->hp = 0;
      e->count = 0;
      e->flag = EF_BOMB;
      b->IsUsed = false;
    }
  }

  count = 0;
}

bool BossManager::ApplyDamage(BossData &b, EnemyData &e, int damage) {
  e.IsDamaged = ((e.count) & 1);
  if (e.hp <= damage) { // ボスの死亡処理(後で変更すること!!)
    SnakyDelete(&b);
    BitDelete();
    Enemies.Clear();
    Effects.SpawnFragment(e.x, e.y, FRG_FATCIRCLE);
    Effects.SpawnBombEffect(e.x, e.y, EXBOMB_STD);
    Scroller.Command(SCMD_QUAKE);
    Snd_SEPlay(SOUND_ID_BOSSBOMB, e.x);
    if (e.LLaserRef) {
      Lasers.ForceCloseLong(&e); // レーザーの強制クローズ
    }
    PowerUp(Cast::down<uint8_t>(e.hp));
    e.hp = 0;
    e.count = 0;
    e.flag = EF_BOMB;

    // 最後の一匹だった場合 //
    if (count == 1) {
      char buf[100];
      const auto temp = Bullets.ScoreToItems(); // 弾→スコアエフェクト
      // sprintf(buf, "%3d Evade  %5dPts", Players.viv.evade, Players.viv.evadesc);
      sprintf(buf, "  Bonus    %7uPts", temp);
      Effects.SpawnStringEffect(180, 60, buf);
      score_add(temp);
    }

    if (e.item) {
      Items.Spawn(e.x, e.y, e.item);
    }
    score_add(e.score);
    Lasers.Clear();
    b.IsUsed = false;
    count--; // ボスの参照カウント？を使用する
  } else {
    Snd_SEPlay(SOUND_ID_HIT, e.x);
    PowerUp(damage);
    e.hp -= damage;
  }
  return true;
}

// ボスにダメージを与える //
bool BossManager::DamageAt(int x, int y, int damage) {
  int i;
  EnemyData *e;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0) {
    return false;
  }

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.viv.bomb_time)
        continue;
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (HITCHK(x, e->x, e->g_width) && HITCHK(y, e->y, e->g_height) &&
          (e->flag & EF_DAMAGE)) {
        if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE))
          continue;
        else {
          return ApplyDamage(*b, *e, damage);
        }
      }
    }
  }
  return false;
}

// ボスにダメージを与える(ｙ上方向無限Ver) //
bool BossManager::DamageAt2(int x, int y, int damage) {
  int i;
  EnemyData *e;
  bool ret_val = false;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0) {
    return false;
  }

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.viv.bomb_time)
        continue;
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (HITCHK(x, e->x, e->g_width) && (y > e->y) && (e->flag & EF_DAMAGE)) {
        if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE))
          continue;
        else {
          ret_val = ApplyDamage(*b, *e, damage);
        }
      }
    }
  }
  return ret_val;
}

// ボスにダメージを与える(ナナメレーザー) //
void BossManager::DamageAt3(int x, int y, uint8_t d) {
  int i;
  EnemyData *e;
  // BOOL			ret_val = FALSE;
  int damage = 2;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0)
    return;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.viv.bomb_time)
        continue;
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (EnemyManager::LaserHITCHK(e, x, y, d) && (e->flag & EF_DAMAGE)) {
        if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE))
          continue;
        else {
          ApplyDamage(*b, *e, damage);
        }
      }
    }
  }
}

// ボスにダメージを与える(すべての敵)
void BossManager::DamageAll(int damage) {
  int i;
  EnemyData *e;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0)
    return;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.viv.bomb_time)
        continue;
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (e->flag & EF_DAMAGE) {
        if (e->flag == EF_BOMB || !(e->flag & EF_DAMAGE))
          continue;
        else {
          ApplyDamage(*b, *e, damage);

          // return TRUE;
        }
      }
    }
  }
  //	return FALSE;
}

int BossManager::PutBoss(int x, int y, uint32_t id) {
  // if(EnemyNow+1>=ENEMY_MAX) return;
  // e = Enemy+ (*(EnemyInd+EnemyNow));
  // EnemyNow++;
  // n      = 4 + (((BYTE)p[4])<<2);
  // e->x   = (*(short *)(&p[0]));	//((int)(*(short *)(&p[0])))*64;
  // e->y   = (*(short *)(&p[2]));	//((int)(*(short *)(&p[2])))*64;

  auto it =
      std::ranges::find_if(bosses, [](const auto &it) { return !it.IsUsed; });

  // まず、あり得ないのだが... //
  if (it == std::end(bosses)) {
    return BOSS_MAX;
  }

  auto *e = &it->Edat;

  const uint32_t addr = (4 + (id << 2)); // ちと、やばいね...
  Enemies.InitDataX64(e, x, y, addr);
  e->item = 0;

  /*
  e->x = x;
  e->y = y;

  // ランダム配置に対応するぞ //
  //e->x = (e->x==X_RNDV) ? GX_RND : (e->x<<6);
  //e->y = (e->y==Y_RNDV) ? GY_RND : (e->y<<6);
  addr   = 4 + (id<<2);		// ちと、やばいね...
  e->cmd = (*(DWORD *)(&ECL_Head[addr]));

  e->call_addr = e->cmd;

  e->hp       = 0xffffffff;
  e->amp      = 0;
  e->anm_ptn  = 0;
  e->anm_sp   = 0;
  e->anm_c    = 0;
  e->count    = 0;
  e->evscore  = 0;
  e->d        = 64;
  e->flag     = EF_DAMAGE|EF_DRAW|EF_HITSB;

  e->tama_c   = rnd()&0xff;
  e->t_rep    = 0;			// 弾の発射間隔(０：自動発射しない)
  e->g_width  = 0;
  e->g_height = 0;

  e->item     = 0;

  e->rep_c    = 0;
  e->cmd_c    = 0;
  e->v        = 64;
  e->vd       = 0;
  e->vx       = cosl(e->d,e->v);
  e->vy       = sinl(e->d,e->v);

  e->LLaserRef = 0;

  e->t_cmd.c      = 0;
  e->t_cmd.cmd    = TC_WAY;
  e->t_cmd.d      = 64;
  e->t_cmd.n      = 1;
  e->t_cmd.option = TE_NONE;
  e->t_cmd.type   = T_NORM;
  e->t_cmd.v      = 3;
  e->t_cmd.x      = 0;
  e->t_cmd.y      = 0;

  e->t_cmd.dw     = 16;
  e->t_cmd.ns     = 1;
  e->t_cmd.rep    = 0;
  e->t_cmd.vd     = 0;

  // 変数用レジスタの初期化 //
  e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
  e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;

  // 割り込みベクタの初期化 //
  Enemies.InitInterrupts(e);
*/
  return std::distance(std::begin(bosses), it);
}

// ノーマルECL互換の移動 //
void BossManager::STDMove(BossData *b) {
  EnemyData *e = &(b->Edat);

  // 通常の敵の処理 //
  Enemies.CheckInterrupts(e);
  Enemies.Execute(e);

  // 弾発射モードによる分岐 //
  if (e->t_rep) {
    e->tama_c = (e->tama_c + 1) % (e->t_rep);
    if (e->tama_c == 0) {
      Bullets.command =e->t_cmd;
      Bullets.command.x += e->x;
      Bullets.command.y += e->y;
      Bullets.Spawn();
    }
  }

  switch (b->ExState) {
  case (BEXST_WING01):
    if (b->ExCount < 64 + 16 + 8)
      b->ExCount++;
    break;
  }
}

// ボスの体力の総和を求める //
uint32_t BossManager::GetHPSum(void) {
  uint32_t HP_Sum = 0;
  EnemyData *e;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);
      HP_Sum += e->hp;
    }
  }

  return HP_Sum;
}

// ボス用割り込み処理 //
void BossManager::Interrupt(EnemyData *e, uint8_t IntID) {
  auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  // 割り込み番号による分岐 //
  switch (IntID) {
  case (ECLINT_SNAKEON):
    SnakySet(&*b, 30, 11);
    break;

  case (ECLINT_LBWING01): // 蝶の羽も描画する
    b->ExState = BEXST_WING01;
    b->ExCount = 0;
    break;

  case (ECLINT_LBWING02): // 鳥の羽も描画する
    b->ExState = BEXST_WING02;
    b->ExCount = 0;
    break;

  case (ECLINT_BITON5):
    BitSet(&*b,5, 3);
    break;

  case (ECLINT_BITON6):
    BitSet(&*b,6, 3);
    break;

  case (ECLINT_SHILD1):
    b->ExState = BEXST_SHILD1;
    break;

  case (ECLINT_SHILD2):
    b->ExState = BEXST_SHILD2;
    break;
  }
}

// ビット攻撃アドレス指定 //
void BossManager::BitAttack(EnemyData *e, uint32_t AtkID) {
  const auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  BitSelectAttack(AtkID);
}

// ビットにレーザーコマンドセット //
void BossManager::BitLaser(EnemyData *e, uint8_t cmd) {
  const auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  BitLaserCommand(cmd);
}

// ビット命令送信 //
void BossManager::BitCommand(EnemyData *e, uint8_t Cmd, int Param) {
  const auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  BitSendCommand(Cmd, Param);
}

// 残りビット数を返す //
int BossManager::GetBitLeft(void) { return BitGetNum(); }
