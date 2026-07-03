///
/// BulletSubsystem - implementation (migrated from bullet.cpp).
///

#include <utility>

#include "bullet_subsystem.h"

#include "audio/snd.h"
#include "core/entity.h"
#include "core/gian.h"
#include "core/level.h"
#include "gfx/graphics_backend.h"
#include "util/cast.h"
#include "util/ut_math.h"

namespace bullets {

namespace {

// Draw helper used by TamaEffectDraw — local to this TU.
constexpr PIXEL_LTRB RCSET(int x, int y, int w) { return {x, y, x + w, y + w}; }

void TamaEffectDraw(const Bullet *t) {
  static constexpr PIXEL_LTRB Data[6][5] = {
      {RCSET(168, 344, 32), RCSET(232, 344, 28), RCSET(288, 344, 24),
       RCSET(336, 344, 20), RCSET(328, 416, 16)},
      {RCSET(168, 344 + 32, 32), RCSET(232, 344 + 28, 28),
       RCSET(288, 344 + 24, 24), RCSET(336, 344 + 20, 20),
       RCSET(328 + 16, 416, 16)},
      {RCSET(168, 344 + (32 * 2), 32), RCSET(232, 344 + (28 * 2), 28),
       RCSET(288, 344 + (24 * 2), 24), RCSET(336, 344 + (20 * 2), 20),
       RCSET(328 + (16 * 2), 416, 16)},
      {RCSET(168 + 32, 344, 32), RCSET(232 + 28, 344, 28),
       RCSET(288 + 24, 344, 24), RCSET(336 + 20, 344, 20),
       RCSET(328, 416 + 16, 16)},
      {RCSET(168 + 32, 344 + 32, 32), RCSET(232 + 28, 344 + 28, 28),
       RCSET(288 + 24, 344 + 24, 24), RCSET(336 + 20, 344 + 20, 20),
       RCSET(328 + 16, 416 + 16, 16)},
      {RCSET(168 + 32, 344 + (32 * 2), 32), RCSET(232 + 28, 344 + (28 * 2), 28),
       RCSET(288 + 24, 344 + (24 * 2), 24), RCSET(336 + 20, 344 + (20 * 2), 20),
       RCSET(328 + (16 * 2), 416 + 16, 16)},
  };

  static int Width[5] = {32 / 2, 28 / 2, 24 / 2, 20 / 2, 16 / 2};
  static constexpr std::span<const PIXEL_LTRB, 5> Target[16 * 3] = {
      Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[0], Data[0],
      Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],
      Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[0], Data[0],
      Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],
      Data[0], Data[1], Data[5], Data[3], Data[4], Data[5], Data[0], Data[0],
      Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],
  };

  const int ptn = ((t->count / 4) % 5);
  const int x = (t->x >> 6) - Width[ptn];
  const int y = (t->y >> 6) - Width[ptn];

  const PIXEL_LTRB temp = (t->c >= 16 * 3) ? Target[3][ptn] : Target[t->c][ptn];
  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, temp);
}

// Graze — formerly TamaEvadeAdd.
inline void EvadeAdd(Bullet *t, Player &players) {
  if (t->flag & TF_EVADE) {
    players.AddEvadeEx(t->x, t->y, 0);
  } else {
    t->flag |= TF_EVADE;
    players.AddEvadeEx(t->x, t->y, TAMA_EVADE);
  }
}

} // namespace

// ============================================================
//  Public hit-radius helpers
// ============================================================
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

// ============================================================
//  Construction & reset
// ============================================================
BulletSubsystem::BulletSubsystem(world::Refs w) : world_(w) {}

void BulletSubsystem::Reset() {
  for (size_t i = 0; i < kEnemySmallMax; ++i)
    enemy_small_idx_[i] = static_cast<uint16_t>(i);
  for (size_t i = 0; i < kEnemyLargeMax; ++i)
    enemy_large_idx_[i] = static_cast<uint16_t>(i);
  ResetPlayerIndices();
  enemy_small_now_ = enemy_large_now_ = 0;
}

void BulletSubsystem::ResetEnemyIndices() { Reset(); }

void BulletSubsystem::ResetPlayerIndices() {
  for (size_t i = 0; i < kPlayerMax; ++i)
    player_idx_[i] = static_cast<uint16_t>(i);
  player_now_ = 0;
}

