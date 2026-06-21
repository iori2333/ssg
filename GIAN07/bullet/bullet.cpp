///
/// Bullet - Bullet firing and management
///

#include <utility>

#include "bullet.h"
#include "bullet_manager.h"

#include "audio/snd.h"
#include "core/config.h"
#include "core/gian.h"
#include "core/level.h"
#include "gfx/graphics_backend.h"
#include "util/cast.h"
#include "util/ut_math.h"

//// Global variables → moved to BulletManager in bullet_manager.cpp
// command, bullets, count_small, count_large → referenced from
// bullet_manager.cpp (cross-module) indices_small, indices_large, max_small,
// max_large, speed → accessed directly via bullet_manager.h

//// Local functions ////
// Private methods are declared in bullet_manager.h
void TamaEffectDraw(const Bullet *t); // Draw bullet as effect?

int GetBulletHitRadius(uint8_t c) {
  switch (c & 0xF0) {
  case TAMA_SMALL:
    return TAMA_HIT_S;
  case TAMA_LARGE:
  case TAMA_EXTRA2:
    return TAMA_HIT_M;
  case TAMA_ANGLE:
    return (c == 0x25) ? TAMA_HIT_M : TAMA_HIT_S;
  case TAMA_EXTRA: {
    constexpr int radii[4] = {TAMA_HIT_XL, TAMA_HIT_L, TAMA_HIT_M, TAMA_HIT_S};
    return radii[c & 3];
  }
  default:
    return TAMA_HIT_M;
  }
}

int GetBulletEvadeRadius(uint8_t c) {
  return ((c & 0xF0) == TAMA_SMALL) ? TAMA_EVADE_RADIUS_SMALL
                                    : TAMA_EVADE_RADIUS_LARGE;
}

void TamaEvadeAdd(Bullet *t) {
  if (t->flag & TF_EVADE)
    Players.AddEvadeEx(t->x, t->y, 0);
  else {
    t->flag |= TF_EVADE;
    Players.AddEvadeEx(t->x, t->y, TAMA_EVADE);
  }
}

void BulletManager::Spawn() {
  int v = 0;

  // Do not change for NORMAL (in-game difficulty adjustment was planned but not
  // implemented) // Probably should be written inside the switch... //
  switch (Ranking.state.GameLevel) {
  case GameLevel::EASY:
    SetEasy();
    break;

  case GameLevel::HARD:
    SetHard();
    break;

  case GameLevel::LUNATIC:
    SetLunatic();
    break;
  }

  // Value is simply (speed/2)*rank/32 + speed/2
  v = SPEEDM(command.v); // Set base speed value (GIAN.H)
  if ((command.type & 0x0f) == T_NORM) {
    speed = (((v >> 1) * (Ranking.state.Rank)) >> (5 + 8)) + (v >> 1);
  } else {
    speed = v;
  }

  TamaSetMain();
}

void BulletManager::SpawnEX() {
  speed = SPEEDM(command.v);

  TamaSetMain();
}

// Set bullet (line formation fire)
void BulletManager::SpawnLine() {
  // uint32_t temp;
  uint16_t *indnow = nullptr;
  uint16_t *indmax = nullptr;
  uint16_t *indp = nullptr; // Same as above

  speed = SPEEDM(command.v);

  // Set "access region" (small bullet or special bullet)        //
  if ((command.c & 0xf0) == TAMA_SMALL) {
    indnow = &count_small, indmax = &max_small,
    indp = &indices_small[count_small];
  } else {
    indnow = &count_large, indmax = &max_large,
    indp = &indices_large[count_large];
  }

  // Number of bullets to set (accounting for rapid fire)
  const uint16_t setmax =
      (command.n * (((command.cmd & TAMA_REN) != 0) ? command.ns : 1));

  // Set other parameters //
  command.cmd = (command.cmd & 0xf0) | TC_WAY;

  for (const auto i : std::views::iota(0U, setmax)) {
    if ((*indnow) + 1 >= (*indmax)) {
      return; // Cannot set
    }

    *indnow = *indnow + 1;       // Increment bullet count
    auto *t = &bullets[indp[i]]; // Set bullet pointer

    t->x = t->tx = command.x; // Set X coordinate
    t->y = t->ty = command.y; // Set Y coordinate

    t->a = command.a; // Note: size is char

    // temp = random_ref;
    t->d = Dir(i); // Bullet firing angle
    // Debug(temp,31);

    t->d16 = (t->d << 8); // Used for angular velocity movement

    // temp = random_ref;
    t->v = t->v0 = LineCmdNewSpeed(i); // Set initial speed
    // Debug(temp,30);

    t->vx = cosl(t->d, t->v); // Set X component of velocity
    t->vy = sinl(t->d, t->v); // Set Y component of velocity

    t->vd = command.vd;             // Angular velocity or homing rate
    t->c = command.c;               // Bullet color & shape
    t->rep = command.rep;           // Repeat count
    t->type = command.type;         // Bullet type
    t->option = command.option;     // Bullet attributes (vibe, reflect, etc.)
    t->effect = command.cmd & 0xf0; // Bullet effect
    t->count = 0;                   // Initialize counter
    t->flag = Flag();               // Initialize flags
  }
}

