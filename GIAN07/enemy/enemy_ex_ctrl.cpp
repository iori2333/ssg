///
/// EnemyExCtrl.cpp   Extra enemy (boss) control
///

#include <algorithm>
#include <cstddef>

#include "enemy_ex_ctrl.h"

#include "boss_manager.h"
#include "enemy_manager.h"

#include "audio/snd.h"
#include "bullet/long_laser.h"
#include "core/loader.h"
#include "core/world.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"
#include <utility>

static constexpr auto BIT_VIRTUAL_HP = 990000; // Bit virtual HP

// SnakeData, BitData moved to BossManager struct

// Initialize snake-type enemy array
void BossManager::SnakyInit() {
  // Deactivate all snakes
  for (auto &it : snake_data) {
    it.bIsUse = false;
    it.Parent = nullptr;
  }
}

// Set snake-type enemy
void BossManager::SnakySet(BossData *b, int len, uint32_t TailID) {
  EnemyData *e = nullptr;

  auto *s =
      std::ranges::find_if(snake_data, [](const auto &s) { return !s.bIsUse; });
  if (s == std::end(snake_data)) {
    return;
  }

  s->bIsUse = true;
  s->Parent = b;
  s->Head = 0;

  // Hardcoded to 30 in the original game. No way around dynamic allocation
  // if mods ever want to safely customize it.
  assert(s->Length() == len);

  // Initialize vertex buffer here
  // Note: loop break value to be changed later
  for (auto &point : s->PointBuffer) {
    point.x = b->Edat.x;
    point.y = b->Edat.y;
    point.d = ut_math_detail::deg256_to_rad(b->Edat.d);
  }

  const auto n = (4 + (TailID << 2));
  for (auto &enemy_ptr : s->EnemyPtr) {
    if (Enemies.count + 1 < ENEMY_MAX) {
      e = &Enemies.entities[Enemies.indices[Enemies.count++]];

      Enemies.InitDataX64(e, b->Edat.x, b->Edat.y, n);
      enemy_ptr = e;
    } else {
      enemy_ptr = nullptr; // Invalidate pointer
    }
  }
}

// Snake-type enemy movement processing
void BossManager::SnakyMove() {
  EnemyData *e = nullptr;

  for (auto &it : snake_data) {
    auto *s = &it;
    if (!s->bIsUse) {
      continue;
    }

    // Buffer update processing
    using DATA_TYPE = std::remove_reference_t<decltype(*s)>;
    constexpr auto points = (DATA_TYPE::Length() * SNAKEYMOVE_POINTS_PER_ENEMY);
    for (const auto j : std::views::iota(0U, SNAKYMOVE_DATA<30>::Length())) {
      e = s->EnemyPtr[j];
      if (e == nullptr) {
        continue;
      }

      const auto ptr =
          ((s->Head + points -
            (static_cast<size_t>(j * SNAKEYMOVE_POINTS_PER_ENEMY))) %
           points);

      e->x = s->PointBuffer[ptr].x;
      e->y = s->PointBuffer[ptr].y;
      e->d = ut_math_detail::rad_to_deg256(s->PointBuffer[ptr].d);
    }

    s->Head = ((s->Head + 1) % points);
    s->PointBuffer[s->Head].x = s->Parent->Edat.x;
    s->PointBuffer[s->Head].y = s->Parent->Edat.y;
    s->PointBuffer[s->Head].d = ut_math_detail::deg256_to_rad(s->Parent->Edat.d);
  }
}

// Kill snake-type enemy
void BossManager::SnakyDelete(const BossData *b) {
  auto *s = std::ranges::find_if(
      snake_data, [b](const auto &s) { return (s.Parent == b); });
  if (s == std::end(snake_data)) {
    return;
  }

  for (auto &e : s->EnemyPtr) {
    if (e == nullptr) {
      break;
    }

    // Snd_SEPlay(SOUND_ID_BOMB, e->x);
    if (e->LLaserRef != 0U) {
      gWorld().projectiles.Long().ForceCloseLong(e); // Force close laser
    }
    // PowerUp(e->hp);			// Power up
    e->hp = 0;
    e->count = 0;
    e->flag = EF_BOMB;
    /// score_add(e->score);
    // Items.Spawn(e->x,e->y,0);
  }

  s->bIsUse = false;
  s->Parent = nullptr;
}

