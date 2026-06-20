///
/// ENEMY.C - Enemy management and spawn control
///

#include "enemy.h"

#include "core/entity.h"
#include "ecl_len.h"
#include "enemy_manager.h"
#include "game/cast.h"
#include "game/debug.h"
#include "game/endian.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "gian.h"
#include "level.h"
#include "platform/graphics_backend.h"
#include <utility>

// ECL debug macro
static void ECL_DEBUG(const char *s, auto param) {
#ifdef SCRIPT_TRACE
  char _ECL_Debug[1000];
  const auto size = snprintf(_ECL_Debug, sizeof(_ECL_Debug), s, param);
  if (size <= 0) {
    return;
  }
  DebugLog({_ECL_Debug,
            static_cast<size_t>(std::min(size, (int)(sizeof(_ECL_Debug) - 1)))});
#endif
}

// Variable entities moved to EnemyManager in enemy_manager.cpp
// References below are for backward compatibility:
// Enemy, indices, count, ecl_head, scl_head, scl_now, Anime,
// homing_x, homing_y, homing_flag — defined as references in enemy_manager.cpp

// Enemies.enemy_exdeg, Enemies.enemy_exdeg_d moved to
// EnemyManager in enemy_manager.cpp

// Functions
static void EnemyDrawBomb(int x, int y, uint32_t count);

// (Indsort<EnemyData> wrapper removed — pass predicate directly)

// Convert from ECLCST_?? to its value
static uint32_t ID2Value(const EnemyData *e, uint8_t id);

void EnemyManager::UpdateHoming(const EnemyData *e) {
  const int temp = (Players.Y() - e->y);

  if (temp < 0) {
    return;
  }

  if (temp < homing_flag) {
    homing_flag = temp;
    homing_x = e->x;
    homing_y = e->y;
  }
}

bool EnemyManager::EnemyManager::LaserHITCHK(const EnemyData *e, int ox, int oy,
                                             uint8_t d) {
  const int chkw = (std::min(e->g_height, e->g_width) + (3 * 64));

  const int tx = (e->x - ox);
  const int ty = (e->y - oy);

  const int l = (cosl(d, tx) + sinl(d, ty));
  const int w = abs(-sinl(d, tx) + cosl(d, ty));

  return ((l > 0) && (w < chkw));
}

void EnemyData::Draw() const {
  constexpr auto sid = SURFACE_ID::ENEMY;

  // TODO: Remove once the structure itself uses WORLD_POINT.
  const WORLD_POINT center = {&x, &y};

  const auto &a = Enemies.anime[anm_ptn];
  const auto topleft = center.ToPixel(a.size); // Coordinate set

  // Drawing mode selection
  const auto &src =
      ((a.mode == ANM_DEG) ? a.ptn[static_cast<uint8_t>(d - 64 + 8) >> 4]
                           : a.ptn[anm_c]);
  if (GrpSurface_Blit({topleft.x, topleft.y}, sid, src)) {
    if ((anm_ptn != anm_ptnEx) && (IsDamaged != 0U)) {
      const auto &a = Enemies.anime[anm_ptnEx];
      const auto topleft = center.ToPixel(a.size); // Coordinate set
      GrpSurface_Blit({topleft.x, topleft.y}, sid, a.ptn[0]);
    }
  }
}

void EnemyData::UpdateAnimation() { Enemies.UpdateAnimation(this); }

void EnemyManager::Move() {
  int i = 0; //,chkx,chky;

  if (Bosses.count == 0) {
    homing_flag = HOMING_DUMMY;
  }

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];
    e->IsDamaged = 0;
    if ((e->flag & EF_BOMB) == 0) {
      // Normal enemy processing
      CheckInterrupts(e);
      Execute(e);

      // Branch by bullet fire mode
      if ((e->t_rep != 0U) && (e->hp != 0U)) {
        e->tama_c = (e->tama_c + 1) % (e->t_rep);
        if (e->tama_c == 0) {
          Bullets.command = e->t_cmd;
          Bullets.command.x += e->x;
          Bullets.command.y += e->y;
          Bullets.Spawn();
        }
      }

      // Cactus hit check
      if (HITCHK(e->x, Players.X(), e->g_width) &&
          HITCHK(e->y, Players.Y(), e->g_height) && Players.IsInvincible() == 0) {
        // Might be interesting to damage the enemy around here?
        if ((e->flag & EF_HITSB) != 0) {
          Players.OnHit();
        }
      }

      // Out-of-bounds check
      if ((e->y < GY_MIN - (e->g_height)) || (e->y > GY_MAX + (e->g_height)) ||
          (e->x < GX_MIN - (e->g_width)) || (e->x > GX_MAX + (e->g_width))) {
        if ((e->flag & EF_CLIP) == 0) {
          if (e->LLaserRef != 0U) {
            Lasers.ForceCloseLong(e);
          }
          e->flag = EF_DELETE;
        }
      }
    } else if (e->count >= (8 * ENEMY_BOMB_SPD) - 1) {
      e->flag = EF_DELETE;
    }

    // Homing preparation
    if ((Bosses.count == 0) && ((e->flag & EF_DAMAGE) != 0)) {
      UpdateHoming(e);
    }

    // Animation update
    UpdateAnimation(e);

    e->count++;
  }

  Indsort(indices, count, entities,
          [](const EnemyData &e) { return (e.flag & EF_DELETE); });
}

void EnemyManager::Draw() {
  int i = 0;
  int x = 0;
  int y = 0;
  // HRESULT		ddrval;

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];

    // Draw enemy (add clipping & width, height handling)
    x = (e->x >> 6);
    y = (e->y >> 6);
    if (e->flag == EF_BOMB) {
      EnemyDrawBomb(x, y, e->count);
      continue;
    }

    if ((e->flag & EF_DRAW) != 0) {
      e->Draw();
    }
  }
}

// Clear small enemies
void EnemyManager::Clear() {
  int i = 0;

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];
    if (e->flag == EF_BOMB) {
      continue;
    }

    if ((e->flag & EF_DRAW) != 0) {
      e->flag = EF_BOMB;
      e->hp = 0;
      e->count = 0;
      if (e->LLaserRef != 0U) {
        Lasers.ForceCloseLong(e); // Force close laser
      }
      Snd_SEPlay(SOUND_ID_BOMB, e->x);
    } else {
      // Erasing non-drawing type enemies differs from other cases:
      // do not play explosion animation/sound
      e->flag = EF_DELETE;
      e->hp = 0;
      e->count = 0;
      if (e->LLaserRef != 0U) {
        Lasers.ForceCloseLong(e); // Force close laser
      }
      // Do not play explosion sound
    }
  }

  Indsort(indices, count, entities,
          [](const EnemyData &e) { return (e.flag & EF_DELETE); });
}

void EnemyManager::InitIndices() {
  int i = 0;

  for (i = 0; std::cmp_less(i, ENEMY_MAX); i++) {
    // memset(Enemy+i,0,sizeof(Enemy));
    indices[i] = i;
  }

  count = 0;
}

