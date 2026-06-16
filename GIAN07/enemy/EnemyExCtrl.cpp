/*                                                                           */
/*   EnemyExCtrl.cpp   敵用の特殊処理                                        */
/*                                                                           */
/*                                                                           */

#include "EnemyExCtrl.h"
#include "boss_manager.h"
#include "enemy_manager.h"
#include "LLASER.h"
#include "laser_manager.h"
#include "LOADER.h"
#include "MAID.h"
#include "player_manager.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

static constexpr auto BIT_VIRTUAL_HP = 990000; // ビットの仮想ＨＰ

// SnakeData, BitData moved to BossManager struct

// 蛇型の敵配列の初期化 //
void BossManager::SnakyInit(void) {
  // 全ての蛇さんをダメダメにするの //
  for (auto &it : this->snake_data) {
    it.bIsUse = false;
    it.Parent = nullptr;
  }
}

// 蛇型の敵をセットする //
void BossManager::SnakySet(BOSS_DATA *b, int len, uint32_t TailID) {
  ENEMY_DATA *e;

  auto s =
      std::ranges::find_if(this->snake_data, [](const auto &s) { return !s.bIsUse; });
  if (s == std::end(this->snake_data)) {
    return;
  }

  s->bIsUse = true;
  s->Parent = b;
  s->Head = 0;

  // Hardcoded to 30 in the original game. No way around dynamic allocation
  // if mods ever want to safely customize it.
  assert(s->Length() == len);

  // ここでは頂点バッファの初期化を行うのだ //
  // なお、ループ中断値は後で変更のこと     //
  for (auto &point : s->PointBuffer) {
    point.x = b->Edat.x;
    point.y = b->Edat.y;
    point.d = b->Edat.d;
  }

  const auto n = (4 + (TailID << 2));
  for (auto &enemy_ptr : s->EnemyPtr) {
    if (EnemyNow + 1 < ENEMY_MAX) {
      e = &Enemy[EnemyInd[EnemyNow++]];

      Enemies.InitDataX64(e, b->Edat.x, b->Edat.y, n);
      enemy_ptr = e;
    } else {
      enemy_ptr = nullptr; // ポインタを無効化
    }
  }
}

// 蛇型の敵の移動処理 //
void BossManager::SnakyMove(void) {
  ENEMY_DATA *e;

  for (auto &it : this->snake_data) {
    auto *s = &it;
    if (s->bIsUse == false) {
      continue;
    }

    // バッファ更新処理 //
    using DATA_TYPE = std::remove_reference_t<decltype(*s)>;
    constexpr auto points = (DATA_TYPE::Length() * SNAKEYMOVE_POINTS_PER_ENEMY);
    for (const auto j : std::views::iota(0u, s->Length())) {
      e = s->EnemyPtr[j];
      if (e == nullptr) {
        continue;
      }

      const auto ptr =
          ((s->Head + points - (j * SNAKEYMOVE_POINTS_PER_ENEMY)) % points);

      e->x = s->PointBuffer[ptr].x;
      e->y = s->PointBuffer[ptr].y;
      e->d = s->PointBuffer[ptr].d;
    }

    s->Head = ((s->Head + 1) % points);
    s->PointBuffer[s->Head].x = s->Parent->Edat.x;
    s->PointBuffer[s->Head].y = s->Parent->Edat.y;
    s->PointBuffer[s->Head].d = s->Parent->Edat.d;
  }
}

// 蛇型の敵を殺す
void BossManager::SnakyDelete(const BOSS_DATA *b) {
  auto s = std::ranges::find_if(this->snake_data,
                                [b](const auto &s) { return (s.Parent == b); });
  if (s == std::end(this->snake_data)) {
    return;
  }

  for (auto &e : s->EnemyPtr) {
    if (e == nullptr) {
      break;
    }

    // Snd_SEPlay(SOUND_ID_BOMB, e->x);
    if (e->LLaserRef)
      Lasers.ForceCloseLong(e); // レーザーの強制クローズ
    // PowerUp(e->hp);			// パワーアップ
    e->hp = 0;
    e->count = 0;
    e->flag = EF_BOMB;
    /// score_add(e->score);
    // Items.Spawn(e->x,e->y,0);
  }

  s->bIsUse = false;
  s->Parent = nullptr;
}