// Initialize bit array
void BossManager::BitInit() {
  int i = 0;

  // This initialization doesn't mean much
  bit_data.x = 0;
  bit_data.y = 0;
  bit_data.BaseAngle = 0;
  bit_data.Length = 0;
  bit_data.FinalLength = 0;
  bit_data.v = 0;
  bit_data.d = 64;
  bit_data.BitSpeed = 0;
  bit_data.NumBits = 0;
  //	bit_data.DeltaAngle  = 0;
  bit_data.LaserState = BLASERCMD_DISABLE;
  bit_data.bIsLaserEnable = false;
  //	bit_data.ForceCount  = 0;

  // Initialization below this is the main part
  bit_data.State = BITCMD_DISABLE;
  bit_data.Parent = nullptr;

  for (i = 0; i < BIT_MAX; i++) {
    bit_data.Bit[i].pEnemy = nullptr; // Pointer to enemy data
    bit_data.Bit[i].Angle = 0;        // Current angle
    bit_data.Bit[i].Force = 0;        // Other force direction
    bit_data.Bit[i].BitID = i;        // Bit index from start
    bit_data.Bit[i].BitHP = 0;        // Bit durability
  }
}

// Set bits
void BossManager::BitSet(BossData *b, uint8_t NumBits, uint32_t BitID) {
  static const uint8_t BitHPTable[BIT_MAX] = {1, 4, 2, 5, 3, 6};

  int i = 0;

  // Unlike other functions, note the inequality
  // If this bit structure is active, this function cannot execute
  // So return immediately
  if (bit_data.State != BITCMD_DISABLE) {
    return;
  }

  // Invalid bit count
  if (NumBits == 0 || NumBits > BIT_MAX) {
    return;
  }

  bit_data.State = BITCMD_STDMOVE;
  bit_data.Parent = b;

  bit_data.x = b->Edat.x;
  bit_data.y = b->Edat.y;

  bit_data.Length = 0;
  bit_data.FinalLength = 64 * 80;
  bit_data.NumBits = NumBits;
  bit_data.BitSpeed = (((rnd() >> 1) & 1) != 0) ? 2 : -2;
  //	bit_data.DeltaAngle  = (256*256)/NumBits;
  bit_data.BaseAngle = 0; //(256*256)/NumBits;//0;
  bit_data.LaserState = BLASERCMD_DISABLE;
  bit_data.bIsLaserEnable = false;
  //	bit_data.ForceCount  = 0;

  const auto n = (4 + (BitID << 2));

  for (i = 0; std::cmp_less(i, NumBits); i++) {
    if (Enemies.count + 1 < ENEMY_MAX) {
      // Request enemy resource
      auto *e = &Enemies.entities[Enemies.indices[Enemies.count++]];

      // Initialize data
      Enemies.InitDataX64(e, bit_data.x, bit_data.y, n);
      e->hp = BIT_VIRTUAL_HP;
      e->d = ut_math_detail::deg256_to_rad(static_cast<uint8_t>(i * (256 / NumBits)));
      e->GR[0] = i;
      e->GR[1] = NumBits;
      Enemies.Execute(e);

      // Associate this structure with the created enemy
      bit_data.Bit[i].pEnemy = e;   // Pointer to enemy data
      bit_data.Bit[i].Angle = ut_math_detail::rad_to_deg256(e->d); // Current angle
      bit_data.Bit[i].Force = 0;    // Other force direction
      bit_data.Bit[i].BitID = i;    // Bit index from start

      bit_data.Bit[i].BitHP = 95 * BitHPTable[i]; // Bit durability
    } else {
      bit_data.Bit[i].pEnemy = nullptr;
    }
  }
}

