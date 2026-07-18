///
/// Laser - Laser processing (infinite distance, reflection, short)
///

#include <utility>

#include "laser.h"
#include "laser_manager.h"
#include "long_laser.h"

#include "core/entity.h"
#include "core/gian.h"
#include "core/level.h"
#include "gameflow/play_rank.h"
#include "player/player.h"
#include "gameflow/rank_manager.h"
#include "gfx/graphics_backend.h"
#include "gfx/geometry.h"
#include "util/ut_math.h"

// Laser coordinate management uses the following structure:
//
//    3-----------------Length---> >----------2
//   Width                      < <             |
//    +(x,y)                     > >            +
//   Width                      < <             |
//    0-----------------Length---> >----------1
//
//
// 0-3 counterclockwise from the start point
//
// Optimized distance calculation using trigonometric functions
//
// Let lasers(sx,sy), Saboten(x,y), laser angle in degrees
// Length and width are calculated as:
// (tx = sx-x; ty = sy-y;)
// length = -(cosm(degree)*tx/256+sinm(degree)*ty/256);
// width(1) = (length*cosm(degree)+tx*256)/(-sinm(degree));
// width(2) = (length*sinm(degree)+ty*256)/( cosm(degree));

// Laser constants
static constexpr auto RT_MAX = 10;      // Maximum number of reflectors
static constexpr auto LS_ZSET = 0x08;   // Laser cactus (player) set attribute
static constexpr auto SLASER_EVADE = 3; // Short laser graze points
static constexpr auto LASER_EVADE_WIDTH = (12 * 64); // Laser graze tolerance

// Laser type constants
static constexpr auto LS_SHORT = 0x00; // Short laser
static constexpr auto LS_REF = 0x01;   // Reflection laser
static constexpr auto LS_LONG = 0x02;  // Infinite distance laser
static constexpr auto LS_LONGY =
    0x03; // Vertical infinite distance laser (angle ignored, fixed at 64)

// Laser command constants
#undef LC_ALL // There's a locale macro with the same name...
static constexpr auto LC_WAY = 0x00;  // Fan-shaped fire
static constexpr auto LC_ALL = 0x01;  // All-direction fire
static constexpr auto LC_RND = 0x02;  // Random with base angle set
static constexpr auto LC_WAYZ = 0x08; // Fan-shaped fire & cactus set
static constexpr auto LC_ALLZ = 0x09; // All-direction fire & cactus set
static constexpr auto LC_RNDZ = 0x0a; // Random with base angle cactus set

// Laser flag constants (depends on some laser types)
static constexpr auto LF_NONE = 0x00;  // No flag state
static constexpr auto LF_CLEAR = 0x01; // Laser is disappearing

static constexpr auto LF_SHOT = 0x02; // Laser firing
static constexpr auto LF_HIT = 0x04;  // Laser hitting (against REF_OBJECT)
static constexpr auto LF_NMOVE =
    0x06; // Laser length unchanged (LF_SHOT|LF_HIT)

// LASER_DATA, LF_DELETE moved to laser.h

// Global variables
// LaserCmd, count, lasers[], laser_indices[] moved to LaserManager in
// laser_manager.cpp REFLECTOR		Reflector[RT_MAX]; // Reflector
// structure
//  uint16_t	ReflectorNow;		// Number of reflectors

// private methods declared in laser_manager.h
// private methods declared in laser_manager.h
// private methods declared in laser_manager.h

// private methods declared in laser_manager.h
// private methods declared in laser_manager.h
// private methods declared in laser_manager.h

// private methods declared in laser_manager.h
// private methods declared in laser_manager.h

// private methods declared in laser_manager.h
// private methods declared in laser_manager.h

void LaserManager::Spawn() {
  switch (Ranking.state.level) {
  case GameLevel::EASY:
    SetEasy();
    break;

  case GameLevel::NORMAL:
    break;

  case GameLevel::HARD:
  case GameLevel::EXTRA:
    SetHard();
    break;

  case GameLevel::LUNATIC:
    SetLunatic();
    break;
  }

  cmd.v = (((cmd.v >> 1) * (Ranking.state.Rank)) >> (5 + 8)) + (cmd.v >> 1);

  SpawnEX();
}