bool EnemyManager::ApplyDamage(EnemyData &e, int damage) {
  e.IsDamaged = ((e.count) & 1);
  if (std::cmp_less_equal(e.hp, damage)) {
    Snd_SEPlay(SOUND_ID_BOMB, e.x);
    if (e.LLaserRef != 0U) {
        Lasers.ForceCloseLong(&e); // Force close laser
    }
    Players.PowerUp(static_cast<uint8_t>(e.hp)); // Power up
    e.hp = 0;
    e.count = 0;
    e.flag = EF_BOMB;
    Players.AddScore(e.score);
    if (e.item != 0U) {
      Items.Spawn(e.x, e.y, e.item);
    }
  } else {
    Snd_SEPlay(SOUND_ID_HIT, e.x);
    Players.PowerUp(damage); // Power up here too
    e.hp -= damage;
  }
  return true;
}

bool EnemyManager::DamageAt(int x, int y, int damage) {
  int i = 0;

  if (Bosses.DamageAt(x, y, damage)) {
    return true;
  }

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];
    if (HITCHK(x, e->x, e->g_width) && HITCHK(y, e->y, e->g_height) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      return ApplyDamage(*e, damage);
    }
  }

  return false;
}

bool EnemyManager::DamageAt2(int x, int y, int damage) {
  int i = 0;
  auto ret_val = Bosses.DamageAt2(x, y, damage);

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];
    if (HITCHK(x, e->x, e->g_width) && (y > e->y) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ret_val = ApplyDamage(*e, damage);
    }
  }

  return ret_val;
}

// Diagonal laser hit detection
void EnemyManager::DamageAt3(int x, int y, uint8_t d) {
  int i = 0;
  // bool	ret_val = false;
  constexpr int damage = 8;

  Bosses.DamageAt3(x, y, d);

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];
    if (EnemyManager::LaserHITCHK(e, x, y, d) && ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ApplyDamage(*e, damage);
    }
  }
}

// Damage all enemies
void EnemyManager::DamageAll(int damage) {
  int i = 0;

  Bosses.DamageAll(damage);

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *e = &entities[indices[i]];
    if ((e->flag & EF_DAMAGE) != 0) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ApplyDamage(*e, damage);
      // return true;
    }
  }

  // return false;
}

// Initialize enemy data (x,y specified as x64)
void EnemyManager::InitDataX64(EnemyData *e, int x, int y, uint32_t EclID) {
  e->x = x;
  e->y = y;

  e->cmd = U32LEAt(&ecl_head[EclID]);

  e->call_addr = e->cmd;

  e->hp = 0xffffffff;
  e->amp = 0;
  e->anm_ptn = 0;
  e->anm_ptnEx = 0; // Added: 2000/11/27 (animation while damaged)
  e->anm_sp = 0;
  e->anm_c = 0;
  e->count = 0;
  e->evscore = 0;
  e->d = 64;
  e->flag = EF_DAMAGE | EF_DRAW | EF_HITSB;

  e->IsDamaged = 0;

  e->tama_c = Cast::down<uint8_t>(rnd()); // & 0xff;
  e->t_rep = 0;                           // Bullet fire interval (0: no auto-fire)
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

  // Initialize variable registers
  e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
  e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;

  // Initialize interrupt vectors
  InitInterrupts(e);
}

// Force move between ECL blocks
void EnemyManager::LongJump(EnemyData *e, uint32_t EclID) {
  e->cmd = U32LEAt(&ecl_head[EclID]);

  e->call_addr = e->cmd;

  e->t_rep = 0; // Bullet fire interval (0: no auto-fire)
  e->rep_c = 0;
  e->cmd_c = 0;
}

// Initialize enemy data (x,y specified as non-x64, random possible)
void EnemyManager::InitDataSTD(EnemyData *e, short x, short y, uint32_t EclID) {
  int EnemyX = x;
  int EnemyY = y;
  // e->x   = I16LEAt(&p[0]);	// PixelToWorld(I16LEAt(&p[0]));
  // e->y   = I16LEAt(&p[2]);	// PixelToWorld(I16LEAt(&p[2]));

  // Handle random placement
  EnemyX = (EnemyX == X_RNDV) ? GX_RND() : (EnemyX << 6);
  EnemyY = (EnemyY == Y_RNDV) ? GY_RND() : (EnemyY << 6);

  InitDataX64(e, EnemyX, EnemyY, EclID);
}

void EnemyManager::UpdateAnimation(EnemyData *e) {
  const auto *a = &anime[e->anm_ptn];

  switch (a->mode) {
  case ANM_NORM:
    if (e->anm_sp > 0 && (e->count % e->anm_sp == 0)) {
      e->anm_c = (e->anm_c + 1) % (a->n);
    } else if (e->anm_sp < 0 && (e->count % (-e->anm_sp) == 0)) {
      e->anm_c = (e->anm_c + a->n - 1) % (a->n);
    }
    break;

  // Reverse direction is not allowed...
  case ANM_STOP:
    if (e->anm_sp > 0 && (e->count % e->anm_sp == 0)) {
      if (e->anm_c < (a->n - 1)) {
        e->anm_c++;
      }
    }
    break;

  default:
    break;
  }
}

static void EnemyDrawBomb(int x, int y, uint32_t count) {
  PIXEL_LTRB src;

  src.top = 296;
  src.left = (count / ENEMY_BOMB_SPD) * 48;
  src.bottom = 296 + 48;
  src.right = src.left + 48;

  x -= 24;
  y -= 24;

  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
}