// Move bits
void BossManager::BitMove() {
  int i = 0;
  int j = 0;
  EnemyData *e = nullptr;
  bool bIsDestroyed = false;

  if (bit_data.NumBits == 0) {
    return;
  }

  switch (bit_data.State) {
  case BITCMD_STDMOVE:
    bit_data.x = bit_data.Parent->Edat.x;
    bit_data.y = bit_data.Parent->Edat.y;

    BitSTDRad();
    BitSTDRoll();
    break;

  case BITCMD_MOVTARGET:
    bit_data.v += bit_data.a;
    bit_data.x += cosl(bit_data.d, bit_data.v);
    bit_data.y += sinl(bit_data.d, bit_data.v);

    if (bit_data.v <= -64 * 10) {
      bit_data.State = BITCMD_STDMOVE;
    }

    BitSTDRad();
    BitSTDRoll();
    break;

  case BITCMD_DISABLE:
  default:
    return;
  }

  // Damage is nullified during laser emission
  if (bit_data.bIsLaserEnable) {
    // Restore enemy HP to virtual HP
    // -> To accumulate damage, comment out the for loop below
    for (i = 0; std::cmp_less(i, bit_data.NumBits); i++) {
      bit_data.Bit[i].pEnemy->hp = BIT_VIRTUAL_HP;
    }
    return;
  }

  // Note: bit_data.NumBits decreases when bits are removed
  for (i = 0; std::cmp_less(i, bit_data.NumBits); i++) {
    e = bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }

    const uint32_t damage = (BIT_VIRTUAL_HP - e->hp);
    if (bit_data.Bit[i].BitHP <= damage) {
      bIsDestroyed = true;

      // Send deletion request to enemy associated with bit array
      if (e->LLaserRef != 0U) {
        gWorld().projectiles.Long().ForceCloseLong(e);
      }
      e->hp = 0;
      e->count = 0; // For explosion animation set
      e->flag = EF_BOMB;

      Snd_SEPlay(SOUND_ID_BOMB, e->x);

      for (j = i + 1; std::cmp_less(j, bit_data.NumBits); j++) {
        bit_data.Bit[j - 1] = bit_data.Bit[j];
        bit_data.Bit[j - 1].BitID--;
      }

      // When the bit at the base angle is destroyed
      if (i == 0) {
        bit_data.BaseAngle += (256 / bit_data.NumBits);
      }

      // Decrease total bit count
      bit_data.NumBits--;

      // Transition to bit disabled state
      if (bit_data.NumBits == 0) {
        bit_data.State = BITCMD_DISABLE;
      }

      // Apply force before and after the destroyed bit
      // Note: count is already decremented at this point
      if (bit_data.NumBits != 0U) {
        j = i - 1 + bit_data.NumBits;
        bit_data.Bit[j % bit_data.NumBits].Force -= 30;
        bit_data.Bit[i % bit_data.NumBits].Force += 30;
      }
      // bit_data.ForceCount += 60;

      // The bit was erased, so the next data is now at index i
      // Therefore, decrement i to move to the next bit reference
      //
      i--;
    } else {
      // Restore enemy HP to virtual HP
      e->hp = BIT_VIRTUAL_HP;

      // Where actual damage is applied
      bit_data.Bit[i].BitHP -= damage;
    }
  }

  // Update registers
  for (i = 0; std::cmp_less(i, bit_data.NumBits); i++) {
    e = bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }
    e->GR[1] = bit_data.NumBits;
  }
}

// Basic radius processing
void BossManager::BitSTDRad() {
  if (bit_data.Length > bit_data.FinalLength) {
    bit_data.Length -= 64 * 2;

    bit_data.Length = std::max(bit_data.Length, bit_data.FinalLength);
  } else if (bit_data.Length < bit_data.FinalLength) {
    bit_data.Length += 64 * 2;

    bit_data.Length = std::min(bit_data.Length, bit_data.FinalLength);
  }
}