// ============================================================
//  Enemy spawn API
// ============================================================
void BulletSubsystem::Spawn(const BulletCommand &cmd_in) {
  BulletCommand cmd = cmd_in;

  switch (world_.ranking.state.GameLevel) {
  case GameLevel::EASY:
    SetEasy(cmd);
    break;
  case GameLevel::NORMAL:
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    SetHard(cmd);
    break;
  case GameLevel::LUNATIC:
    SetLunatic(cmd);
    break;
  }

  const int v = SPEEDM(cmd.v);
  if ((cmd.type & 0x0f) == T_NORM) {
    speed_ = (((v >> 1) * (world_.ranking.state.Rank)) >> (5 + 8)) + (v >> 1);
  } else {
    speed_ = v;
  }
  TamaSetMain(cmd);
}

void BulletSubsystem::SpawnEX(const BulletCommand &cmd) {
  speed_ = SPEEDM(cmd.v);
  TamaSetMain(cmd);
}

void BulletSubsystem::SpawnLine(const BulletCommand &cmd_in) {
  BulletCommand cmd = cmd_in;
  speed_ = SPEEDM(cmd.v);
  cmd.cmd = (cmd.cmd & 0xf0) | TC_WAY;

  const auto setmax =
      static_cast<uint16_t>(cmd.n * (((cmd.cmd & TAMA_REN) != 0) ? cmd.ns : 1));
  for (uint16_t i = 0; i < setmax; ++i) {
    auto *t = AllocEnemy(cmd.c);
    if (t == nullptr)
      return;
    t->x = t->tx = cmd.x;
    t->y = t->ty = cmd.y;
    t->a = cmd.a;
    const uint8_t deg = Dir(i, cmd);
    t->d = ut_math_detail::deg256_to_rad(deg);
    t->v = t->v0 = LineCmdNewSpeed(i, cmd);
    t->vx = cos_len(t->d, t->v);
    t->vy = sin_len(t->d, t->v);
    t->vd = cmd.vd;
    t->c = cmd.c;
    t->rep = cmd.rep;
    t->type = cmd.type;
    t->option = cmd.option;
    t->effect = cmd.cmd & 0xf0;
    t->count = 0;
    t->flag = FlagForCmd(cmd);
  }
}

void BulletSubsystem::SpawnExtra01(const BulletCommand &cmd_in) {
  BulletCommand cmd = cmd_in;
  speed_ = SPEEDM(cmd.v);

  const auto setmax =
      static_cast<uint16_t>(cmd.n * (((cmd.cmd & TAMA_REN) != 0) ? cmd.ns : 1));
  for (uint16_t i = 0; i < setmax; ++i) {
    auto *t = AllocEnemy(cmd.c);
    if (t == nullptr)
      return;
    t->x = t->tx = cmd.x;
    t->y = t->ty = cmd.y;
    t->a = cmd.a;
    const uint8_t deg = Dir(i, cmd);
    t->d = ut_math_detail::deg256_to_rad(deg);
    t->v = t->v0 = SpeedEx(deg, cmd);
    t->vx = cos_len(t->d, t->v);
    t->vy = sin_len(t->d, t->v);
    t->vd = cmd.vd;
    t->c = cmd.c;
    t->rep = cmd.rep;
    t->type = cmd.type;
    t->option = cmd.option;
    t->effect = cmd.cmd & 0xf0;
    t->count = 0;
    t->flag = FlagForCmd(cmd);
  }
}

void BulletSubsystem::TamaSetMain(const BulletCommand &cmd) {
  const auto setmax =
      static_cast<uint16_t>(cmd.n * (((cmd.cmd & TAMA_REN) != 0) ? cmd.ns : 1));
  for (uint16_t i = 0; i < setmax; ++i) {
    auto *t = AllocEnemy(cmd.c);
    if (t == nullptr)
      return;
    t->x = t->tx = cmd.x;
    t->y = t->ty = cmd.y;
    t->v = t->v0 = NewSpeed(i, cmd);
    t->a = cmd.a;
    const uint8_t deg = Dir(i, cmd);
    t->d = ut_math_detail::deg256_to_rad(deg);
    t->vx = cos_len(t->d, t->v);
    t->vy = sin_len(t->d, t->v);
    t->vd = cmd.vd;
    t->c = cmd.c;
    t->rep = cmd.rep;
    t->type = cmd.type;
    t->option = cmd.option;
    t->effect = cmd.cmd & 0xf0;
    t->count = 0;
    t->flag = FlagForCmd(cmd);
  }
}

Bullet *BulletSubsystem::AllocEnemy(uint8_t c) {
  if ((c & 0xF0) == TAMA_SMALL) {
    if (enemy_small_now_ + 1 >= kEnemySmallMax)
      return nullptr;
    auto *t = &enemy_small_bullets_[enemy_small_idx_[enemy_small_now_]];
    enemy_small_now_++;
    return t;
  }
  if (enemy_large_now_ + 1 >= kEnemyLargeMax)
    return nullptr;
  auto *t = &enemy_large_bullets_[enemy_large_idx_[enemy_large_now_]];
  enemy_large_now_++;
  return t;
}