void LaserManager::SpawnEX() {
  for (decltype(cmd.n) i = 0; i < cmd.n; i++) {
    if (count + 1 == LASER_MAX) {
      return; // Exceeded maximum count
    }

    auto *lp = &lasers[laser_indices[count++]];

    lp->v = cmd.v;      // Laser speed
    lp->a = cmd.a;      // Laser acceleration
    lp->d = CalcDir(i); // Laser angle

    if (cmd.l2 != 0) {
      lp->x = cmd.x + cosl(lp->d, cmd.l2);
      lp->y = cmd.y + sinl(lp->d, cmd.l2);
    } else {
      lp->x = cmd.x; // Start point x coordinate
      lp->y = cmd.y; // Start point y coordinate
    }

    lp->vx = cosl(lp->d, lp->v); // vx from d and v
    lp->vy = sinl(lp->d, lp->v); // vy from d and v

    lp->w = cmd.w;    // Laser thickness
    lp->lmax = cmd.l; // Laser max length

    lp->lx = lp->ly = 0;
    lp->wx = -sinl(lp->d, lp->w >> 6); // cosl(d+64,w)
    lp->wy = cosl(lp->d, lp->w >> 6);  // sinl(d+64,w)

    lp->l = lp->count = 0; // Zero-initialize length and counter

    lp->c = cmd.c;       // Set laser color
    lp->type = cmd.type; // Set laser type

    if (lp->type == LS_REF) {
      lp->flag = LF_SHOT;
    } else {
      lp->flag = LF_NONE; // Initialize flag
    }

    lp->evade = 0; // Graze flag

    lp->notr = cmd.notr; // Reflector to not reflect against

    SetupShort(lp); // Set 4-point coordinates (for short)
  }
}

void LaserManager::Move() {
  // [count] will get mutated for reflecting lasers!
  for (uint16_t i = 0; i < count; i++) {
    auto *lp = &lasers[laser_indices[i]];
    MoveLaser(lp);
    lp->count++;
    if ((lp->x) < GX_MIN || (lp->x) > GX_MAX || (lp->y) < GY_MIN ||
        (lp->y) > GY_MAX) {
      lp->flag = LF_DELETE;
    }

    if (Players.IsInvincible() == 0 &&
        ((lp->flag & (LF_CLEAR | LF_DELETE)) == 0)) {
      HitCheck(lp);
    }
  }
  Indsort(laser_indices, count, lasers,
          [](const LASER_DATA &l) { return (l.flag & LF_DELETE); });
}

void LaserManager::Draw() {
  int i = 0;

  GrpGeom->Lock();

  for (i = 0; std::cmp_less(i, count); i++) {
    auto *lp = &lasers[laser_indices[i]];
    switch (lp->type) {
    // Normal short laser & reflection laser
    case LS_SHORT:
    case LS_REF:
      DrawShort(lp);
      break;

      // Infinite distance laser & Y_positive_infinite laser
      // case(LS_LONG):case(LS_LONGY):
      //        Ldraw(lp);
      // break;
    }
  }

  // for(i=0;i<count;i++){
  //         auto* lp = &lasers[laser_indices[i]];
  //         if(lp->type==LS_REF || lp->type==LS_SHORT)
  //                 DrawShort(lp);
  // }

  GrpGeom->Unlock();
}

void LaserManager::Clear() {
  for (uint16_t i = 0; i < count; i++) {
    auto &l = lasers[laser_indices[i]];
    if (l.flag != LF_CLEAR) {
      l.flag = LF_CLEAR;
      l.count = 0;
    }
  }
}

void LaserManager::SetIndices() {
  for (auto i = 0; i < LASER_MAX; i++) {
    laser_indices[i] = i;
    // memset(lasers+i,0,sizeof(LASER_DATA));
  }

  count = 0;
}

void LaserManager::SetEasy() {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    if (cmd.n >= 3) {
      cmd.n -= 2; // Do not change odd/even
    }
    cmd.dw += (cmd.dw >> 2); // Widen
    break;

  case LC_ALL:
  case LC_RND:
    cmd.n >>= 1; // Half the count
    break;
  }

  cmd.l -= (cmd.l >> 2);
}