void EnemyManager::Execute(EnemyData *e) {
  // Lambda for left-right reversal (formerly macros ABS_DEGRL/ABS_VXRL/REL_DEGRL)
  auto AbsDegRL = [e](uint8_t d) -> uint8_t {
    return (e->flag & EF_RLCHG) ? (128 - d) : d;
  };
  auto AbsVxRL = [e](int vx) { return (e->flag & EF_RLCHG) ? (-vx) : vx; };
  auto RelDegRL = [e](int8_t d) -> int8_t {
    return (e->flag & EF_RLCHG) ? static_cast<int8_t>(-d) : d;
  };

  bool bRetFlag = false; // Set to false for execution clock 0 instructions
  int RegCmp = 0;
  HomingLaserInfo HInfo{};

  const PIXEL_LTRB rcDegX2 = {
      GX_MIN + (150 * 64), GY_MIN + ((GY_MID - GY_MIN - (40 * 64)) / 3),
      GX_MAX - (150 * 64),
      GY_MID - ((GY_MID - GY_MIN - (40 * 64)) / 3) - (40 * 64)};
  uint16_t BaseAngle = 0;
  uint16_t DeltaAngle = 0;

// A GOTO label in a place like this!!
ECL_HEAD:
  bRetFlag = true;
  auto *cmd = (ecl_head.data() + e->cmd);

  switch (*cmd) {
  case ECL_CEFC: {
    const auto x = (e->x + PixelToWorld(I16LEAt(&cmd[1 + 0])));
    const auto y = (e->y + PixelToWorld(I16LEAt(&cmd[1 + 2])));
    Effects.SpawnCircleEffect(x, y, cmd[1 + 2 + 2]);
    bRetFlag = false;
  } break;

  case ECL_XYRND:
    if (e->x > GX_MID) {
      e->x = (X_MID * 64) - ((rnd() % (X_MAX - X_MIN - 100)) * 32);
    } else {
      e->x = (X_MID * 64) + ((rnd() % (X_MAX - X_MIN - 100)) * 32);
    }

    e->y = ((rnd() % (Y_MID - Y_MIN - 160)) * 64) + ((Y_MIN + 40) * 64);
    bRetFlag = false;
    break;

  case ECL_XYL: // 1+2 Bytes Param
    e->x += cosl(e->d, PixelToWorld(I16LEAt(&cmd[1])));
    e->y += sinl(e->d, PixelToWorld(I16LEAt(&cmd[1])));
    bRetFlag = false;
    break;

  case ECL_STG4EFC:
    switch (cmd[1]) {
    case STG4ROCK_STDMOVE:
      Effects.SendCmdStg4Rocks(cmd[1], 0);
      break;
    case STG4ROCK_ACCMOVE1:
      Effects.SendCmdStg4Rocks(cmd[1], 0);
      break;
    case STG4ROCK_ACCMOVE2:
      Effects.SendCmdStg4Rocks(cmd[1], e->d);
      break;
    case STG4ROCK_3DMOVE:
      Effects.SendCmdStg4Rocks(cmd[1], 0);
      break;
    case STG4ROCK_LEAVE:
      Effects.SendCmdStg4Rocks(cmd[1], 0);
      break;
    case STG4ROCK_END:
      Effects.SendCmdStg4Rocks(cmd[1], 0);
      break;
    }

    bRetFlag = false;
    break;

  case ECL_STG3EFC:
    Scroller.Command(SCMD_STG3STAR);
    break;

  case ECL_ITEM:
    e->item = cmd[1];
    break;

  case ECL_HITXY: // Change hit detection
    e->g_width = PixelToWorld(U16LEAt(&cmd[1]));
    e->g_height = PixelToWorld(U16LEAt(&cmd[3]));
    bRetFlag = false;
    break;

  case ECL_HLASER: // Homing laser set
    HInfo.c = e->l_cmd.c;
    HInfo.d = e->l_cmd.d;
    HInfo.dw = e->l_cmd.dw;
    HInfo.n = e->l_cmd.n;
    HInfo.type = e->l_cmd.type;
    HInfo.x = e->x + e->l_cmd.x;
    HInfo.y = e->y + e->l_cmd.y;
    Lasers.SpawnHoming(&HInfo);
    break;

  case ECL_LLSET: // Long laser set
    Lasers.long_cmd.c = e->l_cmd.c;
    Lasers.long_cmd.d = e->l_cmd.d;
    Lasers.long_cmd.dx = e->l_cmd.x;
    Lasers.long_cmd.dy = e->l_cmd.y;
    Lasers.long_cmd.e = e;
    Lasers.long_cmd.type = e->l_cmd.type;
    // Lasers.long_cmd.type = (e->l_cmd.type==0) ? LLS_LONG : LLS_SETDEG;
    Lasers.long_cmd.v = e->l_cmd.v;
    Lasers.long_cmd.w = e->l_cmd.w;

    // Do not increment reference count on failure
    if (Lasers.SpawnLongLaser(e->LLaserRef)) {
      e->LLaserRef++;
    }
    bRetFlag = false;
    break;

  case ECL_LLOPEN: // Long laser open cmd,id
    Lasers.OpenLong(e, cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LLCLOSE: // Long laser close (delete & decrement ref count) cmd,id
    Lasers.CloseLong(e, cmd[1]);
    if (cmd[1] == ECLCST_LLASERALL) {
      e->LLaserRef = 0;
    } else {
      e->LLaserRef -= 1; // Beware: there's a slight bug here
    }
    bRetFlag = false;
    break;

  case ECL_LLCLOSEL: // Long laser to line state cmd,id
    Lasers.LineLong(e, cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LLDEGR: // Long laser relative angle change cmd,id,deg
    // Order is reversed, so be careful
    Lasers.RotateLongRel(e, Cast::sign<int8_t>(cmd[2]), cmd[1]);
    bRetFlag = false;
    break;

  case ECL_SETUP: // Enemy initialization
    ECL_DEBUG("ECL_SETUP", 0);
    e->hp = U32LEAt(&cmd[1 + 0]);
    e->score = U32LEAt(&cmd[1 + 4]);
    if (e->hp == 0) {
      Bosses.KillAll();
    }
    bRetFlag = false;
    break;

  case ECL_END: // Force enemy deletion
    ECL_DEBUG("ECL_END", 0);
    if (e->LLaserRef != 0U) {
      Lasers.ForceCloseLong(e); // Force close laser
    }
    e->flag = EF_DELETE; // To be changed later
    return;              // Bug prevention (maybe)

  case ECL_JMP: // ECL unconditional jump (slightly special behavior)
    ECL_DEBUG("ECL_JMP", 0);
    e->cmd = U32LEAt(&cmd[1]);
    goto ECL_HEAD;

  case ECL_LOOP: // Repeat a certain interval
    ECL_DEBUG("ECL_LOOP : %d", e->rep_c);
    if (e->rep_c == 0) {
      e->rep_c = (U16LEAt(&cmd[1 + 4]) + 1);
    }
    if ((--e->rep_c) != 0) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    }
    bRetFlag = false;
    break;

  case ECL_CALL: // Call subroutine
    ECL_DEBUG("ECL_CALL", 0);
    e->call_addr = e->cmd + ECL_CmdLen[ECL_CALL];
    e->cmd = U32LEAt(&cmd[1]);
    goto ECL_HEAD;

  case ECL_RET: // Return from subroutine
    ECL_DEBUG("ECL_RET", 0);
    e->cmd = e->call_addr;
    goto ECL_HEAD;

  case ECL_JHPL: { // Jump if HP is greater than specified value
    const auto target = U32LEAt(&cmd[1]);
    ECL_DEBUG("ECL_JHPL : %u", target);
    if (e->hp > U32LEAt(&cmd[1 + 4])) {
      e->cmd = target;
      goto ECL_HEAD;
    }
    bRetFlag = false;
  } break;

  case ECL_JHPS: { // Jump if HP is less than specified value
    const auto target = U32LEAt(&cmd[1]);
    ECL_DEBUG("ECL_JHPS : %u", target);
    if (e->hp < U32LEAt(&cmd[1 + 4])) {
      e->cmd = target;
      goto ECL_HEAD;
    }
    bRetFlag = false;
  } break;

  case ECL_JDIF: // Jump by difficulty
    ECL_DEBUG("ECL_JDIF", 0);
    switch (Ranking.state.GameLevel) {
    case GameLevel::EASY:
      e->cmd = U32LEAt(&cmd[1 + 0]);
      break;
    default:
    case GameLevel::NORMAL:
      e->cmd = U32LEAt(&cmd[1 + 4]);
      break;
    case GameLevel::HARD:
      e->cmd = U32LEAt(&cmd[1 + 8]);
      break;
    case GameLevel::LUNATIC:
      e->cmd = U32LEAt(&cmd[1 + 12]);
      break;
    }
    goto ECL_HEAD;

  case ECL_JDSB: { // Jump if player heading angle matches
    ECL_DEBUG("ECL_JDSB", 0);
    const uint8_t temp =
        abs(atan8((Players.X() - e->x), (Players.Y() - e->y)) - (e->d));
    if (temp < 4) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    }
    bRetFlag = false;
  } break;

  case ECL_JFCL: // Jump if frame counter is greater
    ECL_DEBUG("ECL_JFCL", 0);
    if (e->count > U32LEAt(&cmd[1 + 4])) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    }
    bRetFlag = false;
    break;

  case ECL_JFCS: // Jump if frame counter is smaller
    ECL_DEBUG("ECL_JFCS", 0);
    if (e->count < U32LEAt(&cmd[1 + 4])) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    }
    bRetFlag = false;
    break;

  case ECL_STI: // Set interrupt vector Addr(4),condition(1),compare(4)
    switch (cmd[1 + 4]) {
    case ECLVECT_BITLEFT:
      e->Vect[ECLVECT_BITLEFT].vect = U32LEAt(&cmd[1]);
      e->Vect[ECLVECT_BITLEFT].value = U32LEAt(&cmd[1 + 4 + 1]);
      break;

    case ECLVECT_BOSSLEFT:
      e->Vect[ECLVECT_BOSSLEFT].vect = U32LEAt(&cmd[1]);
      e->Vect[ECLVECT_BOSSLEFT].value = U32LEAt(&cmd[1 + 4 + 1]);
      break;

    case ECLVECT_HP:
      e->Vect[ECLVECT_HP].vect = U32LEAt(&cmd[1]);
      e->Vect[ECLVECT_HP].value = U32LEAt(&cmd[1 + 4 + 1]);
      break;

    case ECLVECT_TIMER:
      e->Vect[ECLVECT_TIMER].vect = U32LEAt(&cmd[1]);
      e->Vect[ECLVECT_TIMER].value = U32LEAt(&cmd[1 + 4 + 1]);
      e->IntTimer = 0;
      break;

    default:
      ECL_DEBUG("Invalid access to interrupt vector %d", cmd[1 + 4]);
      break;
    }
    bRetFlag = false;
    break;

  case ECL_CLI: // Clear interrupt vector
    switch (cmd[1]) {
    case ECLVECT_BITLEFT:
      e->Vect[ECLVECT_BITLEFT].vect = 0;
      break;

    case ECLVECT_BOSSLEFT:
      e->Vect[ECLVECT_BOSSLEFT].vect = 0;
      break;

    case ECLVECT_HP:
      e->Vect[ECLVECT_HP].vect = 0;
      break;

    case ECLVECT_TIMER:
      e->Vect[ECLVECT_TIMER].vect = 0;
      break;
    }
    bRetFlag = false;
    break;

  case ECL_NOP: // Do nothing
    ECL_DEBUG("ECL_NOP : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1]) + 1);
    }
    if ((--e->cmd_c) != 0) {
      return;
    }
    bRetFlag = false;
    break;

  case ECL_NOPSC: // Carried by scroll
    ECL_DEBUG("ECL_NOPSC : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1]) + 1);
    }
    if ((--e->cmd_c) != 0) {
      // Processing to be carried by scroll
      return;
    }
    bRetFlag = false;
    break;

  case ECL_T2ITEM: // Convert some % of bullets to items
    Bullets.ToItems(cmd[1]);
    bRetFlag = false;
    break;

  case ECL_ACC: // Accelerated movement
    ECL_DEBUG("ECL_ACC : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      // Initialization
      e->cmd_c = (U16LEAt(&cmd[2]) + 1);
      // e->vx    = cosl(e->d,e->v);
      // e->vy    = sinl(e->d,e->v);
    }
    if ((--e->cmd_c) != 0) {
      e->v += Cast::sign<int8_t>(cmd[1]);
      e->x += cosl(e->d, e->v);
      e->y += sinl(e->d, e->v);

      return;
    }
    // In the end, it does nothing...
    bRetFlag = false;
    break;

  case ECL_ACCXYA: // XY-specified accelerated movement
                   // Hold on a sec...
    break;

  case ECL_DEGX2: // Limited random angle
    if (e->y < rcDegX2.top) {
      if (e->x < rcDegX2.left) {
        // Top-left
        BaseAngle = 32 - 16; // 0;
        DeltaAngle = 32;     // 64;
      } else if (e->x > rcDegX2.right) {
        // Top-right
        BaseAngle = 96 - 16; // 64;
        DeltaAngle = 32;     // 64;
      } else {
        // Top edge
        // BaseAngle  = 24+(rnd()>>1)%(64-16)-16;//0;
        BaseAngle = 32 + (((rnd() >> 1) & 1) * 64) - 16;
        DeltaAngle = 32; // 128;
      }
    } else if (e->y > rcDegX2.bottom) {
      if (e->x < rcDegX2.left) {
        // Bottom-left
        BaseAngle = -32 - 16; // 192;
        DeltaAngle = 32;      // 64;
      } else if (e->x > rcDegX2.right) {
        // Bottom-right
        BaseAngle = 128 + 32 - 16; // 128;
        DeltaAngle = 32;           // 64;
      } else {
        // Bottom edge
        BaseAngle = 128 + 64 - 16; // 128;
        DeltaAngle = 32;           // 128;
      }
    } else {
      if (e->x < rcDegX2.left) {
        // Left side
        BaseAngle = -16; // 192;
        DeltaAngle = 32; // 128;
      } else if (e->x > rcDegX2.right) {
        // Right side
        BaseAngle = 128 - 16; // 64;
        DeltaAngle = 32;      // 128;
      } else {
        // Center
        BaseAngle = (((rnd() >> 1) & 1) != 0) ? (-16) : (128 - 16);
        DeltaAngle = 32;
      }
    }

    // Determine actual angle
    e->d = BaseAngle + ((rnd() >> 1) % DeltaAngle);

    bRetFlag = false;
    break;

  case ECL_MOV: // Linear movement
    ECL_DEBUG("ECL_MOV : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1]) + 1);
      e->vx = cosl(e->d, e->v);
      e->vy = sinl(e->d, e->v);
    }
    if ((--e->cmd_c) != 0) {
      e->x += e->vx;
      e->y += e->vy;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_ROL: // Rotational movement
    ECL_DEBUG("ECL_ROL : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 1]) + 1);
      e->vd = RelDegRL(Cast::sign<int8_t>(cmd[1]));
    }
    if ((--e->cmd_c) != 0) {
      e->x += cosl(e->d, e->v);
      e->y += sinl(e->d, e->v);
      e->d += e->vd;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_LROL: // Rotation & linear movement
    ECL_DEBUG("ECL_LROL : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 9]) + 1);
      e->vx = AbsVxRL(I32LEAt(&cmd[1]));
      e->vy = I32LEAt(&cmd[1 + 4]);
      e->vd = RelDegRL(static_cast<char>(cmd[1 + 8]));
    }
    if ((--e->cmd_c) != 0) {
      e->x += (cosl(e->d, e->v) + e->vx);
      e->y += (sinl(e->d, e->v) + e->vy);
      e->d += e->vd;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_WAVX: // Wave X movement
    ECL_DEBUG("ECL_WAVX : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 6]) + 1);
      e->vx = AbsVxRL(I32LEAt(&cmd[1]));
      e->vy = e->y;
      e->amp = cmd[1 + 4];
      e->vd = Cast::sign<int8_t>(cmd[1 + 5]);
      // e->d     = 0;
    }
    if ((--e->cmd_c) != 0) {
      e->x += e->vx;
      e->y = e->vy + sinl(e->d, e->amp << 6);
      e->d += e->vd;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_WAVY: // Wave Y movement
    ECL_DEBUG("ECL_WAVY : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 6]) + 1);
      e->vy = I32LEAt(&cmd[1]);
      e->vx = e->x;
      e->amp = cmd[1 + 4];
      e->vd = Cast::sign<int8_t>(cmd[1 + 5]);
      // e->d     = 0;
    }
    if ((--e->cmd_c) != 0) {
      e->y += e->vy;
      e->x = e->vx + sinl(e->d, e->amp << 6);
      e->d += e->vd;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_MXA: // X absolute movement
    ECL_DEBUG("ECL_MXA : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 2]) + 1);
      e->vx = ((PixelToWorld(U16LEAt(&cmd[1])) - e->x) / e->cmd_c);
      e->vy = 0;
    }
    if ((--e->cmd_c) != 0) {
      e->x += e->vx;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_MYA: // Y absolute movement
    ECL_DEBUG("ECL_MYA : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 2])) + 1;
      e->vy = ((PixelToWorld(U16LEAt(&cmd[1])) - e->y) / e->cmd_c);
      e->vx = 0;
    }
    if ((--e->cmd_c) != 0) {
      e->y += e->vy;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_MXYA: // XY absolute movement
    ECL_DEBUG("ECL_MXYA : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1 + 4]) + 1);
      e->vx = ((PixelToWorld(U16LEAt(&cmd[1 + 0])) - e->x) / e->cmd_c);
      e->vy = ((PixelToWorld(U16LEAt(&cmd[1 + 2])) - e->y) / e->cmd_c);
    }
    if ((--e->cmd_c) != 0) {
      e->x += e->vx;
      e->y += e->vy;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_MXS: // X cactus set movement
    ECL_DEBUG("ECL_MXS : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1]) + 1);
      e->vx = ((Players.X()) - (e->x)) / e->cmd_c;
      e->vy = 0;
    }
    if ((--e->cmd_c) != 0) {
      e->x += e->vx;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_MYS: // Y cactus set movement
    ECL_DEBUG("ECL_MYS : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1]) + 1);
      e->vx = 0;
      e->vy = ((Players.Y()) - (e->y)) / e->cmd_c;
    }
    if ((--e->cmd_c) != 0) {
      e->y += e->vy;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_MXYS: // XY cactus set movement
    ECL_DEBUG("ECL_MXYS : %d", e->cmd_c);
    if (e->cmd_c == 0) {
      e->cmd_c = (U16LEAt(&cmd[1]) + 1);
      e->vx = ((Players.X()) - (e->x)) / e->cmd_c;
      e->vy = ((Players.Y()) - (e->y)) / e->cmd_c;
    }
    if ((--e->cmd_c) != 0) {
      e->x += e->vx;
      e->y += e->vy;
      return;
    }
    bRetFlag = false;
    break;

  case ECL_GRAX: // Gravity X reflection movement (including Y_MIN...)
    // Note: There is no way to escape this instruction except interrupts
    if (e->cmd_c == 0) {
      e->cmd_c = 9999; // Any non-zero value is fine
      e->vx = cosl(e->d, e->v);
      e->vy = sinl(e->d, e->v);
      e->vd = Cast::sign<int8_t>(cmd[1]); // Gravity acceleration!!
      e->flag |= EF_CLIP; // Automatically set clipping attribute
    } else {
      e->x += e->vx;
      e->y += e->vy;
      e->vy += e->vd;

      // X direction check
      if ((e->x) < GX_MIN || (e->x) > GX_MAX) {
        e->vx = -(e->vx); // Velocity reversal
        e->x += e->vx;
      }
      // Y direction (up) check
      if ((e->y) < GY_MIN) {
        e->vy = -(e->vy); // Reverse velocity
        e->y += e->vy;
      }
      // Y direction (down) check -> goodbye then
      // Only this part takes a wider vertical judgment
      if ((e->y) > GY_MAX + (e->g_height)) {
        e->flag = EF_DELETE; // Disappear
      }
    }
    return;

  case ECL_DEGA: // Absolute angle set
    ECL_DEBUG("ECL_DEGA : %u", cmd[1]);
    e->d = AbsDegRL(cmd[1]);
    bRetFlag = false;
    break;

  case ECL_DEGR: // Relative angle set
    ECL_DEBUG("ECL_DEGR : %d", Cast::sign<int8_t>(cmd[1]));
    e->d += RelDegRL(Cast::sign<int8_t>(cmd[1]));
    bRetFlag = false;
    break;

  case ECL_DEGX: // Random angle set
    ECL_DEBUG("ECL_DEGX", 0);
    e->d = rnd() & 0xff;
    bRetFlag = false;
    break;

  case ECL_DEGXU: // Random angle set (up)
    e->d = 128 + (rnd() & 0x7f);
    bRetFlag = false;
    break;

  case ECL_DEGXD: // Random angle set (down)
    e->d = rnd() & 0x7f;
    bRetFlag = false;
    break;

  case ECL_DEGEX:
    e->d = enemy_exdeg;
    enemy_exdeg += enemy_exdeg_d;
    bRetFlag = false;
    break;

  case ECL_DEGS: // Angle set to player
    ECL_DEBUG("ECL_DEGS", 0);
    e->d = atan8(Players.X() - e->x, Players.Y() - e->y);
    bRetFlag = false;
    break;

  case ECL_SPDA: // Absolute speed set
    ECL_DEBUG("ECL_SPDA", 0);
    e->v = I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_SPDR: // Relative speed set
    ECL_DEBUG("ECL_SPDR", 0);
    e->v += I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_XYA: // Absolute coordinate set
    ECL_DEBUG("ECL_XYA", 0);
    e->x = PixelToWorld(I16LEAt(&cmd[1 + 0]));
    e->y = PixelToWorld(I16LEAt(&cmd[1 + 2]));
    bRetFlag = false;
    break;

  case ECL_XYR: // Relative coordinate set
    ECL_DEBUG("ECL_XYR", 0);
    e->x += PixelToWorld(I16LEAt(&cmd[1 + 0]));
    e->y += PixelToWorld(I16LEAt(&cmd[1 + 2]));
    bRetFlag = false;
    break;

  case ECL_XYS:
    e->x = Players.X();
    e->y = Players.Y();
    bRetFlag = false;
    break;

  case ECL_TAMA: // Fire bullet
    Bullets.command = e->t_cmd;
    Bullets.command.x += e->x;
    Bullets.command.y += e->y;
    Bullets.Spawn();
    bRetFlag = false;
    break;

  case ECL_TAMA2: // Fire bullet (no difficulty change)
    Bullets.command = e->t_cmd;
    Bullets.command.x += e->x;
    Bullets.command.y += e->y;
    Bullets.SpawnEX();
    bRetFlag = false;
    break;

  case ECL_TAMAL: // Fire bullets in a line
    Bullets.command = e->t_cmd;
    Bullets.command.x += e->x;
    Bullets.command.y += e->y;
    Bullets.SpawnLine();
    bRetFlag = false;
    break;

  case ECL_TAMAEX:
    Bullets.command = e->t_cmd;
    Bullets.command.x += e->x;
    Bullets.command.y += e->y;
    Bullets.SpawnExtra01();
    bRetFlag = false;
    break;

  case ECL_TAUTO: // Bullet fire mode change
    e->t_rep = cmd[1];
    bRetFlag = false;
    break;

  case ECL_TXYR: // Relative bullet fire position set
    e->t_cmd.x = PixelToWorld(I16LEAt(&cmd[1 + 0]));
    e->t_cmd.y = PixelToWorld(I16LEAt(&cmd[1 + 2]));
    bRetFlag = false;
    break;

  case ECL_TCMD: // Bullet command
    e->t_cmd.cmd = cmd[1];
    bRetFlag = false;
    break;

  case ECL_TDEGA: // Absolute bullet fire angle
    e->t_cmd.d = cmd[1];
    e->t_cmd.dw = cmd[1 + 1];
    bRetFlag = false;
    break;

  case ECL_TDEGR: // Relative bullet fire angle
    e->t_cmd.d += Cast::sign<int8_t>(cmd[1]);
    e->t_cmd.dw += Cast::sign<int8_t>(cmd[1 + 1]);
    bRetFlag = false;
    break;

  case ECL_TDEGS: // Bullet fire angle cactus set
    // Strictly speaking, should use TamaCmd x,y...
    e->t_cmd.d = atan8(Players.X() - e->x, Players.Y() - e->y);
    bRetFlag = false;
    break;

  case ECL_TDEGE: // Sync bullet fire angle
    e->t_cmd.d = e->d;
    bRetFlag = false;
    break;

  case ECL_TNUMA: // Absolute bullet count
    e->t_cmd.n = cmd[1];
    e->t_cmd.ns = cmd[1 + 1];
    bRetFlag = false;
    break;

  case ECL_TNUMR: // Relative bullet count
    e->t_cmd.n += Cast::sign<int8_t>(cmd[1]);
    e->t_cmd.ns += Cast::sign<int8_t>(cmd[1 + 1]);
    bRetFlag = false;
    break;

  case ECL_TSPDA: // Absolute bullet speed
    e->t_cmd.v = cmd[1];
    e->t_cmd.a = Cast::sign<int8_t>(cmd[1 + 1]);
    bRetFlag = false;
    break;

  case ECL_TSPDR: { // Relative bullet speed
    const auto temp = e->t_cmd.v;

    // Remove flags for calculation
    e->t_cmd.v = (((temp & 0x3f) + Cast::sign<int8_t>(cmd[1])) & 0x3f);

    e->t_cmd.v |= (temp & 0xc0); //(temp&0x3c); // Write back flags
    e->t_cmd.a += Cast::sign<int8_t>(cmd[1 + 1]);
    bRetFlag = false;
  } break;

  case ECL_TOPT: // Bullet option
    e->t_cmd.option = cmd[1];
    bRetFlag = false;
    break;

  case ECL_TTYPE: // Bullet type
    e->t_cmd.type = cmd[1];
    bRetFlag = false;
    break;

  case ECL_TCOL: // Bullet color or shape
    e->t_cmd.c = cmd[1];
    bRetFlag = false;
    break;

  case ECL_TVDEG: // Bullet angular velocity
    e->t_cmd.vd = Cast::sign<int8_t>(cmd[1]);
    bRetFlag = false;
    break;

  case ECL_TREP: // Bullet REP set
    e->t_cmd.rep = cmd[1];
    bRetFlag = false;
    break;

  case ECL_TCLR:       // Clear all enemy bullets (including lasers)
    Bosses.ClearCmd(); // Prioritize this above all (includes bit clearing)
    Bullets.Clear();
    Lasers.Clear();
    Lasers.ClearHoming();
    Clear();
    bRetFlag = false;
    break;

  case ECL_LASER: // Fire laser
    Lasers.cmd = e->l_cmd;
    Lasers.cmd.x += e->x;
    Lasers.cmd.y += e->y;
    Lasers.Spawn();
    bRetFlag = false;
    break;

  case ECL_LASER2: // Fire laser
    Lasers.cmd = e->l_cmd;
    Lasers.cmd.x += e->x;
    Lasers.cmd.y += e->y;
    Lasers.SpawnEX();
    bRetFlag = false;
    break;

  case ECL_LCMD: // Laser command set
    e->l_cmd.cmd = cmd[1];
    bRetFlag = false;
    break;

  case ECL_LLA: // Laser length absolute set
    e->l_cmd.l = I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LLR: // Laser length relative set
    e->l_cmd.l += I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LL2: // Laser fire position
    e->l_cmd.l2 = I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LDEGA: // Laser angle and width absolute set
    e->l_cmd.d = cmd[1];
    e->l_cmd.dw = cmd[2];
    bRetFlag = false;
    break;

  case ECL_LDEGR: // Laser angle and width relative set
    e->l_cmd.d += Cast::sign<int8_t>(cmd[1]);
    e->l_cmd.dw += Cast::sign<int8_t>(cmd[2]);
    bRetFlag = false;
    break;

  case ECL_LDEGS: // Laser fire angle cactus set
    // Strictly speaking, should use LaserCmd x,y...
    e->l_cmd.d = atan8(Players.X() - e->x, Players.Y() - e->y);
    bRetFlag = false;
    break;

  case ECL_LDEGE: // Set laser angle to own direction
    e->l_cmd.d = e->d;
    bRetFlag = false;
    break;

  case ECL_LNUMA: // Absolute laser count
    e->l_cmd.n = cmd[1];
    bRetFlag = false;
    break;

  case ECL_LNUMR: // Relative laser count
    e->l_cmd.n += Cast::sign<int8_t>(cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LSPDA: // Absolute laser speed
    e->l_cmd.v = I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LSPDR: // Relative laser speed
    e->l_cmd.v = I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LCOL: // Laser color
    e->l_cmd.c = cmd[1];
    bRetFlag = false;
    break;

  case ECL_LTYPE: // Laser type
    e->l_cmd.type = cmd[1];
    bRetFlag = false;
    break;

  case ECL_LWA: // Absolute laser thickness
    e->l_cmd.w = I32LEAt(&cmd[1]);
    bRetFlag = false;
    break;

  case ECL_LXY: // Laser fire position set
    e->l_cmd.x = PixelToWorld(I16LEAt(&cmd[1 + 0]));
    e->l_cmd.y = PixelToWorld(I16LEAt(&cmd[1 + 2]));
    bRetFlag = false;
    break;

  case ECL_DRAW_ON: // Draw
    ECL_DEBUG("ECL_DRAW_ON", 0);
    bRetFlag = false;
    e->flag |= EF_DRAW;
    break;

  case ECL_DRAW_OFF: // Don't draw
    ECL_DEBUG("ECL_DRAW_OFF", 0);
    bRetFlag = false;
    e->flag &= (~EF_DRAW);
    break;

  case ECL_CLIP_ON: // Don't erase off-screen
    ECL_DEBUG("ECL_CLIP_ON", 0);
    bRetFlag = false;
    e->flag |= EF_CLIP;
    break;

  case ECL_CLIP_OFF: // Erase off-screen
    ECL_DEBUG("ECL_CLIP_OFF", 0);
    bRetFlag = false;
    e->flag &= (~EF_CLIP);
    break;

  case ECL_DAMAGE_ON: // Damage enabled
    ECL_DEBUG("ECL_DAMAGE_ON", 0);
    bRetFlag = false;
    e->flag |= EF_DAMAGE;
    break;

  case ECL_DAMAGE_OFF: // Damage disabled
    ECL_DEBUG("ECL_DAMAGE_OFF", 0);
    bRetFlag = false;
    e->flag &= (~EF_DAMAGE);
    break;

  case ECL_HITSB_ON: // Player hit detection enabled
    ECL_DEBUG("ECL_HITSB_ON", 0);
    bRetFlag = false;
    e->flag |= EF_HITSB;
    break;

  case ECL_HITSB_OFF: // Player hit detection disabled
    ECL_DEBUG("ECL_HITSB_OFF", 0);
    bRetFlag = false;
    e->flag &= (~EF_HITSB);
    break;

  case ECL_RLCHG_ON: // Left-right flip enabled (set if on left)
    ECL_DEBUG("ECL_RLCHG_ON", 0);
    bRetFlag = false;
    if (e->x < GX_MID) {
      e->flag |= EF_RLCHG;
    } else {
      e->flag &= (~EF_RLCHG);
    }
    break;

  case ECL_RLCHG_OFF: // Left-right flip disabled
    ECL_DEBUG("ECL_RLCHG_OFF", 0);
    bRetFlag = false;
    e->flag &= (~EF_RLCHG);
    break;

  case ECL_ANM: // Animation set
    // Setting anm_ptnEx is for backward compatibility
    e->anm_ptn = e->anm_ptnEx = cmd[1];
    e->anm_sp = Cast::sign<int8_t>(cmd[2]);
    // if(e->anm_sp==0) e->anm_sp=1;
    e->g_height = (anime[e->anm_ptn].size.h << 5);
    e->g_width = (anime[e->anm_ptn].size.w << 5);
    // if(anime[e->anm_ptn].size.h>32) e->g_height = (e->g_height<<1)/3;
    // if(anime[e->anm_ptn].size.w>32) e->g_width  = (e->g_width <<1)/3;
    e->anm_c = 0;
    bRetFlag = false;
    break;

  case ECL_ANMEX:
    // Do not change any other parameters
    e->anm_ptnEx = cmd[1];
    bRetFlag = false;
    break;

  case ECL_PSE: // Play sound effect
    ECL_DEBUG("ECL_PSE", 0);
    Snd_SEPlay(cmd[1], e->x);
    bRetFlag = false;
    break;

  case ECL_INT: // Boss interrupt trigger
    Bosses.Interrupt(e, cmd[1]);
    bRetFlag = false;
    break; // Do not move cmd

  case ECL_BITATTACK: // (Boss privileged) Set attack pattern to bit
    Bosses.BitAttack(e, U32LEAt(&cmd[1]));
    bRetFlag = false;
    break;

  case ECL_BITLASER: // (Boss privileged) Set laser command to bit
    Bosses.BitLaser(e, cmd[1]);
    bRetFlag = false;
    break;

  case ECL_BITCMD:
    Bosses.BitCommand(e, cmd[1], I32LEAt(&cmd[2]));
    bRetFlag = false;
    break;

  case ECL_EXDEGD: // Special angle increment change
    enemy_exdeg_d = cmd[1];
    bRetFlag = false;
    break;

  case ECL_ENEMYSET:  // Spawn enemy as small fry
  case ECL_ENEMYSETD: // + angle set (register)
  {
    short x = 0;
    short y = 0;

    bRetFlag = false;

    if (count + 1 >= ENEMY_MAX) {
      break;
    }
    auto *new_enemy = &entities[indices[count++]];

    x = ((e->x >> 6) + I16LEAt(&cmd[1])); // PixelToWorld(I16LEAt(&p[0]));
    y = ((e->y >> 6) + I16LEAt(&cmd[3])); // PixelToWorld(I16LEAt(&p[2]));

    // Beware of bugs!!
    if (cmd[0] == ECL_ENEMYSETD) {
      const uint32_t n = (4 + (cmd[6] << 2));
      InitDataSTD(new_enemy, x, y, n);
      new_enemy->d = ID2Value(e, cmd[5]);
    } else {
      const uint32_t n = (4 + (cmd[5] << 2));
      InitDataSTD(new_enemy, x, y, n);
    }
  } break;

  case ECL_BOSSSET: // Spawn boss
    Bosses.SetEx((e->x) >> 6, (e->y) >> 6, cmd[1]);
    bRetFlag = false;
    break;

  case ECL_MOVR: { // Register to struct variable assignment
    const auto dwTemp = ID2Value(e, cmd[2]);
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]] = dwTemp;
      break;

    case ECLCST_LCMD_D:
      e->l_cmd.d = dwTemp;
      break; // Laser command (angle)
    case ECLCST_LCMD_DW:
      e->l_cmd.dw = dwTemp;
      break; // Laser command (angle difference)
    case ECLCST_LCMD_N:
      e->l_cmd.n = dwTemp;
      break; // Laser command (count)
    case ECLCST_LCMD_C:
      e->l_cmd.c = dwTemp;
      break; // Laser command (color)
    case ECLCST_LCMD_L:
      e->l_cmd.l = dwTemp;
      break; // Laser command (length)
    case ECLCST_LCMD_V:
      e->l_cmd.v = dwTemp;
      break; // Laser command (speed)

    case ECLCST_TCMD_D:
      e->t_cmd.d = dwTemp;
      break; // Bullet command (angle)
    case ECLCST_TCMD_DW:
      e->t_cmd.dw = dwTemp;
      break; // Bullet command (angle difference)
    case ECLCST_TCMD_N:
      e->t_cmd.n = dwTemp;
      break; // Bullet command (count)
    case ECLCST_TCMD_NS:
      e->t_cmd.ns = dwTemp;
      break; // Bullet command (rapid count)
    case ECLCST_TCMD_V:
      e->t_cmd.v = dwTemp;
      break; // Bullet command (speed)
    case ECLCST_TCMD_C:
      e->t_cmd.c = dwTemp;
      break; // Bullet command (color)
    case ECLCST_TCMD_A:
      e->t_cmd.a = dwTemp;
      break; // Bullet command (acceleration)

    case ECLCST_TCMD_REP:
      // char buf[100];
      // sprintf(buf,"REP=%d [REG:%d]",dwTemp,cmd[2]);
      //  DebugOut(buf);
      e->t_cmd.rep = dwTemp;
      break; // Bullet command (repeat)

    case ECLCST_TCMD_VD:
      e->t_cmd.vd = dwTemp;
      break; // Bullet command (angular velocity)

    case ECLCST_ENEMY_X:
      e->x = dwTemp;
      break; // Enemy X coordinate
    case ECLCST_ENEMY_Y:
      e->y = dwTemp;
      break; // Enemy X coordinate
    case ECLCST_ENEMY_D:
      e->d = dwTemp;
      break; // Enemy angle

    default:
      DebugOut("Unknown register specified++");
      break;
    }
    bRetFlag = false;
  } break;

  case ECL_MOVC: // Register <- constant (immediate) assignment
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]] = U32LEAt(&cmd[2]);
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_INC: // Register +1
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]]++;
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_DEC: // Register -1
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]]--;
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_ADD: // Add instruction (second arg need not be a register)
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]] += ID2Value(e, cmd[2]);
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_SUB: // Subtract instruction (second arg need not be a register)
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]] -= ID2Value(e, cmd[2]);
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_SINL: // sinl(Gr0,Gr1)
    if (cmd[1] < ECLREG_MAX && cmd[2] < ECLREG_MAX) {
      e->GR[cmd[1]] = sinl(Cast::down<uint8_t>(e->GR[cmd[2]]), e->GR[cmd[1]]);
    } // else {
      // Really should use switch() to distinguish...
    // }
    bRetFlag = false;
    break;

  case ECL_COSL: // cosl(Gr0,Gr1)
    if (cmd[1] < ECLREG_MAX && cmd[2] < ECLREG_MAX) {
      e->GR[cmd[1]] = cosl(Cast::down<uint8_t>(e->GR[cmd[2]]), e->GR[cmd[1]]);
    } // else {
      // Really should use switch() to distinguish...
    // }
    bRetFlag = false;
    break;

  case ECL_MOD: // Gr0 = Gr0 % Const
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      if (U32LEAt(&cmd[2]) != 0) {
        e->GR[cmd[1]] %= U32LEAt(&cmd[2]);
      }
      // else
      //  Zero division error
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_RND: // Gr0 = rnd()
    switch (cmd[1]) {
    case ECLCST_GR0:
    case ECLCST_GR1:
    case ECLCST_GR2:
    case ECLCST_GR3:
    case ECLCST_GR4:
    case ECLCST_GR5:
    case ECLCST_GR6:
    case ECLCST_GR7:
      e->GR[cmd[1]] = (Cast::up<uint32_t>(rnd()) * rnd());
      break;

    default: // Invalid register specified
      DebugOut("Unknown register specified");
      break;
    }
    bRetFlag = false;
    break;

  case ECL_CMPR: // Register to register comparison (Reg0,Reg1)
    if (cmd[1] >= ECLREG_MAX || cmd[2] >= ECLREG_MAX) {
      return; // Error
    }
    RegCmp = (ID2Value(e, cmd[1]) - ID2Value(e, cmd[2]));
    bRetFlag = false;
    break;

  case ECL_CMPC: // Register to constant comparison (Reg,Const)
    if (cmd[1] >= ECLREG_MAX) {
      return; // Error
    }
    RegCmp = (ID2Value(e, cmd[1]) - I32LEAt(&cmd[1 + 1]));
    bRetFlag = false;
    break;

  case ECL_JL: // Jump if comparison result > 0
    if (RegCmp > 0) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    } else {
      {
        bRetFlag = false;
      }
    }
    break;

  case ECL_JS: // Jump if comparison result < 0
    if (RegCmp < 0) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    } else {
      {
        bRetFlag = false;
      }
    }
    break;

  case ECL_JEQ: // Jump if comparison result == 0
    if (RegCmp == 0) {
      e->cmd = U32LEAt(&cmd[1]);
      goto ECL_HEAD;
    } else {
      {
        bRetFlag = false;
      }
    }
    break;

  default: // Undefined instruction!!
    ECL_DEBUG("Unrecognizable Operation Code %02x", cmd[0]);
    return;
  }

  // Prepare for next instruction (return inside switch above if not executing)
  e->cmd += ECL_CmdLen[*cmd];

  if (bRetFlag) {
    return;
  }
  goto ECL_HEAD;
}