// ============================================================
//  Difficulty adjusters
// ============================================================
void BulletSubsystem::SetEasy(BulletCommand &cmd) const {
  switch (cmd.cmd & 0x03) {
  case TC_WAY:
    if (cmd.n >= 3)
      cmd.n -= 2;
    cmd.dw += (cmd.dw >> 2);
    break;
  case TC_ALL:
  case TC_RND:
    cmd.n >>= 1;
    break;
  }
  if (cmd.ns >= 2)
    cmd.ns--;
}

void BulletSubsystem::SetHard(BulletCommand &cmd) const {
  switch (cmd.cmd & 0x03) {
  case TC_WAY:
    cmd.n += 2;
    cmd.dw -= (cmd.dw >> 3);
    break;
  case TC_ALL:
    cmd.n += (((cmd.n >> 2) > 6) ? 6 : (cmd.n >> 2));
    break;
  case TC_RND:
    cmd.n += (cmd.n >> 1);
    break;
  }
  cmd.ns++;
}

void BulletSubsystem::SetLunatic(BulletCommand &cmd) const {
  switch (cmd.cmd & 0x03) {
  case TC_WAY:
    cmd.n += 4;
    cmd.dw -= (cmd.dw / 3);
    break;
  case TC_ALL:
    cmd.n += (((cmd.n / 3) > 12) ? 12 : (cmd.n / 3));
    break;
  case TC_RND:
    cmd.n <<= 1;
    break;
  }
  cmd.ns += 2;
}

// ============================================================
//  Direction / speed calculators
// ============================================================
uint8_t BulletSubsystem::Dir(uint16_t i, const BulletCommand &cmd) const {
  uint8_t deg =
      (((cmd.cmd & TAMA_ZSET) != 0)
           ? atan8((world_.players.X() - cmd.x), (world_.players.Y() - cmd.y))
           : 0);
  deg += cmd.d;
  i = i % cmd.n;
  switch (cmd.cmd & 0x03) {
  case TC_WAY:
    i++;
    if ((cmd.n & 1) != 0) {
      return deg + ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));
    }
    return deg - (cmd.dw >> 1) + ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));
  case TC_ALL:
    return deg + ((i << 8) / cmd.n);
  case TC_RND:
    return deg + (rnd() % cmd.dw) - (cmd.dw >> 1);
  default:
    return 0;
  }
}

int BulletSubsystem::NewSpeed(uint16_t i, const BulletCommand &cmd) const {
  int temp = 0;
  const int vret = speed_;
  switch (cmd.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8;
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16;
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32;
    break;
  }
  if ((cmd.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i / cmd.n)) + temp;
  }
  return vret + temp;
}

int BulletSubsystem::LineCmdNewSpeed(uint16_t i,
                                     const BulletCommand &cmd) const {
  int vret = speed_;
  i = (i % cmd.n) + 1;
  const auto deg_factor = ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));
  const uint8_t deg =
      (((cmd.n & 1) != 0) ? deg_factor : -(cmd.dw >> 1) + deg_factor);
  vret = cosDiv(deg, vret);
  if ((cmd.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i - 1));
  }
  return vret;
}

int BulletSubsystem::SpeedEx(uint8_t d, const BulletCommand &cmd) const {
  int temp = 0;
  switch (cmd.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8;
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16;
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32;
    break;
  }
  int delta = cmd.d - d;
  if (delta > 128)
    delta -= 256;
  if (delta < -128)
    delta += 256;
  return speed_ - ((speed_ * abs(delta)) / 23) + temp;
}

int BulletSubsystem::SpeedFromCmd(uint16_t i, const BulletCommand &cmd) const {
  int temp = 0;
  const int vret = SPEEDM(cmd.v);
  switch (cmd.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8;
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16;
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32;
    break;
  }
  if ((cmd.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i / cmd.n)) + temp;
  }
  return vret + temp;
}