// Basic bit rotation processing
void BossManager::BitSTDRoll() {
  int i = 0;
  int ox = 0;
  int oy = 0;
  int n = 0;
  int l = 0;

  int dir = 0;
  uint8_t LaserDeg = 0;

  EnemyData *e = nullptr;
  BitParam *bit = nullptr;

  if (bit_data.NumBits == 0) {
    return;
  }

  bit_data.BaseAngle += bit_data.BitSpeed;

  //	if(bit_data.ForceCount) bit_data.ForceCount--;

  n = bit_data.NumBits;
  l = bit_data.Length;

  ox = bit_data.x;
  oy = bit_data.y;

  const int delta = (256 / bit_data.NumBits);
  const int ExSpeed = abs(bit_data.BitSpeed / 2);

  //	if((bit_data.DeltaAngle / 256) < delta){
  //		bit_data.DeltaAngle += 64;
  //	}

  // d       : Target angle for the bit
  // delta   : Ideal angle between bits (convergence angle)
  // ExSpeed : Absolute rotation speed of the bit + 1
  for (i = 0; i < n; i++) {
    bit = bit_data.Bit + i;
    e = bit->pEnemy;
    if (e == nullptr) {
      continue;
    }

    // Find target angle
    const uint8_t d = ((bit_data.BaseAngle >> 1) + (delta * bit->BitID));

    // Normal angle convergence processing
    dir = (Cast::up_sign<int>(d) - Cast::up_sign<int>(bit->Angle));

    if (dir < -128) {
      dir += 256;
    } else if (dir > 128) {
      dir -= 256;
    }

    if (dir > 0) {
      dir = std::min(dir, 2);
      // if(bit_data.ForceCount)        bit->Angle += min(dir, 2);
      if (bit_data.BitSpeed > 0) {
        bit->Angle += std::max(dir, ExSpeed);
      } else {
        bit->Angle += std::max(dir, (ExSpeed + 1));
      }
      //			if(dir > 2) bit->Angle+=ExSpeed;
      //			else        bit->Angle+=(ExSpeed-1);
      //			char	buf[100];
      //			sprintf(buf, "dir = %d    ExSpeed = %d", dir,
      // ExSpeed); 			DebugOut(buf);
    } else if (dir < 0) {
      dir = std::max(dir, -2);
      //			if(bit_data.ForceCount)        bit->Angle -=
      // min(-dir, 2);
      if (bit_data.BitSpeed < 0) {
        bit->Angle -= std::max(-dir, ExSpeed);
      } else {
        bit->Angle -= std::max(-dir, (ExSpeed + 1));
      }
      //			if(dir < -2) bit->Angle-=ExSpeed;
      //			else         bit->Angle-=(ExSpeed-1);
      //			char	buf[100];
      //			sprintf(buf, "dir = %d    ExSpeed = %d", dir,
      // ExSpeed); 			DebugOut(buf);
    }

    // Reflect force influence
    if (bit->Force > 0) {
      bit->Force--;
      if (bit_data.BitSpeed > 0) {
        bit->Angle++;
      } else {
        bit->Angle += (ExSpeed + 1);
      }
      // Sleep(100);
    } else if (bit->Force < 0) {
      bit->Force++;
      if (bit_data.BitSpeed < 0) {
        bit->Angle--;
      } else {
        bit->Angle -= (ExSpeed + 1);
      }
      // Sleep(100);
    }

    e->d = ut_math_detail::deg256_to_rad(bit->Angle);
    e->x = ox + cos_len(e->d, l);
    e->y = oy + sin_len(e->d, l);

    // Reflect laser command
    switch (bit_data.LaserState) {
    case BLASERCMD_TYPE_A: // Emit unidirectional fixed-angle laser
    case BLASERCMD_TYPE_B: // Emit bidirectional fixed-angle laser
      break;

    case BLASERCMD_TYPE_C: // Angle-synchronized n-point star laser
      if (bit_data.NumBits == 0) {
        break;
      }
      LaserDeg = 64 + (256 / bit_data.NumBits);
      gWorld().projectiles.Long().RotateLongAbs(
          e, ut_math_detail::rad_to_deg256(e->d) + LaserDeg, 0);
      gWorld().projectiles.Long().RotateLongAbs(
          e, ut_math_detail::rad_to_deg256(e->d) - LaserDeg, 1);
      break;
    }
  }
}

// Destroy bits
void BossManager::BitDelete() {
  int i = 0;
  EnemyData *e = nullptr;

  if (bit_data.State == BITCMD_DISABLE) {
    return;
  }

  // Destroy each bit
  for (i = 0; std::cmp_less(i, bit_data.NumBits); i++) {
    e = bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }

    if (e->LLaserRef != 0U) {
      gWorld().projectiles.Long().ForceCloseLong(e);
    }
    e->hp = 0;
    e->count = 0;
    e->flag = EF_BOMB;

    Snd_SEPlay(SOUND_ID_BOMB, e->x);
  }

  // Delegate the rest to this function
  BitInit();
}

// Draw lines between bits
void BossManager::BitLineDraw() {
  int i = 0;
  int j = 0;
  int n = 0;
  int x1 = 0;
  int x2 = 0;
  int y1 = 0;
  int y2 = 0;
  EnemyData *RefTable[BIT_MAX * 2];

  if (bit_data.State == BITCMD_DISABLE) {
    return;
  }

  n = bit_data.NumBits;
  if (n == 0) {
    return;
  }

  for (i = 0, j = -1; i < n; i++) {
    while (bit_data.Bit[++j].pEnemy == nullptr) {
    }

    RefTable[i] = RefTable[i + n] = bit_data.Bit[j].pEnemy;
  }

  GrpGeom->Lock();
  GrpGeom->SetColor({4, 4, 5});

  for (i = 0; i < n; i++) {
    if (n >= 5) {
      j = i + 2;
    } else {
      j = i + 1;
    }

    x1 = RefTable[i]->x >> 6;
    y1 = RefTable[i]->y >> 6;
    x2 = RefTable[j]->x >> 6;
    y2 = RefTable[j]->y >> 6;
    GrpGeom->DrawLine(x1, y1, x2, y2);
  }

  GrpGeom->Unlock();
}