// Extra boss bullet pattern (wider angle = slower) //
void BulletManager::SpawnExtra01() {
  // uint32_t temp;
  uint16_t *indnow = nullptr;
  uint16_t *indmax = nullptr;
  uint16_t *indp = nullptr; // Same as above

  speed = SPEEDM(command.v);

  // Set "access region" (small bullet or special bullet)        //
  if ((command.c & 0xf0) == TAMA_SMALL) {
    indnow = &count_small, indmax = &max_small,
    indp = &indices_small[count_small];
  } else {
    indnow = &count_large, indmax = &max_large,
    indp = &indices_large[count_large];
  }

  // Number of bullets to set (accounting for rapid fire)
  const uint16_t setmax =
      (command.n * (((command.cmd & TAMA_REN) != 0) ? command.ns : 1));

  // Set other parameters //
  // command.cmd = (command.cmd & 0xf0);

  for (const auto i : std::views::iota(0U, setmax)) {
    if ((*indnow) + 1 >= (*indmax)) {
      return; // Cannot set
    }

    *indnow = *indnow + 1;       // Increment bullet count
    auto *t = &bullets[indp[i]]; // Set bullet pointer

    t->x = t->tx = command.x; // Set X coordinate
    t->y = t->ty = command.y; // Set Y coordinate

    t->a = command.a; // Note: size is char

    t->d = Dir(i);        // Bullet firing angle
    t->d16 = (t->d << 8); // Used for angular velocity movement

    t->v = t->v0 = SpeedEx(t->d); // Set initial speed

    t->vx = cosl(t->d, t->v); // Set X component of velocity
    t->vy = sinl(t->d, t->v); // Set Y component of velocity

    t->vd = command.vd;             // Angular velocity or homing rate
    t->c = command.c;               // Bullet color & shape
    t->rep = command.rep;           // Repeat count
    t->type = command.type;         // Bullet type
    t->option = command.option;     // Bullet attributes (vibe, reflect, etc.)
    t->effect = command.cmd & 0xf0; // Bullet effect
    t->count = 0;                   // Initialize counter
    t->flag = Flag();               // Initialize flags
  }
}

int BulletManager::SpeedEx(uint8_t d) const {
  int temp = 0;
  int delta = 0;

  switch (command.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8; // DebugOut("2");
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16; // DebugOut("3");
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32; // DebugOut("4");
    break;
  }

  // Change speed based on distance between d and command.d //
  delta = command.d - d;
  if (delta > 128) {
    delta -= 256;
  }
  if (delta < -128) {
    delta += 256;
  }

  return speed - ((speed * abs(delta)) / 23) + temp;
}