uint8_t BulletSubsystem::FlagForCmd(const BulletCommand &cmd) const {
  switch (cmd.type) {
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

// ============================================================
//  Enemy Move / Draw / Clear / Item conversion
// ============================================================
void BulletSubsystem::Move() {
  auto process = [this](Bullet *t) {
    if (t->effect == TE_NONE) {
      MoveByType(t, world_);
      MoveByOption(t);
      if (((t->flag & TF_CLIP) == 0) &&
          ((t->x) < GX_MIN - (4 * 64) || (t->x) > GX_MAX + (4 * 64) ||
           (t->y) < GY_MIN - (4 * 64) || (t->y) > GY_MAX + (4 * 64))) {
        t->flag = TF_DELETE;
      }
      t->count++;
      if (world_.players.IsInvincible())
        return;

      const int ev_r = GetBulletEvadeRadius(t->c);
      if (world_.players.HitCheck(t->x, t->y, ev_r))
        EvadeAdd(t, world_.players);

      const int r = GetBulletHitRadius(t->c);
      if (world_.players.HitCheck(t->x, t->y, r)) {
        t->flag = TF_DELETE;
        world_.players.OnHit();
      }
    } else {
      MoveByEffect(t);
      t->count++;
    }
  };

  for (uint16_t i = 0; i < enemy_small_now_; ++i) {
    process(&enemy_small_bullets_[enemy_small_idx_[i]]);
  }
  Indsort(enemy_small_idx_, enemy_small_now_, enemy_small_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  for (uint16_t i = 0; i < enemy_large_now_; ++i) {
    process(&enemy_large_bullets_[enemy_large_idx_[i]]);
  }
  Indsort(enemy_large_idx_, enemy_large_now_, enemy_large_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });
}

void BulletSubsystem::Draw() {
  PIXEL_LTRB src;
  int x = 0, y = 0, dx = 0, dy = 0;

  static const PIXEL_LTRB rcExtraTama[4] = {
      {128, 384, 128 + 32, 384 + 32},
      {128 + 32, 384, 128 + 56, 384 + 24},
      {128 + 56, 384, 128 + 72, 384 + 16},
      {128 + 72, 384, 128 + 80, 384 + 8},
  };
  static constexpr uint8_t sizeExtraTama[4] = {16, 12, 8, 4};

  for (uint16_t i = 0; i < enemy_large_now_; ++i) {
    auto *t = &enemy_large_bullets_[enemy_large_idx_[i]];
    x = (t->x >> 6) - 8;
    y = (t->y >> 6) - 8;

    switch (t->effect) {
    case TE_DELETE:
      src = PIXEL_LTWH{(384 + ((t->count / 6) << 4)), 104, 16, 16};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      continue;
    case TE_CIRCLE1:
      TamaEffectDraw(t);
      continue;
    default:
      break;
    }

    switch (t->c & 0xf0) {
    case TAMA_LARGE:
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
      const auto d = (Cast::down_sign<uint8_t>(ut_math_detail::rad_to_deg256(t->d) + 4) / 8);
      src.top = 320 + ((t->c & 3) << 4);
      src.left = d * 16;
      src.bottom = src.top + 16;
      src.right = src.left + 16;
      GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY, src);
    }
      continue;
    case TAMA_ANGLE:
      if (t->c != 32 + 5) {
        src.top = 24 + ((t->c & 0x0f) << 4);
        src.left = ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0) + 384;
        src.bottom = src.top + 16;
        src.right = src.left + 16;
      } else {
        const auto d = (Cast::down_sign<uint8_t>(ut_math_detail::rad_to_deg256(t->d) + 4) / 8);
        dx = (d % 8) * 32;
        dy = (d / 8) * 32;
        src.top = 304 + dy;
        src.left = 384 + dx;
        src.bottom = src.top + 32;
        src.right = src.left + 32;
        x -= 8;
        y -= 8;
      }
      break;
    default:
      continue; // unreachable in original
    }
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  for (uint16_t i = 0; i < enemy_small_now_; ++i) {
    auto *t = &enemy_small_bullets_[enemy_small_idx_[i]];
    x = (t->x >> 6) - 4;
    y = (t->y >> 6) - 4;

    switch (t->effect) {
    case TE_DELETE:
      src = PIXEL_LTWH{(384 + ((t->count / 6) << 3)), 120, 8, 8};
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
      continue;
    case TE_CIRCLE1:
      TamaEffectDraw(t);
      continue;
    default:
      break;
    }

    if (t->c != 0x25) {
      src.top = 0;
      src.left = ((t->c) << 3) + 384;
      src.bottom = 8;
      src.right = src.left + 8;
    } else {
      src.top = 24 + ((t->c & 0x0f) << 4);
      src.left = ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0) + 384;
      src.bottom = src.top + 16;
      src.right = src.left + 16;
    }
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }
}

void BulletSubsystem::Clear() {
  for (uint16_t i = 0; i < enemy_small_now_; ++i) {
    auto &t = enemy_small_bullets_[enemy_small_idx_[i]];
    if (t.effect != TE_DELETE) {
      t.effect = TE_DELETE;
      t.count = 0;
      t.d = 0;
    }
  }
  for (uint16_t i = 0; i < enemy_large_now_; ++i) {
    auto &t = enemy_large_bullets_[enemy_large_idx_[i]];
    if (t.effect != TE_DELETE) {
      t.effect = TE_DELETE;
      t.count = 0;
      t.d = 0;
    }
  }
}