void LaserManager::SetHard() {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    cmd.n += 2;              // Do not change odd/even
    cmd.dw -= (cmd.dw >> 3); // Narrow
    break;

  case LC_ALL:
    cmd.n += (((cmd.n >> 2) > 6) ? 6 : (cmd.n >> 2));
    break;

  case LC_RND:
    cmd.n += (cmd.n >> 1); // 50% increase
    break;
  }

  cmd.l += (cmd.l >> 2);
}

void LaserManager::SetLunatic() {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    cmd.n += 4;             // Do not change odd/even
    cmd.dw -= (cmd.dw / 3); // Narrow
    break;

  case LC_ALL:
    cmd.n += (((cmd.n / 3) > 12) ? 12 : (cmd.n / 3));
    break;

  case LC_RND:
    cmd.n <<= 1; // Double the count
    break;
  }

  cmd.l += (cmd.l >> 1);
}

uint8_t LaserManager::CalcDir(uint16_t i) const {
  uint8_t deg = 0;

  if ((cmd.cmd & LS_ZSET) != 0) {
    deg = atan8(Players.X() - cmd.x, Players.Y() - cmd.y);
  }

  deg += cmd.d; // Base angle set

  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    i++;
    if ((cmd.n & 1) != 0) {
      return deg + ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));
    }
    return deg - (cmd.dw >> 1) + ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));

  case LC_ALL:
    return deg + ((i << 8) / cmd.n);

  case LC_RND:
    return deg + (rnd() % cmd.dw) - (cmd.dw >> 1);

  default: // Should never reach here...
    return 0;
  }
}

void LaserManager::SetupShort(LASER_DATA *lp) {
  lp->p[1].x = lp->p[0].x = (lp->x >> 6) + lp->wx;
  lp->p[1].y = lp->p[0].y = (lp->y >> 6) + lp->wy;

  lp->p[2].x = lp->p[3].x = (lp->x >> 6) - lp->wx;
  lp->p[2].y = lp->p[3].y = (lp->y >> 6) - lp->wy;

  lp->p[1].x += lp->lx;
  lp->p[1].y += lp->ly;
  lp->p[2].x += lp->lx;
  lp->p[2].y += lp->ly;
}

void LaserManager::DrawShort(const LASER_DATA *lp) {
  constexpr RGB216 col = {1, 0, 5};
  int x = 0;
  int y = 0;

  int tx = 0;
  int ty = 0;

  if (lp->flag == LF_CLEAR) {
    // GrpGeom->SetColor(laser_color[lp->c]);
    GrpGeom->SetColor(col);
    GrpGeom->DrawLine(lp->p[0].x, lp->p[0].y, lp->p[1].x, lp->p[1].y);
    GrpGeom->DrawLine(lp->p[3].x, lp->p[3].y, lp->p[2].x, lp->p[2].y);
    return;
  }

  x = lp->x >> 6;
  tx = 0; //(lp->p[0].x - x)>>2;
  y = lp->y >> 6;
  ty = 0; //(lp->p[0].y - y)>>2;

  // Reference
  // From the rendering state, it seems some drivers enable alpha
  // as soon as the alpha setting is changed. So GrpSetAlpha is disabled here.

  // GrpGeom->SetAlpha(0, GRAPHICS_ALPHA::ONE);
  // GrpGeom->SetColor({ 4, 1, 0 });
  // GrpGeom->SetAlpha(128, GRAPHICS_ALPHA::ONE);	// Removed on 2000/09/06

  if (auto *gp = GrpGeom_Poly()) {
    GeomGrdRect(*gp, lp->p, col.ToRGB());
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({1, 0, 5});
    gf->DrawTriangleFan(lp->p);

    gf->SetColor({5, 5, 5});

    VERTEX_XY p[4];
    p[0].x = p[1].x = lp->p[0].x - (lp->wx * 3 / 4); //- wx*4/len;
    p[0].y = p[1].y = lp->p[0].y - (lp->wy * 3 / 4); //- wy*4/len;
    p[3].x = p[2].x = lp->p[3].x + (lp->wx * 3 / 4); //+ wx*4/len;
    p[3].y = p[2].y = lp->p[3].y + (lp->wy * 3 / 4); //+ wy*4/len;
    p[1].x += lp->lx;
    p[1].y += lp->ly;
    p[2].x += lp->lx;
    p[2].y += lp->ly;
    gf->DrawTriangleFan(p);
    // gf->DrawLine(lp->p[0].x, lp->p[0].y, lp->p[1].x, lp->p[1].y);
    // gf->DrawLine(lp->p[2].x, lp->p[2].y, lp->p[3].x, lp->p[3].y);
    // gf->DrawTriangleFan(lp->p, 4);
    // gf->SetColor({ 5, 5, 5 });
    // gf->DrawLine(x, y, (x + lp->lx), (y + lp->ly));
  }
  // Grp_Polygon(temp,4,RGB256(5,5,5));
}