// ビット配列の初期化 //
void BossManager::BitInit(void) {
  int i;

  // ここら辺の初期化には、あまり意味がない //
  this->bit_data.x = 0;
  this->bit_data.y = 0;
  this->bit_data.BaseAngle = 0;
  this->bit_data.Length = 0;
  this->bit_data.FinalLength = 0;
  this->bit_data.v = 0;
  this->bit_data.d = 64;
  this->bit_data.BitSpeed = 0;
  this->bit_data.NumBits = 0;
  //	this->bit_data.DeltaAngle  = 0;
  this->bit_data.LaserState = BLASERCMD_DISABLE;
  this->bit_data.bIsLaserEnable = false;
  //	this->bit_data.ForceCount  = 0;

  // ここから下の初期化がメインとなる //
  this->bit_data.State = BITCMD_DISABLE;
  this->bit_data.Parent = nullptr;

  for (i = 0; i < BIT_MAX; i++) {
    this->bit_data.Bit[i].pEnemy = nullptr; // 敵データへのポインタ
    this->bit_data.Bit[i].Angle = 0;        // 現在の角度
    this->bit_data.Bit[i].Force = 0;        // その他の？力の加えられている方向
    this->bit_data.Bit[i].BitID = i;        // ビットの先頭からの番号
    this->bit_data.Bit[i].BitHP = 0;        // ビットの耐久度
  }
}

// ビットをセットする //
void BossManager::BitSet(BOSS_DATA *b, uint8_t NumBits, uint32_t BitID) {
  static const uint8_t BitHPTable[BIT_MAX] = {1, 4, 2, 5, 3, 6};

  int i;

  // 他の関数と違い、不等号なので注意すべし
  // このビット構造体が有効な場合、この関数の実行はできないので
  // すぐ、リターンする
  if (this->bit_data.State != BITCMD_DISABLE)
    return;

  // ビット数が不正である //
  if (NumBits == 0 || NumBits > BIT_MAX)
    return;

  this->bit_data.State = BITCMD_STDMOVE;
  this->bit_data.Parent = b;

  this->bit_data.x = b->Edat.x;
  this->bit_data.y = b->Edat.y;

  this->bit_data.Length = 0;
  this->bit_data.FinalLength = 64 * 80;
  this->bit_data.NumBits = NumBits;
  this->bit_data.BitSpeed = ((rnd() >> 1) & 1) ? 2 : -2;
  //	this->bit_data.DeltaAngle  = (256*256)/NumBits;
  this->bit_data.BaseAngle = 0; //(256*256)/NumBits;//0;
  this->bit_data.LaserState = BLASERCMD_DISABLE;
  this->bit_data.bIsLaserEnable = false;
  //	this->bit_data.ForceCount  = 0;

  const auto n = (4 + (BitID << 2));

  for (i = 0; i < NumBits; i++) {
    if (EnemyNow + 1 < ENEMY_MAX) {
      // 敵資源の要求 //
      auto *e = &Enemy[EnemyInd[EnemyNow++]];

      // データを初期化 //
      Enemies.InitDataX64(e, this->bit_data.x, this->bit_data.y, n);
      e->hp = BIT_VIRTUAL_HP;
      e->d = i * (256 / NumBits);
      e->GR[0] = i;
      e->GR[1] = NumBits;
      Enemies.ParseECL(e);

      // この構造体と作成した敵を関連づける //
      this->bit_data.Bit[i].pEnemy = e;   // 敵データへのポインタ
      this->bit_data.Bit[i].Angle = e->d; // 現在の角度
      this->bit_data.Bit[i].Force = 0;    // その他の？力の加えられている方向
      this->bit_data.Bit[i].BitID = i;    // ビットの先頭からの番号

      this->bit_data.Bit[i].BitHP = 95 * BitHPTable[i]; // ビットの耐久度
    } else {
      this->bit_data.Bit[i].pEnemy = nullptr;
    }
  }
}