uint32_t BulletSubsystem::ScoreToItems() {
  uint32_t sum = 0;
  uint32_t Score = TAMA1_POINT + (world_.players.GrazeCount() * 100);
  for (uint16_t i = 0; i < enemy_small_now_; ++i) {
    auto *t = &enemy_small_bullets_[enemy_small_idx_[i]];
    if (t->effect != TE_DELETE) {
      world_.effects.SpawnPointEffect(t->x - (64 * 4), t->y - (64 * 4), Score);
      sum += Score;
      t->flag = TF_DELETE;
      t->count = 0;
      t->c = 0x25;
      t->d = 0;
    }
  }
  Indsort(enemy_small_idx_, enemy_small_now_, enemy_small_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  Score = TAMA2_POINT + (world_.players.GrazeCount() * 100);
  for (uint16_t i = 0; i < enemy_large_now_; ++i) {
    auto *t = &enemy_large_bullets_[enemy_large_idx_[i]];
    if (t->effect != TE_DELETE) {
      world_.effects.SpawnPointEffect(t->x - (64 * 8), t->y - (64 * 8), Score);
      sum += Score;
      t->flag = TF_DELETE;
      t->count = 0;
      t->c = 0x25;
      t->d = 0;
    }
  }
  Indsort(enemy_large_idx_, enemy_large_now_, enemy_large_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });
  return sum;
}

void BulletSubsystem::ToItems(uint8_t n) {
  if (n == 0) {
    Clear();
    return;
  }

  for (uint16_t i = 0; i < enemy_small_now_; ++i) {
    auto *t = &enemy_small_bullets_[enemy_small_idx_[i]];
    if (t->effect != TE_DELETE) {
      t->count = 0;
      t->d = 0;
      if (rnd() % n == 0) {
        world_.items.Spawn(t->x, t->y, ITEM_SCORE);
        t->flag = TF_DELETE;
        t->c = 0x25;
      } else {
        t->effect = TE_DELETE;
        t->count = 0;
        t->d = 0;
      }
    }
  }
  Indsort(enemy_small_idx_, enemy_small_now_, enemy_small_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  for (uint16_t i = 0; i < enemy_large_now_; ++i) {
    auto *t = &enemy_large_bullets_[enemy_large_idx_[i]];
    if (t->effect != TE_DELETE) {
      t->count = 0;
      t->d = 0;
      if (rnd() % n == 0) {
        world_.items.Spawn(t->x, t->y, ITEM_SCORE);
        t->flag = TF_DELETE;
        t->c = 0x25;
      } else {
        t->effect = TE_DELETE;
        t->count = 0;
        t->d = 0;
      }
    }
  }
  Indsort(enemy_large_idx_, enemy_large_now_, enemy_large_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });
}

// ============================================================
//  Player shot pool — formerly in Player/WeaponForm
// ============================================================
void BulletSubsystem::SpawnPlayer(const BulletCommand &cmd) {
  for (uint8_t i = 0; i < cmd.n; i++) {
    if (player_now_ + 1 >= kPlayerMax)
      return;

    auto *t = &player_bullets_[player_idx_[player_now_++]];
    t->x = t->tx = cmd.x;
    t->y = t->ty = cmd.y;
    t->v = t->v0 = SpeedFromCmd(i, cmd);
    t->a = cmd.a;
    const uint8_t deg = Dir(i, cmd);
    t->d = ut_math_detail::deg256_to_rad(deg);
    t->vx = cos_len(t->d, t->v);
    t->vy = sin_len(t->d, t->v);
    t->vd = cmd.vd;
    t->c = cmd.c;
    t->rep = cmd.rep;
    t->type = cmd.type;
    t->option = cmd.option;
    t->effect = 0;
    t->count = 0;
    t->flag = FlagForCmd(cmd);
  }
}