// Check interrupt jumps
void EnemyManager::CheckInterrupts(EnemyData *e) {
  int i = 0;

  for (i = 0; i < ECLVECT_MAX; i++) {
    if (e->Vect[i].vect == 0) {
      continue; // No interrupt active
    }
    switch (i) {
    case ECLVECT_BITLEFT: // Bit remaining interrupt
      if (Bosses.GetBitLeft() <= e->Vect[ECLVECT_BITLEFT].value) {
        e->cmd = e->Vect[ECLVECT_BITLEFT].vect;
        e->cmd_c = 0; // Command repeat counter
        e->rep_c = 0; // LOOP (formerly REP) instruction counter
        e->t_rep = 0; // Auto bullet fire timing (0: no auto-fire)
        return;
      }
      break;

    case ECLVECT_BOSSLEFT: // Boss remaining interrupt
      if (std::cmp_less_equal(Bosses.count, e->Vect[ECLVECT_BOSSLEFT].value)) {
        e->cmd = e->Vect[ECLVECT_BOSSLEFT].vect;
        e->cmd_c = 0; // Command repeat counter
        e->rep_c = 0; // LOOP (formerly REP) instruction counter
        e->t_rep = 0; // Auto bullet fire timing (0: no auto-fire)
        return;
      }
      break;

    case ECLVECT_HP: // HPL interrupt
      // char buf[100];
      // sprintf(buf,"hp = %d",e->hp);
      //  DebugOut(buf);
      if (std::cmp_less_equal(e->hp, e->Vect[ECLVECT_HP].value)) {
        e->cmd = e->Vect[ECLVECT_HP].vect;
        e->cmd_c = 0; // Command repeat counter
        e->rep_c = 0; // LOOP (formerly REP) instruction counter
        e->t_rep = 0; // Auto bullet fire timing (0: no auto-fire)
        // DebugOut("Accepted");
        return;
      }
      break;

    case ECLVECT_TIMER: // Timer interrupt
      if (std::cmp_greater(e->IntTimer, e->Vect[ECLVECT_TIMER].value)) {
        e->cmd = e->Vect[ECLVECT_TIMER].vect;
        e->cmd_c = 0;    // Command repeat counter
        e->rep_c = 0;    // LOOP (formerly REP) instruction counter
        e->t_rep = 0;    // Auto bullet fire timing (0: no auto-fire)
        e->IntTimer = 0; // Initialization specific to this interrupt
        return;
      } else {
        e->IntTimer++;
      }
      break;

    default:
      break;
    }
  }
}