// ビットを動作させる //
void BossManager::BitMove(void) {
  int i, j;
  ENEMY_DATA *e;
  bool bIsDestroyed = false;

  if (this->bit_data.NumBits == 0)
    return;

  switch (this->bit_data.State) {
  case BITCMD_STDMOVE:
    this->bit_data.x = this->bit_data.Parent->Edat.x;
    this->bit_data.y = this->bit_data.Parent->Edat.y;

    this->BitSTDRad();
    this->BitSTDRoll();
    break;

  case BITCMD_MOVTARGET:
    this->bit_data.v += this->bit_data.a;
    this->bit_data.x += cosl(this->bit_data.d, this->bit_data.v);
    this->bit_data.y += sinl(this->bit_data.d, this->bit_data.v);

    if (this->bit_data.v <= -64 * 10)
      this->bit_data.State = BITCMD_STDMOVE;

    this->BitSTDRad();
    this->BitSTDRoll();
    break;

  case BITCMD_DISABLE:
  default:
    return;
  }

  // レーザー放出中はダメージが無効化される //
  if (this->bit_data.bIsLaserEnable) {
    // 敵のＨＰを仮想ＨＰに回復？させる //
    // -> ダメージを蓄積させたい場合は、下のforを注釈化する事 //
    for (i = 0; i < this->bit_data.NumBits; i++) {
      this->bit_data.Bit[i].pEnemy->hp = BIT_VIRTUAL_HP;
    }
    return;
  }

  // this->bit_data.NumBits は、削除が行われるとその数が減ることに注意 //
  for (i = 0; i < this->bit_data.NumBits; i++) {
    e = this->bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }

    const uint32_t damage = (BIT_VIRTUAL_HP - e->hp);
    if (this->bit_data.Bit[i].BitHP <= damage) {
      bIsDestroyed = true;

      // ビット配列に関連づけられた敵に削除要求を送出 //
      if (e->LLaserRef)
        Lasers.ForceCloseLong(e);
      e->hp = 0;
      e->count = 0; // 爆発のアニメセット用
      e->flag = EF_BOMB;

      Snd_SEPlay(SOUND_ID_BOMB, e->x);

      for (j = i + 1; j < this->bit_data.NumBits; j++) {
        this->bit_data.Bit[j - 1] = this->bit_data.Bit[j];
        this->bit_data.Bit[j - 1].BitID--;
      }

      // 基本角となっていたビットが破壊された場合 //
      if (i == 0) {
        this->bit_data.BaseAngle += (256 / this->bit_data.NumBits);
      }

      // ビットの総数を減らす //
      this->bit_data.NumBits--;

      // ビット無効状態に推移する //
      if (this->bit_data.NumBits == 0) {
        this->bit_data.State = BITCMD_DISABLE;
      }

      // 破壊されたビットの前後に力を加える                 //
      // 注意：この時点で総数はすでにデクリメントされている //
      if (this->bit_data.NumBits) {
        j = i - 1 + this->bit_data.NumBits;
        this->bit_data.Bit[j % this->bit_data.NumBits].Force -= 30;
        this->bit_data.Bit[i % this->bit_data.NumBits].Force += 30;
      }
      // this->bit_data.ForceCount += 60;

      // ビットの消去を行ったので、もう一度 i 番目には次のデータが格納されている
      // // したがって、次のビットの参照に移行するために、i をデクリメントする
      // //
      i--;
    } else {
      // 敵のＨＰを仮想ＨＰに回復？させる //
      e->hp = BIT_VIRTUAL_HP;

      // 真の意味で、ダメージを与えるところ //
      this->bit_data.Bit[i].BitHP -= damage;
    }
  }

  // レジスタ更新 //
  for (i = 0; i < this->bit_data.NumBits; i++) {
    e = this->bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }
    e->GR[1] = this->bit_data.NumBits;
  }
}

// 基本的な半径処理
void BossManager::BitSTDRad(void) {
  if (this->bit_data.Length > this->bit_data.FinalLength) {
    this->bit_data.Length -= 64 * 2;

    if (this->bit_data.Length < this->bit_data.FinalLength)
      this->bit_data.Length = this->bit_data.FinalLength;
  } else if (this->bit_data.Length < this->bit_data.FinalLength) {
    this->bit_data.Length += 64 * 2;

    if (this->bit_data.Length > this->bit_data.FinalLength)
      this->bit_data.Length = this->bit_data.FinalLength;
  }
}