// Set or change attack pattern
void BossManager::BitSelectAttack(uint32_t BitID) {
  int i = 0;

  const auto n = (4 + (BitID << 2));

  for (i = 0; std::cmp_less(i, bit_data.NumBits); i++) {
    Enemies.LongJump(bit_data.Bit[i].pEnemy, n);
  }
}

// Issue laser commands
void BossManager::BitLaserCommand(uint8_t Command) {
  int i = 0;
  EnemyData *e = nullptr;
  uint8_t delta = 0;

  bullets::LongLaserCommand lcmd{};
  lcmd.dx = 0;
  lcmd.dy = 0;
  lcmd.v = 64;
  lcmd.w = 64 * 8;

  bit_data.bIsLaserEnable = true;

  for (i = 0; std::cmp_less(i, bit_data.NumBits); i++) {
    e = bit_data.Bit[i].pEnemy;
    if (e == nullptr) {
      continue;
    }

    lcmd.e = e;
    lcmd.d = ut_math_detail::rad_to_deg256(e->d);

    switch (Command) {
    case BLASERCMD_TYPE_A: // Emit unidirectional fixed-angle laser
      lcmd.type = LLS_LONG;
      lcmd.c = 2;
      if (gWorld().projectiles.Long().SpawnLongLaser(lcmd, e->LLaserRef)) {
        e->LLaserRef++;
      }
      break;

    case BLASERCMD_TYPE_B: // Emit bidirectional fixed-angle laser
      lcmd.d += 64;
      lcmd.type = LLS_LONG;
      lcmd.c = 1;
      if (gWorld().projectiles.Long().SpawnLongLaser(lcmd, e->LLaserRef)) {
        e->LLaserRef++;
      }

      lcmd.d += 128;
      if (gWorld().projectiles.Long().SpawnLongLaser(lcmd, e->LLaserRef)) {
        e->LLaserRef++;
      }
      break;

    case BLASERCMD_TYPE_C: // Angle-synchronized n-point star laser
      lcmd.type = LLS_LONG;
      lcmd.c = 0;

      delta = 64 + (256 / bit_data.NumBits);

      lcmd.d = ut_math_detail::rad_to_deg256(e->d) + delta;
      if (gWorld().projectiles.Long().SpawnLongLaser(lcmd, e->LLaserRef)) {
        e->LLaserRef++;
      }
      lcmd.d = ut_math_detail::rad_to_deg256(e->d) - delta;
      if (gWorld().projectiles.Long().SpawnLongLaser(lcmd, e->LLaserRef)) {
        e->LLaserRef++;
      }
      break;

    case BLASERCMD_OPEN:
      gWorld().projectiles.Long().OpenLong(e, ECLCST_LLASERALL);
      continue;

    case BLASERCMD_CLOSE:
      gWorld().projectiles.Long().CloseLong(e, ECLCST_LLASERALL);
      e->LLaserRef = 0;
      bit_data.bIsLaserEnable = false;
      continue;

    case BLASERCMD_CLOSEL:
      gWorld().projectiles.Long().LineLong(e, ECLCST_LLASERALL);
      continue;
    }

    bit_data.LaserState = Command;
  }
}

// Send bit command
void BossManager::BitSendCommand(uint8_t Command, int Param) {
  switch (Command) {
  case BITCMD_CHGSPD: // Change rotation speed
    // Change speed in the same direction
    if (Param > 0) {
      if (bit_data.BitSpeed > 0) {
        bit_data.BitSpeed = Param;
      } else {
        bit_data.BitSpeed = -Param;
      }
    }
    // Reverse rotation direction and change speed
    else {
      if (bit_data.BitSpeed > 0) {
        bit_data.BitSpeed = Param;
      } else {
        bit_data.BitSpeed = -Param;
      }
    }
    break;

  case BITCMD_CHGRADIUS: // Change radius
    bit_data.FinalLength = Param;
    break;

  case BITCMD_MOVTARGET: // Boomerang move toward target (Vivit)
    bit_data.v = 64 * 10;
    bit_data.a = -8;
    bit_data.d = atan8(Players.X() - bit_data.x, Players.Y() - bit_data.y);
    bit_data.State = BITCMD_MOVTARGET;
    break;

  default:
    return;
  }
}

// Get current bit count
int BossManager::BitGetNum() const {
  if (bit_data.State == BITCMD_DISABLE) {
    return 0;
  }
  return bit_data.NumBits;
}