// Initialize interrupt vectors
void EnemyManager::InitInterrupts(EnemyData *e) {
  for (auto &it : e->Vect) {
    it.vect = 0;
  }
}

// Ensure that everything we return fits losslessly into the return type. If
// this ever fails due to new script commands that return larger values,
// ID2Value() needs a larger return type.
#pragma warning(error : 4244)

// Convert from ECLCST_?? to its value
static uint32_t ID2Value(const EnemyData *e, uint8_t id) {
  switch (id) {
  // Register specified case
  case ECLCST_GR0:
  case ECLCST_GR1:
  case ECLCST_GR2:
  case ECLCST_GR3:
  case ECLCST_GR4:
  case ECLCST_GR5:
  case ECLCST_GR6:
  case ECLCST_GR7:
    return e->GR[id];

  case ECLCST_LCMD_D:
    return e->l_cmd.d; // Laser command (angle)
  case ECLCST_LCMD_DW:
    return e->l_cmd.dw; // Laser command (angle difference)
  case ECLCST_LCMD_N:
    return e->l_cmd.n; // Laser command (count)
  case ECLCST_LCMD_C:
    return e->l_cmd.c; // Laser command (color)
  case ECLCST_LCMD_L:
    return e->l_cmd.l; // Laser command (length)
  case ECLCST_LCMD_V:
    return e->l_cmd.v; // Laser command (speed)

  case ECLCST_TCMD_D:
    return e->t_cmd.d; // Bullet command (angle)
  case ECLCST_TCMD_DW:
    return e->t_cmd.dw; // Bullet command (angle difference)
  case ECLCST_TCMD_N:
    return e->t_cmd.n; // Bullet command (count)
  case ECLCST_TCMD_NS:
    return e->t_cmd.ns; // Bullet command (rapid count)
  case ECLCST_TCMD_V:
    return e->t_cmd.v; // Bullet command (speed)
  case ECLCST_TCMD_C:
    return e->t_cmd.c; // Bullet command (color)
  case ECLCST_TCMD_A:
    return e->t_cmd.a; // Bullet command (acceleration)
  case ECLCST_TCMD_REP:
    return e->t_cmd.rep; // Bullet command (repeat)
  case ECLCST_TCMD_VD:
    return e->t_cmd.vd; // Bullet command (angular velocity)

  case ECLCST_ENEMY_X:
    return e->x; // Enemy X coordinate
  case ECLCST_ENEMY_Y:
    return e->y; // Enemy Y coordinate
  case ECLCST_ENEMY_D:
    return e->d; // Enemy angle

  default:
    return 0;
  }
}