void LaserManager::MoveLaser(LASER_DATA *lp) {
  // int i;
  // int ltempx,ltempy;

  if (lp->flag == LF_CLEAR) {
    if (lp->l < lp->lmax) {
      lp->l += lp->v;
      lp->w += 16;
      lp->lx = cosl(lp->d, lp->l >> 6);
      lp->ly = sinl(lp->d, lp->l >> 6);
      lp->p[1].x = lp->p[0].x + lp->lx;
      lp->p[1].y = lp->p[0].y + lp->ly;
      lp->p[2].x = lp->p[3].x + lp->lx;
      lp->p[2].y = lp->p[3].y + lp->ly;
    } else {
      {
        lp->w += 64;
      }
    }

    lp->wx = -sinl(lp->d, lp->w >> 6);
    lp->wy = cosl(lp->d, lp->w >> 6);
    SetupShort(lp);

    if (lp->count > 30) {
      lp->flag = LF_DELETE;
    }
    return;
  }

  switch (lp->type) {
  case LS_SHORT: // Short laser
    if ((lp->l) < (lp->lmax)) {
      lp->l += lp->v; // Extend
      lp->lx = cosl(lp->d, lp->l >> 6);
      lp->ly = sinl(lp->d, lp->l >> 6);
      lp->p[1].x = lp->p[0].x + lp->lx;
      lp->p[1].y = lp->p[0].y + lp->ly;
      lp->p[2].x = lp->p[3].x + lp->lx;
      lp->p[2].y = lp->p[3].y + lp->ly;
    } else {
      lp->x += lp->vx;
      lp->y += lp->vy;
      SetupShort(lp);
    }
    return;

  case LS_REF:       // Reflection laser
    MoveReflect(lp); // Too long, extract to function!
    return;

    // case(LS_LONG):	// Infinite distance laser
    // case(LS_LONGY):	// Y positive direction infinite laser
    //         LONGL_move(lp);
    // return;
  }
}

void LaserManager::HitCheck(LASER_DATA *lp) {
  long tx = 0;
  long ty = 0;
  long w1 = 0;
  long length = 0;

  switch (lp->type) {
  case LS_SHORT:
  case LS_REF:
    // Calculation note: uses x64 for coordinate calculations
    // /256 correction needed because sinm(),cosm() are used

    tx = Players.X() - lp->x;
    ty = Players.Y() - lp->y;
    length = cosl(lp->d, tx) + sinl(lp->d, ty);
    w1 = abs(-sinl(lp->d, tx) + cosl(lp->d, ty));
    // tx = ((lp->x)-(Players.X()));	ty =
    //((lp->y)-(Players.Y())); length =
    //-((cosm(lp->d)*tx+sinm(lp->d)*ty)>>8); tx <<= 8;	ty <<= 8;
    //
    //                     if(cosm(lp->d)==0)
    //                             w1 =
    // abs((length*cosm(lp->d)+tx)/(-sinm(lp->d))); else if(sinm(lp->d)==0) w1 =
    // abs((length*sinm(lp->d)+ty)/( cosm(lp->d))); else{ w2 =
    // abs((length*cosm(lp->d)+tx)/(-sinm(lp->d))); w1 =
    // abs((length*sinm(lp->d)+ty)/( cosm(lp->d))); w1 = (w1+w2)/2;	//
    //  Improved precision
    //                     }
    if (length > 0 && length <= (lp->l) && w1 <= (lp->w)) {
      Players.OnHit();
    } else if (length > 0 && length <= (lp->l) &&
               w1 <= (lp->w + LASER_EVADE_WIDTH)) {
      if (lp->evade != 0U) {
        {
          Players.AddEvade(0);
        }
      } else {
        lp->evade = 0xff;
        Players.AddEvade(SLASER_EVADE);
      }
    }
    break;

    // case(LS_LONG):
    // break;

    // case(LS_LONGY):
    // break;
  }
}