void BulletManager::TamaSetMain() {
  // uint32_t temp;
  uint16_t *indnow = nullptr;
  uint16_t *indmax = nullptr;
  uint16_t *indp = nullptr; // Same as above

  // Set "access region" (small bullet or special bullet)        //
  if ((command.c & 0xf0) == TAMA_SMALL) {
    indnow = &count_small, indmax = &max_small,
    indp = &indices_small[count_small];
  } else {
    indnow = &count_large, indmax = &max_large,
    indp = &indices_large[count_large];
  }

  // Number of bullets to set (accounting for rapid fire)
  const uint16_t setmax =
      (command.n * (((command.cmd & TAMA_REN) != 0) ? command.ns : 1));

  for (const auto i : std::views::iota(0U, setmax)) {
    if ((*indnow) + 1 >= (*indmax)) {
      return; // Cannot set
    }

    *indnow = *indnow + 1; // Increment bullet count
    auto *t = &bullets[indp[i]];

    t->x = t->tx = command.x; // Set X coordinate
    t->y = t->ty = command.y; // Set Y coordinate

    // temp = random_ref;
    t->v = t->v0 = NewSpeed(i); // Set initial speed
    // Debug(temp,30);

    t->a = command.a; // Note: size is char

    // temp = random_ref;
    t->d = Dir(i); // Bullet firing angle
    // Debug(temp,31);

    t->d16 = (t->d << 8); // Used for angular velocity movement

    t->vx = cosl(t->d, t->v); // Set X component of velocity
    t->vy = sinl(t->d, t->v); // Set Y component of velocity

    t->vd = command.vd;             // Angular velocity or homing rate
    t->c = command.c;               // Bullet color & shape
    t->rep = command.rep;           // Repeat count
    t->type = command.type;         // Bullet type
    t->option = command.option;     // Bullet attributes (vibe, reflect, etc.)
    t->effect = command.cmd & 0xf0; // Bullet effect
    t->count = 0;                   // Initialize counter
    t->flag = Flag();               // Initialize flags
  }
}

void BulletManager::Move() {
  // Hit check after cactus death determination because alive //
  // time is longer than dead time...                         //

  auto process_bullet = [this](Bullet *t) {
    if (t->effect == TE_NONE) {
      MoveByType(t);
      MoveByOption(t);
      if (((t->flag & TF_CLIP) == 0) &&
          ((t->x) < GX_MIN - (4 * 64) || (t->x) > GX_MAX + (4 * 64) ||
           (t->y) < GY_MIN - (4 * 64) || (t->y) > GY_MAX + (4 * 64))) {
        t->flag = TF_DELETE;
      }
      t->count++;
      if (Players.IsInvincible()) {
        return;
      }

      const int ev_r = GetBulletEvadeRadius(t->c);
      const auto evade = Players.HitCheck(t->x, t->y, ev_r);
      if (evade) {
        TamaEvadeAdd(t);
      }

      const int r = GetBulletHitRadius(t->c);
      const auto hit = Players.HitCheck(t->x, t->y, r);
      if (hit) {
        t->flag = TF_DELETE;
        Players.OnHit();
      }
    } else {
      MoveByEffect(t);
      t->count++;
    }
  };

  // Small bullet processing //
  for (const auto i : std::views::iota(0U, count_small)) {
    auto *t = &bullets[indices_small[i]];
    process_bullet(t);
  }
  Indsort(indices_small, count_small, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  // Large bullet & special bullet processing //
  for (const auto i : std::views::iota(0U, count_large)) {
    auto *t = &bullets[indices_large[i]];
    process_bullet(t);
  }
  Indsort(indices_large, count_large, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });
}