void BulletSubsystem::MovePlayer() {
  for (uint16_t i = 0; i < player_now_; ++i) {
    auto *t = &player_bullets_[player_idx_[i]];

    if (t->c == TID_HOMING_BOMB_B) {
      world_.enemies.DamageAt(t->x, t->y, TogeDamage[t->c]);
      t->count++;
      if (t->count >= 19)
        t->flag = TF_DELETE;
      continue;
    }
    if (t->effect == TE_NONE) {
      MoveByType(t, world_);
      MoveByOption(t);
      t->count++;
      if (((t->flag & TF_CLIP) == 0) && ((t->x) < GX_MIN || (t->x) > GX_MAX ||
                                         (t->y) < GY_MIN || (t->y) > GY_MAX)) {
        t->flag = TF_DELETE;
      }
      if (world_.enemies.DamageAt(t->x, t->y, TogeDamage[t->c])) {
        if (t->c == TID_HOMING_BOMB_A) {
          BulletCommand cmd{};
          cmd.cmd = TC_WAY;
          cmd.c = TID_HOMING_BOMB_B;
          cmd.type = T_SBHBOMB;
          cmd.x = t->x;
          cmd.y = t->y;
          cmd.d = -64;
          cmd.dw = 16;
          cmd.v = 10;
          cmd.a = 0;
          cmd.n = 1;
          cmd.ns = 0;
          SpawnPlayer(cmd);
        }
        t->flag = TF_DELETE;
        world_.effects.SpawnFragment(t->x, t->y, FRG_HIT);
      }
    } else {
      MoveByEffect(t);
    }
  }
  Indsort(player_idx_, player_now_, player_bullets_,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });
}

void BulletSubsystem::DrawPlayer() {
  PIXEL_LTRB src;
  PIXEL_LTRB ltemp;
  int x = 0, y = 0;
  static PIXEL_LTRB HomingBomb[5] = {
      {520, 104, 520 + 8, 104 + 8},   {528, 104, 528 + 16, 104 + 16},
      {544, 104, 544 + 24, 104 + 24}, {568, 104, 568 + 32, 104 + 32},
      {600, 104, 600 + 40, 104 + 40},
  };

  for (uint16_t i = 0; i < player_now_; ++i) {
    auto *t = &player_bullets_[player_idx_[i]];
    x = (t->x >> 6) - 8;
    y = (t->y >> 6) - 8;

    switch (t->c) {
    case TID_WIDE_MAIN:
    case TID_WIDE_FOCUS_MAIN:
      src = PIXEL_LTWH{(384 + ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0)), 176, 16, 16};
      break;
    case TID_WIDE_SUB:
    case TID_WIDE_FOCUS_SUB:
      src = PIXEL_LTWH{(384 + ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0)), 192, 16, 16};
      break;
    case TID_HOMING_MAIN:
    case TID_HOMING_FOCUS_MAIN:
      src = PIXEL_LTWH{(384 + ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0)), 208, 16, 16};
      break;
    case TID_HOMING_SUB:
    case TID_HOMING_FOCUS_SUB:
      src = PIXEL_LTWH{(384 + ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0)), 224, 16, 16};
      break;
    case TID_HOMING_BOMB_A:
      src = PIXEL_LTWH{(384 + ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0)), 288, 16, 16};
      break;
    case TID_LASER_SUB:
      src = PIXEL_LTWH{(384 + ((ut_math_detail::rad_to_deg256(t->d) + 8) & 0xf0)), 256, 16, 16};
      break;
    case TID_HOMING_BOMB_B:
      src = HomingBomb[(t->count / 4) % 5];
      break;
    default:
      continue;
    }
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }
}