void LaserManager::MoveReflect(LASER_DATA *lp) {
  switch (lp->flag) {
  case LF_NONE: // E --> R  Normal movement
    lp->x += lp->vx;
    lp->y += lp->vy;
    SetupShort(lp);

    // Hit check here!
    if (HitReflect(lp) != 0) {
      lp->flag = LF_HIT;
    }
    return;

  case LF_SHOT: // E --> R  Firing
    lp->l += lp->v;
    lp->lx = cosl(lp->d, lp->l >> 6);
    lp->ly = sinl(lp->d, lp->l >> 6);
    lp->p[1].x = lp->p[0].x + lp->lx;
    lp->p[1].y = lp->p[0].y + lp->ly;
    lp->p[2].x = lp->p[3].x + lp->lx;
    lp->p[2].y = lp->p[3].y + lp->ly;
    if ((lp->l) >= (lp->lmax)) {
      lp->flag = LF_NONE;
    }

    // Hit check needed here too!
    // Note: must execute ltemp:=l
    if (HitReflect(lp) != 0) {
      lp->ltemp = lp->l;
      lp->flag |= LF_HIT; // LF_HIT or LF_NMOVE
    }
    return;

  case LF_HIT: // E --> R  Hitting
    if ((lp->l) <= (lp->v)) {
      lp->flag = LF_DELETE;
    } else {
      lp->l -= lp->v;
    }

    lp->x += lp->vx;
    lp->y += lp->vy;
    lp->lx = cosl(lp->d, lp->l >> 6);
    lp->ly = sinl(lp->d, lp->l >> 6);
    lp->p[0].x = lp->p[1].x - lp->lx;
    lp->p[0].y = lp->p[1].y - lp->ly;
    lp->p[3].x = lp->p[2].x - lp->lx;
    lp->p[3].y = lp->p[2].y - lp->ly;
    return;

  case LF_NMOVE: // E --> R  Firing & hitting
    lp->ltemp += lp->v;
    if ((lp->ltemp) >= (lp->lmax)) {
      lp->flag = LF_HIT;
    }
    return;
  }
}

int LaserManager::HitReflect(const LASER_DATA *lp) {
  // REFLECTOR *rp;
  LongLaserData *ll = nullptr;
  int i = 0;

  // Tip relative to laser direction?
  const long lx = (lp->x + cosl(lp->d, lp->l)); // Laser check point X
  const long ly = (lp->y + sinl(lp->d, lp->l)); // Laser check point Y

  long tx = 0;
  long ty = 0; // Temporary coordinates
  long length = 0;
  long width = 0; // For hit check

  // Reflector collision check (hit -> non-zero)
  // d' = - d + Rd*2    : d:Laser_Degree  Rd:Reflector_Degree

  // Reflect against thick lasers
  for (i = 0; i < LLASER_MAX; i++) {
    if (std::cmp_equal(i, lp->notr)) {
      continue; // Skip if reflected last time
    }
    ll = &long_lasers[i];
    if (ll->flag != LLF_NORM) {
      continue; // Skip if not fully open
    }

    tx = lx - ll->x;
    ty = ly - ll->y;
    length = cosl(ll->d, tx) + sinl(ll->d, ty);
    width = abs(-sinl(ll->d, tx) + cosl(ll->d, ty));

    if (length > 0 && width <= ll->w) {
      cmd.x = lx;
      cmd.y = ly;
      cmd.v = lp->v;
      cmd.d = -(lp->d) + ((ll->d) << 1);
      cmd.w = lp->w; //
      cmd.l =
          lp->lmax;  // Pretty important (to make it look like the same laser)
      cmd.l2 = 0;    // lp->v;
      cmd.n = 1;     // Splitting would be scary...
      cmd.c = lp->c; // Same color
      cmd.cmd = LC_WAY;  //
      cmd.type = LS_REF; // Allow two or more reflections
      cmd.notr = i;      // Do not reflect against self
      SpawnEX();         // No difficulty change
      return 1;          // Hit
    }
  }

  return 0; // Miss
}