// 基本的なビット回転処理 //
void BossManager::BitSTDRoll(void) {
  int i, ox, oy;
  int n, l;

  int dir;
  uint8_t LaserDeg;

  ENEMY_DATA *e;
  BIT_PARAM *bit;

  if (this->bit_data.NumBits == 0)
    return;

  this->bit_data.BaseAngle += this->bit_data.BitSpeed;

  //	if(this->bit_data.ForceCount) this->bit_data.ForceCount--;

  n = this->bit_data.NumBits;
  l = this->bit_data.Length;

  ox = this->bit_data.x;
  oy = this->bit_data.y;

  const uint8_t delta = (256 / this->bit_data.NumBits);
  const uint8_t ExSpeed = abs(this->bit_data.BitSpeed / 2);

  /*	if((this->bit_data.DeltaAngle / 256) < delta){
                  this->bit_data.DeltaAngle += 64;
          }
  */

  // d       : そのビットが目標とする角度
  // delta   : ビット間の理想とする角度(収束する角度)
  // ExSpeed : そのビットの回転速度の絶対値＋１
  for (i = 0; i < n; i++) {
    bit = this->bit_data.Bit + i;
    e = bit->pEnemy;
    if (e == nullptr) {
      continue;
    }

    // 目標とする角度を求める //
    const uint8_t d = ((this->bit_data.BaseAngle >> 1) + (delta * bit->BitID));

    // 通常の角度収束処理 //
    dir = (Cast::up_sign<int>(d) - Cast::up_sign<int>(bit->Angle));

    if (dir < -128)
      dir += 256;
    else if (dir > 128)
      dir -= 256;

    if (dir > 0) {
      if (dir > 2)
        dir = 2;
      // if(this->bit_data.ForceCount)        bit->Angle += min(dir, 2);
      if (this->bit_data.BitSpeed > 0)
        bit->Angle += max(dir, ExSpeed);
      else
        bit->Angle += max(dir, (ExSpeed + 1));
      //			if(dir > 2) bit->Angle+=ExSpeed;
      //			else        bit->Angle+=(ExSpeed-1);
      //			char	buf[100];
      //			sprintf(buf, "dir = %d    ExSpeed = %d", dir,
      // ExSpeed); 			DebugOut(buf);
    } else if (dir < 0) {
      if (dir < -2)
        dir = -2;
      //			if(this->bit_data.ForceCount)        bit->Angle -=
      // min(-dir, 2);
      if (this->bit_data.BitSpeed < 0)
        bit->Angle -= max(-dir, ExSpeed);
      else
        bit->Angle -= max(-dir, (ExSpeed + 1));
      //			if(dir < -2) bit->Angle-=ExSpeed;
      //			else         bit->Angle-=(ExSpeed-1);
      //			char	buf[100];
      //			sprintf(buf, "dir = %d    ExSpeed = %d", dir,
      // ExSpeed); 			DebugOut(buf);
    }

    // 力による影響を反映する //
    if (bit->Force > 0) {
      bit->Force--;
      if (this->bit_data.BitSpeed > 0)
        bit->Angle++;
      else
        bit->Angle += (ExSpeed + 1);
      // Sleep(100);
    } else if (bit->Force < 0) {
      bit->Force++;
      if (this->bit_data.BitSpeed < 0)
        bit->Angle--;
      else
        bit->Angle -= (ExSpeed + 1);
      // Sleep(100);
    }

    e->d = bit->Angle;
    e->x = ox + cosl(e->d, l);
    e->y = oy + sinl(e->d, l);

    // レーザーコマンドの反映 //
    switch (this->bit_data.LaserState) {
    case (BLASERCMD_TYPE_A): // 一方向・角度固定レーザーを放射
    case (BLASERCMD_TYPE_B): // 両方向角度固定レーザーを放射
      break;

    case (BLASERCMD_TYPE_C): // 角度同期ｎ芒星レーザー
      if (this->bit_data.NumBits == 0)
        break;
      LaserDeg = 64 + 256 / this->bit_data.NumBits;
      Lasers.RotateLongAbs(e, e->d + LaserDeg, 0);
      Lasers.RotateLongAbs(e, e->d - LaserDeg, 1);
      break;
    }
  }
}

// ビットを消滅させる //
void BossManager::BitDelete(void) {
  int i;
  ENEMY_DATA *e;

  if (this->bit_data.State == BITCMD_DISABLE)
    return;

  // 各ビットを消滅させる //
  for (i = 0; i < this->bit_data.NumBits; i++) {
    e = this->bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }

    if (e->LLaserRef)
      Lasers.ForceCloseLong(e);
    e->hp = 0;
    e->count = 0;
    e->flag = EF_BOMB;

    Snd_SEPlay(SOUND_ID_BOMB, e->x);
  }

  // 後は、この関数に任せる //
  this->BitInit();
}

// ビット間のラインを描画する //
void BossManager::BitLineDraw(void) {
  int i, j, n;
  int x1, x2, y1, y2;
  ENEMY_DATA *RefTable[BIT_MAX * 2];

  if (this->bit_data.State == BITCMD_DISABLE)
    return;

  n = this->bit_data.NumBits;
  if (n == 0)
    return;

  for (i = 0, j = -1; i < n; i++) {
    while (this->bit_data.Bit[++j].pEnemy == nullptr) {
    }

    RefTable[i] = RefTable[i + n] = this->bit_data.Bit[j].pEnemy;
  }

  GrpGeom->Lock();
  GrpGeom->SetColor({4, 4, 5});

  for (i = 0; i < n; i++) {
    if (n >= 5)
      j = i + 2;
    else
      j = i + 1;

    x1 = RefTable[i]->x >> 6;
    y1 = RefTable[i]->y >> 6;
    x2 = RefTable[j]->x >> 6;
    y2 = RefTable[j]->y >> 6;
    GrpGeom->DrawLine(x1, y1, x2, y2);
  }

  GrpGeom->Unlock();
}