// ============================================================
//  Movement dispatchers (formerly BulletManager::MoveByType / ByOption /
//  ByEffect)
// ============================================================
void BulletSubsystem::MoveByType(Bullet *t, world::Refs w) {
  double deg_t = 0.0; // homing angle delta (radians)
  switch (t->type & 0x0f) {
  case T_NORM:
    t->tx += t->vx;
    t->ty += t->vy;
    return;
  case T_NORM_A:
    t->v += t->a;
    t->tx += cos_len(t->d, t->v);
    t->ty += sin_len(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM;
      t->vx = cos_len(t->d, t->v);
      t->vy = sin_len(t->d, t->v);
    }
    return;
  case T_HOMING:
    t->v += t->a;
    t->tx += cos_len(t->d, t->v);
    t->ty += sin_len(t->d, t->v);
    if ((t->a > 0) && (t->v >= t->v0)) {
      t->a = -(t->a);
      if (--(t->rep) == 0) {
        t->type = (t->type & 0xf0) | T_NORM;
        t->flag &= (~TF_CLIP);
        t->vx = cos_len(t->d, t->v);
        t->vy = sin_len(t->d, t->v);
      }
    }
    if ((t->a < 0) && (t->v <= 0)) {
      t->a = -(t->a);
      t->d = atan2_rad((w.players.Y()) - (t->y), (w.players.X()) - (t->x));
    }
    return;
  case T_HOMING_M:
    if ((t->count > 19) && (t->count % 2 == 0)) {
      deg_t = wrap_pi(atan2_rad((w.players.Y()) - (t->y),
                                (w.players.X()) - (t->x)) -
                      t->d);
      t->d = t->d + (deg_t * (t->vd) / 255.0);
    }
    t->v += t->a;
    t->tx += cos_len(t->d, t->v);
    t->ty += sin_len(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM;
      t->flag &= (~TF_CLIP);
      t->vx = cos_len(t->d, t->v);
      t->vy = sin_len(t->d, t->v);
    }
    return;
  case T_ROLL:
    t->d += t->vd * ut_math_detail::DEG256_TO_RAD;
    t->tx += cos_len(t->d, t->v);
    t->ty += sin_len(t->d, t->v);
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM;
      t->flag &= (~TF_CLIP);
      t->vx = cos_len(t->d, t->v);
      t->vy = sin_len(t->d, t->v);
    }
    return;
  case T_ROLL_A:
    t->v += t->a;
    if (t->a > 0)
      t->d += t->vd * ut_math_detail::DEG256_TO_RAD;
    t->tx += cos_len(t->d, t->v);
    t->ty += sin_len(t->d, t->v);
    if ((t->a < 0) && (t->v <= 0))
      t->a = -(t->a);
    if ((t->a > 0) && (t->v >= t->v0)) {
      t->a = -(t->a);
      if (--(t->rep) == 0) {
        t->type = (t->type & 0xf0) | T_NORM;
        t->flag &= (~TF_CLIP);
        t->vx = cos_len(t->d, t->v);
        t->vy = sin_len(t->d, t->v);
      }
    }
    return;
  case T_ROLL_R:
    t->v += t->a;
    t->d += t->vd * ut_math_detail::DEG256_TO_RAD;
    t->tx += cos_len(t->d, t->v);
    t->ty += sin_len(t->d, t->v);
    if ((t->a < 0) && (t->v <= 0)) {
      t->d += ut_math_detail::PI;
      t->a = -(t->a);
    }
    if ((t->a > 0) && (t->v >= t->v0)) {
      t->a = -(t->a);
      if (--(t->rep) == 0) {
        t->type = (t->type & 0xf0) | T_NORM;
        t->flag &= (~TF_CLIP);
        t->vx = cos_len(t->d, t->v);
        t->vy = sin_len(t->d, t->v);
      }
    }
    return;
  case T_GRAVITY:
    t->vy += t->a;
    t->tx += t->vx;
    t->ty += t->vy;
    return;
  case T_CHANGE:
    t->tx += t->vx;
    t->ty += t->vy;
    if (t->rep == t->count) {
      t->type = (t->type & 0xf0) | T_NORM;
      t->d = t->vd * ut_math_detail::DEG256_TO_RAD;
      t->vx = cos_len(t->d, t->v);
      t->vy = sin_len(t->d, t->v);
    }
    return;
  case T_SBHOMING:
    if ((t->count & 1) != 0) {
      w.effects.SpawnFragment(t->x, t->y, FRG_SMOKE);
    }
    t->tx += t->vx;
    t->ty += t->vy;
    if ((t->count < 130 - 60) && w.enemies.homing_flag != HOMING_DUMMY) {
      deg_t = wrap_pi(atan2_rad(w.enemies.homing_y - (t->y),
                                w.enemies.homing_x - (t->x)) -
                      t->d);
    } else if (t->count < 130 - 60) {
      deg_t = wrap_pi(atan2_rad((-20 * 64) - (t->y), 0) - t->d);
    } else {
      t->flag = TF_NONE;
      deg_t = 0;
    }
    // "On target" — the legacy deg256 math treated the aim as aligned once the
    // delta rounded to a 0 bucket (|delta| < 0.5 deg256). Keep that threshold
    // so the decelerate/steer branches still fire.
    if (std::abs(deg_t) < ut_math_detail::DEG256_TO_RAD / 2.0) {
      if (t->vd != 0)
        t->vd--;
      t->v += t->a;
    } else {
      t->vd++;
      t->v -= t->a;
    }
    t->d += (deg_t * (Cast::sign<uint8_t>(t->vd)) / 255.0);
    t->vx = cos_len(t->d, t->v);
    t->vy = sin_len(t->d, t->v);
    return;
  case T_SBHBOMB:
    if (t->count >= 49)
      t->flag = TF_DELETE;
    return;
  }
}