void BulletManager::Draw() {
  //	HRESULT		ddrval;
  PIXEL_LTRB src;
  int x = 0;
  int y = 0;
  int dx = 0;
  int dy = 0;

  static const PIXEL_LTRB rcExtraTama[4] = {{128, 384, 128 + 32, 384 + 32},
                                            {128 + 32, 384, 128 + 56, 384 + 24},
                                            {128 + 56, 384, 128 + 72, 384 + 16},
                                            {128 + 72, 384, 128 + 80, 384 + 8}};

  static constexpr uint8_t sizeExtraTama[4] = {16, 12, 8, 4};

  // Draw large bullet & special bullet (16*16) //
  for (const auto i : std::views::iota(0U, count_large)) {
    auto *t = &bullets[indices_large[i]];

    x = (t->x >> 6) - 8; // -8 is for coordinate correction
    y = (t->y >> 6) - 8; // Same as above

    switch (t->effect) {
    case TE_DELETE:
      src = PIXEL_LTWH{(384 + ((t->count / 6) << 4)), 104, 16, 16};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      continue;

    case TE_CIRCLE1:
      TamaEffectDraw(t);
      continue;

      // Ignore others //
    }

    switch (t->c & 0xf0) {
    case TAMA_LARGE: // Large round bullet
      src.top = 8;
      src.left = ((t->c & 0x0f) << 4) + 384;
      src.bottom = 24;
      src.right = src.left + 16;
      break;

    case TAMA_EXTRA: {
      const uint8_t d = (t->c & 3);
      src = rcExtraTama[d];
      x = (t->x >> 6) - sizeExtraTama[d];
      y = (t->y >> 6) - sizeExtraTama[d];
      GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY, src);
    }
      continue;

    case TAMA_EXTRA2: {
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
      // Convert 256(-1) -> 32(-1)
      const auto d = (Cast::down_sign<uint8_t>(t->d + 4) / 8);

      src.top = 320 + ((t->c & 3) << 4); // (c mod 4) * 16
      src.left = d * 16;
      src.bottom = src.top + 16;
      src.right = src.left + 16;
      // x   = (t->x>>6) - 8;	// Size is fixed at 16
      // y   = (t->y>>6) - 8;	// So this is fine as-is
      GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY, src);
    }
      continue;

    case TAMA_ANGLE:
      // default:		// Angle animation type
      if (t->c != 32 + 5) {
        src.top = 24 + ((t->c & 0x0f) << 4);
        src.left = ((t->d + 8) & 0xf0) + 384;
        src.bottom = src.top + 16;
        src.right = src.left + 16;
      } else {
        // Same as above.
        //
        // Every 8 steps = 32 divisions, so offset is 4
        // d = (Cast::down_sign<uint8_t>(t->d + 8) / 8);
        const auto d = (Cast::down_sign<uint8_t>(t->d + 4) / 8);

        dx = (d % 8) * 32;
        dy = (d / 8) * 32;
        src.top = 304 + dy;
        src.left = 384 + dx;
        src.bottom = src.top + 32;
        src.right = src.left + 32;
        // Note: (8,8) already subtracted from (x,y), so
        // subtract 8 more to correct to (-16,-16)
        x -= 8; // This is where the hit
        y -= 8; // coordinate correction is done
      }
      break;
    }

    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  // Draw small bullet (8*8) //
  for (const auto i : std::views::iota(0U, count_small)) {
    auto *t = &bullets[indices_small[i]];

    x = (t->x >> 6) - 4; // -4 is for coordinate correction
    y = (t->y >> 6) - 4; // Same as above

    switch (t->effect) {
    case TE_DELETE:
      src = PIXEL_LTWH{(384 + ((t->count / 6) << 3)), 120, 8, 8};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      continue;

    case TE_CIRCLE1:
      TamaEffectDraw(t);
      continue;

      // Ignore others //
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

// Draw bullet as effect? //
namespace {
constexpr auto RCSET(int x, int y, int w) -> PIXEL_LTRB {
  return {x, y, x + w, y + w};
}
} // namespace
void TamaEffectDraw(const Bullet *t) {

  static constexpr PIXEL_LTRB Data[6][5] = {
      // [Color][Pattern]
      {
          // Red //
          RCSET(168, 344, 32),
          RCSET(232, 344, 28),
          RCSET(288, 344, 24),
          RCSET(336, 344, 20),
          RCSET(328, 416, 16),
      },
      {
          // Blue //
          RCSET(168, 344 + 32, 32),
          RCSET(232, 344 + 28, 28),
          RCSET(288, 344 + 24, 24),
          RCSET(336, 344 + 20, 20),
          RCSET(328 + 16, 416, 16),
      },
      {
          // Green //
          RCSET(168, 344 + (32 * 2), 32),
          RCSET(232, 344 + (28 * 2), 28),
          RCSET(288, 344 + (24 * 2), 24),
          RCSET(336, 344 + (20 * 2), 20),
          RCSET(328 + (16 * 2), 416, 16),
      },
      {
          // Purple //
          RCSET(168 + 32, 344, 32),
          RCSET(232 + 28, 344, 28),
          RCSET(288 + 24, 344, 24),
          RCSET(336 + 20, 344, 20),
          RCSET(328, 416 + 16, 16),
      },
      {
          // Silver //
          RCSET(168 + 32, 344 + 32, 32),
          RCSET(232 + 28, 344 + 28, 28),
          RCSET(288 + 24, 344 + 24, 24),
          RCSET(336 + 20, 344 + 20, 20),
          RCSET(328 + 16, 416 + 16, 16),
      },
      {
          // Orange //
          RCSET(168 + 32, 344 + (32 * 2), 32),
          RCSET(232 + 28, 344 + (28 * 2), 28),
          RCSET(288 + 24, 344 + (24 * 2), 24),
          RCSET(336 + 20, 344 + (20 * 2), 20),
          RCSET(328 + (16 * 2), 416 + 16, 16),
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
  int x = 0;
  int y = 0;

  x = (t->x >> 6) - Width[ptn];
  y = (t->y >> 6) - Width[ptn];

  // [Color][Pattern]
  // temp = Data[(t->c&0x0f)%6][ptn];
  if (t->c >= 16 * 3) {
    temp = Target[3][ptn];
  } else {
    temp = Target[t->c][ptn];
  }
  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, temp);
}

void BulletManager::Clear() {
  for (const auto i : std::views::iota(0U, count_small)) {
    auto &t = bullets[indices_small[i]];
    if (t.effect != TE_DELETE) {
      t.effect = TE_DELETE;
      t.count = 0;
      // t.c      = 0x25;
      t.d = 0;
    }
  }

  for (const auto i : std::views::iota(0U, count_large)) {
    auto &t = bullets[indices_large[i]];
    if (t.effect != TE_DELETE) {
      t.effect = TE_DELETE;
      t.count = 0;
      // t.c      = 0x25;
      t.d = 0;
    }
  }
}

// Convert bullets to score (Ret: score)
uint32_t BulletManager::ScoreToItems() {
  uint32_t sum = 0;
  uint32_t Score = 0;

  Score = TAMA1_POINT + (Players.GrazeCount() * 100);
  for (const auto i : std::views::iota(0U, count_small)) {
    auto *t = &bullets[indices_small[i]];
    if (t->effect != TE_DELETE) {
      Effects.SpawnPointEffect(t->x - (64 * 4), t->y - (64 * 4), Score);
      sum += Score;
      t->flag = TF_DELETE;
      t->count = 0;
      t->c = 0x25;
      t->d = 0;
    }
  }
  Indsort(indices_small, count_small, bullets,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  Score = TAMA2_POINT + (Players.GrazeCount() * 100);
  for (const auto i : std::views::iota(0U, count_large)) {
    auto *t = &bullets[indices_large[i]];
    if (t->effect != TE_DELETE) {
      Effects.SpawnPointEffect(t->x - (64 * 8), t->y - (64 * 8), Score);
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

// Convert bullets to items //
void BulletManager::ToItems(uint8_t n) {
  // uint32_t sum = 0;
  // uint32_t Score;

  //	Score = TAMA1_POINT + Players.GrazeCount() * 100;

  if (n == 0) {
    Clear();
    return;
  }

  for (const auto i : std::views::iota(0U, count_small)) {
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

  //	Score = TAMA2_POINT + Players.GrazeCount() * 100;
  for (const auto i : std::views::iota(0U, count_large)) {
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
  int i = 0;

  if (tama1 >= TAMA_MAX) {
    tama1 = TAMA_MAX - 1;
  }

  // Set max bullet count //
  max_small = tama1;
  max_large = TAMA_MAX - tama1;

  // Initialize bullet index array //
  for (i = 0; std::cmp_less(i, tama1); i++) {
    indices_small[i] = i;
  }
  for (i = tama1; i < TAMA_MAX; i++) {
    indices_large[i - tama1] = i;
  }

  // memset(bullets,0,sizeof(TAMA_DATA)*TAMA_MAX);

  count_small = count_large = 0;
}

void BulletManager::SetEasy() {
  switch (command.cmd & 0x03) {
  case TC_WAY:
    if (command.n >= 3) {
      command.n -= 2; // Don't change odd/even
    }
    command.dw += (command.dw >> 2); // Widen spread
    break;

  case TC_ALL:
  case TC_RND:
    command.n >>= 1; // Halve bullet count
    break;
  }

  if (command.ns >= 2) {
    command.ns--; // Decrease rapid fire count
  }
}

void BulletManager::SetHard() {
  switch (command.cmd & 0x03) {
  case TC_WAY:
    command.n += 2;                  // Don't change odd/even
    command.dw -= (command.dw >> 3); // Narrow spread
    break;

  case TC_ALL:
    command.n += (((command.n >> 2) > 6) ? 6 : (command.n >> 2));
    break;

  case TC_RND:
    command.n += (command.n >> 1); // Bullet count 50% up
    break;
  }

  command.ns++; // Increase rapid fire count
}

void BulletManager::SetLunatic() {
  switch (command.cmd & 0x03) {
  case TC_WAY:
    command.n += 4;                 // Don't change odd/even
    command.dw -= (command.dw / 3); // Narrow spread
    break;

  case TC_ALL:
    command.n += (((command.n / 3) > 12) ? 12 : (command.n / 3));
    break;

  case TC_RND:
    command.n <<= 1; // Double bullet count
    break;
  }

  command.ns += 2; // Increase rapid fire count
}

uint8_t BulletManager::Dir(uint16_t i) const {
  uint8_t deg =
      (((command.cmd & TAMA_ZSET) != 0)
           ? atan8((Players.X() - command.x), (Players.Y() - command.y))
           : 0);

  deg += command.d;  // Base angle set complete
  i = i % command.n; // Rapid fire countermeasure

  switch (command.cmd & 0x03) {
  case TC_WAY:
    i++;
    if ((command.n & 1) != 0) {
      return deg + ((i >> 1) * command.dw * (1 - ((i & 1) << 1)));
    }
    return deg - (command.dw >> 1) +
           ((i >> 1) * command.dw * (1 - ((i & 1) << 1)));

  case TC_ALL:
    return deg + ((i << 8) / command.n);

  case TC_RND:
    // DebugOut("1");
    return deg + (rnd() % command.dw) - (command.dw >> 1);

  default:
    return 0; // Should never happen, but warning is annoying...
  }
}

int BulletManager::NewSpeed(uint16_t i) const {
  int temp = 0;           // For setting random element
  const int vret = speed; // SPEEDM(command.v);	// Set base speed value (GIAN.H)

  // Speed randomness should maybe be n% of base value, but... //
  switch (command.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8; // DebugOut("2");
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16; // DebugOut("3");
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32; // DebugOut("4");
    break;
  }

  if ((command.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i / command.n)) + temp;
  }
  return vret + temp;
}

int BulletManager::LineCmdNewSpeed(uint16_t i) const {
  int vret = speed; // Set base speed value (GIAN.H)

  i = (i % command.n) + 1; // Rapid fire countermeasure

  // Angle from center
  const auto deg_factor = ((i >> 1) * command.dw * (1 - ((i & 1) << 1)));
  const uint8_t deg =
      (((command.n & 1) != 0) ? deg_factor : -(command.dw >> 1) + deg_factor);

  vret = cosDiv(deg, vret);

  if ((command.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i - 1));
  }
  return vret;
}

int BulletManager::Speed(uint16_t i) const {
  int temp = 0;                       // For setting random element
  const int vret = SPEEDM(command.v); // Set base speed value (GIAN.H)

  // Speed randomness should maybe be n% of base value, but... //
  switch (command.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8; // DebugOut("2");
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16; // DebugOut("3");
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32; // DebugOut("4");
    break;
  }

  if ((command.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i / command.n)) + temp;
  }
  return vret + temp;
}

uint8_t BulletManager::Flag() const {
  switch (command.type) {
  case T_HOMING:
  case T_HOMING_M:
  case T_ROLL:
  case T_ROLL_A:
  case T_ROLL_R:
  case T_SBHOMING:
    return TF_CLIP;

  default:
    return TF_NONE;
  }
}

void BulletManager::MoveByType(Bullet *t) {
  short deg_t = 0;
  // ENEMY_DATA	*e;

  // Access (tx, ty) instead of directly accessing (x, y)! //
  switch (t->type & 0x0f) {
  case T_NORM: // Normal bullet
    // MMX_ADD32(&t->tx,&t->vx);
    t->tx += t->vx;
    t->ty += t->vy;
    return;

  case T_NORM_A: // Accelerating bullet
    t->v += t->a;
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // Preserve upper bits just in case
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case T_HOMING: // N-times homing
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
      t->d = atan8((Players.X()) - (t->x), (Players.Y()) - (t->y));
    }
    return;

  case T_HOMING_M: // N% homing (missile type?)
    // Not optimized... //
    if ((t->count > 19) && (t->count % 2 == 0)) {
      deg_t = atan8((Players.X()) - (t->x), (Players.Y()) - (t->y)) - (t->d);
      if (deg_t < -128) {
        deg_t += 256;
      }
      if (deg_t > 128) {
        deg_t -= 256;
      }
      t->d = t->d + (deg_t * (t->vd) / 255);
    }
    t->v += t->a;
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // Preserve upper bits just in case
      t->flag &= (~TF_CLIP);
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case T_ROLL: // Rolling bullet
    t->d += Cast::sign<uint8_t>(t->vd);
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // Preserve upper bits just in case
      t->flag &= (~TF_CLIP);
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case T_ROLL_A: // Rolling bullet (accelerating) initial acceleration must be
                 // "negative"!
    t->v += t->a;
    if (t->a > 0) {
      t->d += Cast::sign<uint8_t>(t->vd);
    }
    t->tx += cosl(t->d, t->v);
    t->ty += sinl(t->d, t->v);
    if ((t->a < 0) && (t->v <= 0)) {
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

  case T_ROLL_R: // Rolling bullet (reversing) same as above, watch
                 // acceleration!
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

  case T_GRAVITY: // Falling bullet (can also rise though...)
    t->vy += t->a;
    // MMX_ADD32(&t->tx,&t->vx);
    t->tx += t->vx;
    t->ty += t->vy;
    return;

  case T_CHANGE: // Forced angle change bullet
    // MMX_ADD32(&t->tx,&t->vx);
    t->tx += t->vx;
    t->ty += t->vy;
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM; // Preserve upper bits just in case
      t->d = Cast::sign<uint8_t>(t->vd);
      t->vx = cosl(t->d, t->v);
      t->vy = sinl(t->d, t->v);
    }
    return;

  case T_SBHOMING: // Cactus homing (emits smoke!)
    if ((t->count & 1) != 0) {
      Effects.SpawnFragment(t->x, t->y, FRG_SMOKE);
    }
    t->tx += t->vx;
    t->ty += t->vy;
    if ((t->count < 130 - 60) && Enemies.homing_flag != HOMING_DUMMY) {
      deg_t =
          atan8(Enemies.homing_x - (t->x), Enemies.homing_y - (t->y)) - (t->d);
    } else if (t->count < 130 - 60) {
      deg_t = atan8(0, (-20 * 64) - (t->y)) - (t->d);
    } else {
      t->flag = TF_NONE;
      deg_t = 0;
    }

    if (deg_t < -128) {
      deg_t += 256;
    }
    if (deg_t > 128) {
      deg_t -= 256;
    }
    // if(deg_t>-2 && deg_t<2){
    if (deg_t == 0) {
      if (t->vd != 0) {
        t->vd--;
      }
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

  case T_SBHBOMB: // Cactus homing bomb
    // Note: This case is a dummy and must never be executed //
    if (t->count >= 49) {
      t->flag = TF_DELETE;
    }
    return;
  }
}

void BulletManager::MoveByOption(Bullet *t) {
  int op_temp = 0;

  // Division and bomb need to set the delete request flag //
  // Assign values to (x,y) using calculation results from (tx,ty) //
  switch (t->option & 0xf0) {
  case TOP_NONE: // No option
    t->x = t->tx;
    t->y = t->ty;
    return;

  case TOP_WAVE: // Wave
    op_temp = sinl(Cast::down_sign<uint8_t>(t->count << 2),
                   ((t->option & 0x0f) << 7));
    t->x = t->tx - sinl(t->d, op_temp);
    t->y = t->ty + cosl(t->d, op_temp);
    return;

  case TOP_ROLL: { // Rotation
    const auto angle = Cast::down_sign<uint8_t>(t->d + (t->count << 1));
    op_temp = (t->option & 0x0f) << 8;
    t->x = (t->tx + cosl(angle, op_temp));
    t->y = (t->ty + sinl(angle, op_temp));
  }
    return;

  case TOP_PURU: // Wobble
    return;

  case TOP_REFX: // Reflect X
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      t->d = 128 - t->d;
      t->vx = -(t->vx);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0) {
        t->option = TOP_NONE;
      } else {
        t->option = TOP_REFX | (op_temp - 1);
      }
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;

  case TOP_REFY: // Reflect Y
    if ((t->ty) < GY_MIN) {
      t->d = -t->d;
      t->vy = -(t->vy);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0) {
        t->option = TOP_NONE;
      } else {
        t->option = TOP_REFY | (op_temp - 1);
      }
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;

  case TOP_REFXY: // Reflect XY
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      t->d = 128 - t->d;
      t->vx = -(t->vx);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0) {
        t->option = TOP_NONE;
      } else {
        t->option = TOP_REFXY | (op_temp - 1);
      }
    } else if ((t->ty) < GY_MIN) {
      t->d = -t->d;
      t->vy = -(t->vy);
      t->x = t->tx + cosl(t->d, t->v);
      t->y = t->ty + sinl(t->d, t->v);
      op_temp = (t->option & 0x0f);
      if (op_temp == 0) {
        t->option = TOP_NONE;
      } else {
        t->option = TOP_REFXY | (op_temp - 1);
      }
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;

  case TOP_DIV: // Division
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
      t->flag = TF_DELETE; // Should this be changed to a death effect?
      command.ns = 2;
      command.c = (t->c) & 0x0f;
      command.cmd = (t->option & 0x0f) | TE_CIRCLE1;
      switch (command.cmd & 0x03) {
      case TC_WAY:
        command.n = 3;
        command.dw = 16;
        command.v = 13 - 2;
        break;
      case TC_ALL:
        command.n = 10;
        command.v = 13;
        command.d = Cast::down<uint8_t>(rnd());
        if ((command.cmd & TAMA_REN) != 0) {
          command.v -= 2;
        }
        break;
      case TC_RND:
        command.n = 4;
        command.dw = 128 - 32;        // Above 128 would be off-screen...
        command.v = 13 | TAMASP_RND2; // Random speed enabled
        break;
      }
      if ((command.cmd & TAMA_ZSET) != 0) {
        command.d = 0, command.dw -= 6;
      }
      command.type = T_NORM;
      command.option = TOP_NONE;
      Snd_SEPlay(12, command.x);
      Spawn(); // Key point is that it varies by difficulty
    }
    return;

  case TOP_BOMB: // Bomb
    return;
  }
}

void BulletManager::MoveByEffect(Bullet *t) {
  // TE_NONE: no effect, never comes to this function so writing here is
  // meaningless // TE_DELETE: don't forget to set the delete request flag! //
  switch (t->effect & 0xf0) {
  case TE_ROLL1:
    return;

  case TE_ROLL2:
    return;

  case TE_WARN:
    return;

  case TE_ROCK:
    return;

  case TE_CIRCLE1:
    t->x = (t->tx += (t->vx >> 1));
    t->y = (t->ty += (t->vy >> 1));

    if (t->count >= (5 * 4) - 1) {
      t->effect = 0;
    }
    return;

  case TE_CIRCLE2:
    return;

  case TE_DELETE:
    t->x += (t->vx >> 1);
    t->y += (t->vy >> 1);
    // t->d+=4;
    if (t->count >= 47) {
      t->flag = TF_DELETE;
    }
    return;
  }
}