// 攻撃パターンをセットor変更 //
void BossManager::BitSelectAttack(uint32_t BitID) {
  int i;

  const auto n = (4 + (BitID << 2));

  for (i = 0; i < this->bit_data.NumBits; i++) {
    Enemies.ECL_LongJump(this->bit_data.Bit[i].pEnemy, n);
  }
}

// レーザー系命令を発行 //
void BossManager::BitLaserCommand(uint8_t Command) {
  int i;
  ENEMY_DATA *e;
  uint8_t delta;

  Lasers.long_cmd.dx = 0;
  Lasers.long_cmd.dy = 0;
  Lasers.long_cmd.v = 64;
  Lasers.long_cmd.w = 64 * 8;

  this->bit_data.bIsLaserEnable = true;

  for (i = 0; i < this->bit_data.NumBits; i++) {
    e = this->bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }

    Lasers.long_cmd.e = e;
    Lasers.long_cmd.d = e->d;

    switch (Command) {
    case (BLASERCMD_TYPE_A): // 一方向・角度固定レーザーを放射
      Lasers.long_cmd.type = LLS_LONG;
      Lasers.long_cmd.c = 2;
      if (Lasers.SpawnLongLaser(e->LLaserRef))
        e->LLaserRef++;
      break;

    case (BLASERCMD_TYPE_B): // 両方向角度固定レーザーを放射
      Lasers.long_cmd.d += 64;
      Lasers.long_cmd.type = LLS_LONG;
      Lasers.long_cmd.c = 1;
      if (Lasers.SpawnLongLaser(e->LLaserRef))
        e->LLaserRef++;

      Lasers.long_cmd.d += 128;
      if (Lasers.SpawnLongLaser(e->LLaserRef))
        e->LLaserRef++;
      break;

    case (BLASERCMD_TYPE_C): // 角度同期ｎ芒星レーザー
      Lasers.long_cmd.type = LLS_LONG;
      Lasers.long_cmd.c = 0;

      delta = 64 + 256 / this->bit_data.NumBits;

      Lasers.long_cmd.d = e->d + delta;
      if (Lasers.SpawnLongLaser(e->LLaserRef))
        e->LLaserRef++;
      Lasers.long_cmd.d = e->d - delta;
      if (Lasers.SpawnLongLaser(e->LLaserRef))
        e->LLaserRef++;
      break;

    case (BLASERCMD_OPEN):
      Lasers.OpenLong(e, ECLCST_LLASERALL);
      continue;

    case (BLASERCMD_CLOSE):
      Lasers.CloseLong(e, ECLCST_LLASERALL);
      e->LLaserRef = 0;
      this->bit_data.bIsLaserEnable = false;
      continue;

    case (BLASERCMD_CLOSEL):
      Lasers.LineLong(e, ECLCST_LLASERALL);
      continue;
    }

    this->bit_data.LaserState = Command;
  }
}

// ビット命令を送信 //
void BossManager::BitSendCommand(uint8_t Command, int Param) {
  switch (Command) {
  case (BITCMD_CHGSPD): // 回転速度を変更する
    // 同じ方向で、速度を変更する
    if (Param > 0) {
      if (this->bit_data.BitSpeed > 0)
        this->bit_data.BitSpeed = Param;
      else
        this->bit_data.BitSpeed = -Param;
    }
    // 回転方向を反転し、速度を変更する
    else {
      if (this->bit_data.BitSpeed > 0)
        this->bit_data.BitSpeed = Param;
      else
        this->bit_data.BitSpeed = -Param;
    }
    break;

  case (BITCMD_CHGRADIUS): // 半径を変更する
    this->bit_data.FinalLength = Param;
    break;

  case (BITCMD_MOVTARGET): // 目標(びびっと)に向けてブーメラン移動
    this->bit_data.v = 64 * 10;
    this->bit_data.a = -8;
    this->bit_data.d = atan8(Players.viv.x - this->bit_data.x, Players.viv.y - this->bit_data.y);
    this->bit_data.State = BITCMD_MOVTARGET;
    break;

  default:
    return;
  }
}

// 現在のビット数を取得する //
int BossManager::BitGetNum(void) {
  if (this->bit_data.State == BITCMD_DISABLE)
    return 0;
  return this->bit_data.NumBits;
}