void BulletSubsystem::MoveByOption(Bullet *t) {
  int op_temp = 0;
  switch (t->option & 0xf0) {
  case TOP_NONE:
    t->x = t->tx;
    t->y = t->ty;
    return;
  case TOP_WAVE:
    op_temp = sinl(Cast::down_sign<uint8_t>(t->count << 2),
                   ((t->option & 0x0f) << 7));
    t->x = t->tx - sin_len(t->d, op_temp);
    t->y = t->ty + cos_len(t->d, op_temp);
    return;
  case TOP_ROLL: {
    const double angle =
        t->d + static_cast<double>(t->count << 1) * ut_math_detail::DEG256_TO_RAD;
    op_temp = (t->option & 0x0f) << 8;
    t->x = (t->tx + cos_len(angle, op_temp));
    t->y = (t->ty + sin_len(angle, op_temp));
  }
    return;
  case TOP_PURU:
    return;
  case TOP_REFX:
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      t->d = ut_math_detail::PI - t->d;
      t->vx = -(t->vx);
      t->x = t->tx + cos_len(t->d, t->v);
      t->y = t->ty + sin_len(t->d, t->v);
      op_temp = (t->option & 0x0f);
      t->option = (op_temp == 0) ? TOP_NONE : (TOP_REFX | (op_temp - 1));
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;
  case TOP_REFY:
    if ((t->ty) < GY_MIN) {
      t->d = -t->d;
      t->vy = -(t->vy);
      t->x = t->tx + cos_len(t->d, t->v);
      t->y = t->ty + sin_len(t->d, t->v);
      op_temp = (t->option & 0x0f);
      t->option = (op_temp == 0) ? TOP_NONE : (TOP_REFY | (op_temp - 1));
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;
  case TOP_REFXY:
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      t->d = ut_math_detail::PI - t->d;
      t->vx = -(t->vx);
      t->x = t->tx + cos_len(t->d, t->v);
      t->y = t->ty + sin_len(t->d, t->v);
      op_temp = (t->option & 0x0f);
      t->option = (op_temp == 0) ? TOP_NONE : (TOP_REFXY | (op_temp - 1));
    } else if ((t->ty) < GY_MIN) {
      t->d = -t->d;
      t->vy = -(t->vy);
      t->x = t->tx + cos_len(t->d, t->v);
      t->y = t->ty + sin_len(t->d, t->v);
      op_temp = (t->option & 0x0f);
      t->option = (op_temp == 0) ? TOP_NONE : (TOP_REFXY | (op_temp - 1));
    } else {
      t->x = t->tx;
      t->y = t->ty;
    }
    return;
  case TOP_DIV:
    t->x = t->tx;
    t->y = t->ty;
    if ((t->tx) < GX_MIN || (t->tx) > GX_MAX) {
      op_temp = 1;
      // Recursive self-spawn: build a new command from current state.
      BulletCommand cmd{};
      cmd.d = ut_math_detail::rad_to_deg256(ut_math_detail::PI - t->d);
      // (Spawn path clobbers command.x/y, so build directly)
      cmd.x = t->tx + cosl(cmd.d, t->v);
      cmd.y = t->ty + sinl(cmd.d, t->v);
      t->flag = TF_DELETE;
      cmd.ns = 2;
      cmd.c = (t->c) & 0x0f;
      cmd.cmd = (t->option & 0x0f) | TE_CIRCLE1;
      switch (cmd.cmd & 0x03) {
      case TC_WAY:
        cmd.n = 3;
        cmd.dw = 16;
        cmd.v = 13 - 2;
        break;
      case TC_ALL:
        cmd.n = 10;
        cmd.v = 13;
        cmd.d = Cast::down<uint8_t>(rnd());
        if ((cmd.cmd & TAMA_REN) != 0)
          cmd.v -= 2;
        break;
      case TC_RND:
        cmd.n = 4;
        cmd.dw = 128 - 32;
        cmd.v = 13 | TAMASP_RND2;
        break;
      }
      if ((cmd.cmd & TAMA_ZSET) != 0) {
        cmd.d = 0;
        cmd.dw -= 6;
      }
      cmd.type = T_NORM;
      cmd.option = TOP_NONE;
      Snd_SEPlay(12, cmd.x);
      Spawn(cmd);
    }
    return;
  case TOP_BOMB:
    return;
  }
}

void BulletSubsystem::MoveByEffect(Bullet *t) {
  switch (t->effect & 0xf0) {
  case TE_ROLL1:
  case TE_ROLL2:
  case TE_WARN:
  case TE_ROCK:
    return;
  case TE_CIRCLE1:
    t->x = (t->tx += (t->vx >> 1));
    t->y = (t->ty += (t->vy >> 1));
    if (t->count >= (5 * 4) - 1)
      t->effect = 0;
    return;
  case TE_CIRCLE2:
    return;
  case TE_DELETE:
    t->x += (t->vx >> 1);
    t->y += (t->vy >> 1);
    if (t->count >= 47)
      t->flag = TF_DELETE;
    return;
  }
}

// ============================================================
//  Read-only views (definitions are inline in the header)  // Section
//  intentionally blank — keep this anchor for grep.
// ============================================================

} // namespace bullets