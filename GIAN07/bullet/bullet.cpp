/// Bullet — Bullet class implementation

#include <span>
#include <utility>

#include "bullet.h"
#include "bullet_common.h"

#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/cast.h"
#include "util/ut_math.h"

// ── Free functions ───────────────────────────────────────────────

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

namespace {
constexpr auto RCSET(int x, int y, int w) -> PIXEL_LTRB {
  return {x, y, x + w, y + w};
}

constexpr auto kRapidFlag = 0x04;
constexpr auto kAimFlag = 0x08;

BulletMotion DecodeMotion(uint8_t value) {
  value &= 0x0f;
  if (value <= std::to_underlying(BulletMotion::Bomb)) {
    return static_cast<BulletMotion>(value);
  }
  return BulletMotion::Normal;
}

BulletOptionKind DecodeOption(uint8_t value) {
  value >>= 4;
  if (value <= std::to_underlying(BulletOptionKind::Bomb)) {
    return static_cast<BulletOptionKind>(value);
  }
  return BulletOptionKind::None;
}

BulletEffect DecodeEffect(uint8_t value) {
  switch (value >> 4) {
  case 1:
    return BulletEffect::Roll1;
  case 2:
    return BulletEffect::Roll2;
  case 3:
    return BulletEffect::Warning;
  case 4:
    return BulletEffect::Rock;
  case 5:
    return BulletEffect::Circle1;
  case 6:
    return BulletEffect::Circle2;
  case 15:
    return BulletEffect::Clearing;
  default:
    return BulletEffect::None;
  }
}
} // namespace

void Bullet::DrawEffect() const {
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
  const int ptn = (static_cast<int>(count_) / 4 % 5);
  int x = (x_ >> 6) - Width[ptn];
  int y = (y_ >> 6) - Width[ptn];
  PIXEL_LTRB temp;
  if (c_ >= 16 * 3) {
    temp = Target[3][ptn];
  } else {
    temp = Target[c_][ptn];
  }
  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, temp);
}

// ── MakeBulletSpawnInfo ──────────────────────────────────────────

BulletSpawnInfo MakeBulletSpawnInfo(const EclBulletState &cmd, int ox, int oy,
                                    bool scaling, const GameSession &game,
                                    BulletSpawnType spawn_type) {
  EclBulletState scaled = cmd;
  scaled.x += ox;
  scaled.y += oy;

  if (scaling) {
    switch (game.EffectiveLevel()) {
    case GameLevel::Easy:
      bullet_common::ApplyEasyCountSpread(
          bullet_common::DecodePattern(scaled.cmd), scaled.n, scaled.dw);
      bullet_common::ApplyEasyRapid(scaled.ns);
      break;
    case GameLevel::Hard:
    case GameLevel::Extra:
      bullet_common::ApplyHardCountSpread(
          bullet_common::DecodePattern(scaled.cmd), scaled.n, scaled.dw);
      bullet_common::ApplyHardRapid(scaled.ns);
      break;
    case GameLevel::Lunatic:
      bullet_common::ApplyLunaticCountSpread(
          bullet_common::DecodePattern(scaled.cmd), scaled.n, scaled.dw);
      bullet_common::ApplyLunaticRapid(scaled.ns);
      break;
    case GameLevel::Normal:
    default:
      break;
    }
  }

  int v = bullet_common::DecodeSpeed(scaled.v);
  if (scaling && DecodeMotion(scaled.type) == BulletMotion::Normal) {
    v = bullet_common::ScaleVelocityByRank(v, game.rank);
  }

  return {
      .x = scaled.x,
      .y = scaled.y,
      .speed = v,
      .acceleration = scaled.a,
      .angle = scaled.d,
      .spread = scaled.dw,
      .count = scaled.n,
      .rapid_count = scaled.ns,
      .visual = scaled.c,
      .speed_variance =
          static_cast<BulletSpeedVariance>((scaled.v & 0xc0) >> 6),
      .option = DecodeOption(scaled.option),
      .option_count = static_cast<uint8_t>(scaled.option & 0x0f),
      .motion = DecodeMotion(scaled.type),
      .repeat = scaled.rep,
      .angular_velocity = scaled.vd,
      .effect = DecodeEffect(scaled.cmd),
      .pattern = bullet_common::DecodePattern(scaled.cmd),
      .rapid = (scaled.cmd & kRapidFlag) != 0,
      .aimed = (scaled.cmd & kAimFlag) != 0,
      .spawn_type = spawn_type,
  };
}

// ── Bullet: Spawn ────────────────────────────────────────────────

void Bullet::Spawn(const BulletSpawnInfo &info) {
  x_ = tx_ = info.x;
  y_ = ty_ = info.y;
  v_ = v0_ = info.speed;
  a_ = info.acceleration;
  d_ = info.angle;
  d16_ = static_cast<uint16_t>(d_) << 8;
  vd_ = info.angular_velocity;
  c_ = info.visual;
  rep_ = info.repeat;
  motion_ = info.motion;
  option_ = info.option;
  option_count_ = info.option_count;
  effect_ = info.effect;
  vx_ = cosl(d_, v_);
  vy_ = sinl(d_, v_);
  count_ = 0;
  flag_ = TF_NONE;
}

// ── Bullet: Movement ─────────────────────────────────────────────

BulletUpdateInfo::UpdateResult Bullet::Update(const BulletUpdateInfo &info) {
  UpdateResult result;
  if (effect_ == BulletEffect::None) {
    MoveByType(info, result);
    MoveByOption(result);
    if ((flag_ & TF_CLIP) == 0 && (x_ < playfield::kWorldLeft - 4_px ||
                                   x_ > playfield::kWorldRight + 4_px ||
                                   y_ < playfield::kWorldTop - 4_px ||
                                   y_ > playfield::kWorldBottom + 4_px)) {
      flag_ |= TF_DELETE;
    }
  } else {
    MoveByEffect();
  }
  count_++;
  return result;
}

void Bullet::RevertToNormal() {
  motion_ = BulletMotion::Normal;
  flag_ &= ~TF_CLIP;
  vx_ = cosl(d_, v_);
  vy_ = sinl(d_, v_);
}

void Bullet::MoveByType(const BulletUpdateInfo &info,
                        BulletUpdateInfo::UpdateResult &result) {
  short deg_t = 0;
  switch (motion_) {
  case BulletMotion::Normal:
    tx_ += vx_;
    ty_ += vy_;
    return;
  case BulletMotion::Accelerating:
    v_ += a_;
    tx_ += cosl(d_, v_);
    ty_ += sinl(d_, v_);
    if (rep_ == static_cast<uint8_t>(count_)) {
      RevertToNormal();
    }
    return;
  case BulletMotion::Retargeting:
    v_ += a_;
    tx_ += cosl(d_, v_);
    ty_ += sinl(d_, v_);
    if (a_ > 0 && v_ >= v0_) {
      a_ = -a_;
      if (--rep_ == 0) {
        RevertToNormal();
      }
    }
    if (a_ < 0 && v_ <= 0) {
      a_ = -a_;
      d_ = atan8(info.player_x - x_, info.player_y - y_);
    }
    return;
  case BulletMotion::Homing:
    if (count_ > 19 && count_ % 2 == 0) {
      deg_t = atan8(info.player_x - x_, info.player_y - y_) - d_;
      if (deg_t < -128) {
        deg_t += 256;
      }
      if (deg_t > 128) {
        deg_t -= 256;
      }
      d_ = d_ + (deg_t * vd_ / 255);
    }
    v_ += a_;
    tx_ += cosl(d_, v_);
    ty_ += sinl(d_, v_);
    if (rep_ == static_cast<uint8_t>(count_)) {
      RevertToNormal();
    }
    return;
  case BulletMotion::Turning:
    d_ += Cast::sign<uint8_t>(vd_);
    tx_ += cosl(d_, v_);
    ty_ += sinl(d_, v_);
    if (rep_ == static_cast<uint8_t>(count_)) {
      RevertToNormal();
    }
    return;
  case BulletMotion::TurningAccelerating:
    v_ += a_;
    if (a_ > 0) {
      d_ += Cast::sign<uint8_t>(vd_);
    }
    tx_ += cosl(d_, v_);
    ty_ += sinl(d_, v_);
    if (a_ < 0 && v_ <= 0) {
      a_ = -a_;
    }
    if (a_ > 0 && v_ >= v0_) {
      a_ = -a_;
      if (--rep_ == 0) {
        RevertToNormal();
      }
    }
    return;
  case BulletMotion::TurningReversing:
    v_ += a_;
    d_ += Cast::sign<uint8_t>(vd_);
    tx_ += cosl(d_, v_);
    ty_ += sinl(d_, v_);
    if (a_ < 0 && v_ <= 0) {
      d_ += 128;
      a_ = -a_;
    }
    if (a_ > 0 && v_ >= v0_) {
      a_ = -a_;
      if (--rep_ == 0) {
        RevertToNormal();
      }
    }
    return;
  case BulletMotion::Gravity:
    vy_ += a_;
    tx_ += vx_;
    ty_ += vy_;
    return;
  case BulletMotion::ChangeDirection:
    tx_ += vx_;
    ty_ += vy_;
    if (rep_ == static_cast<uint8_t>(count_)) {
      d_ = Cast::sign<uint8_t>(vd_);
      RevertToNormal();
    }
    return;
  case BulletMotion::SpecialHoming:
    if ((count_ & 1) != 0) {
      result.smoke_spawn = true;
      result.smoke_x = x_;
      result.smoke_y = y_;
    }
    tx_ += vx_;
    ty_ += vy_;
    if (count_ < 130 - 60 && info.enemy_homing_valid) {
      deg_t = atan8(info.enemy_homing_x - x_, info.enemy_homing_y - y_) - d_;
    } else if (count_ < 130 - 60) {
      deg_t = atan8(0, -20_px - y_) - d_;
    } else {
      flag_ = TF_NONE;
      deg_t = 0;
    }
    if (deg_t < -128) {
      deg_t += 256;
    }
    if (deg_t > 128) {
      deg_t -= 256;
    }
    if (deg_t == 0) {
      if (vd_ != 0) {
        vd_--;
      }
      v_ += a_;
    } else {
      if (vd_ < INT8_MAX) {
        vd_++;
      }
      v_ -= a_;
    }
    d_ += (deg_t * Cast::sign<uint8_t>(vd_) / 255);
    vx_ = cosl(d_, v_);
    vy_ = sinl(d_, v_);
    return;
  case BulletMotion::Bomb:
    if (count_ >= 49) {
      flag_ |= TF_DELETE;
    }
    return;
  }
}

void Bullet::MoveByOption(BulletUpdateInfo::UpdateResult &result) {
  int op_temp = 0;
  switch (option_) {
  case BulletOptionKind::None:
    x_ = tx_;
    y_ = ty_;
    return;
  case BulletOptionKind::Wave:
    op_temp = sinl(Cast::down_sign<uint8_t>(static_cast<int>(count_ << 2)),
                   (option_count_ << 7));
    x_ = tx_ - sinl(d_, op_temp);
    y_ = ty_ + cosl(d_, op_temp);
    return;
  case BulletOptionKind::Orbit: {
    const auto angle =
        Cast::down_sign<uint8_t>(static_cast<int>(d_ + (count_ << 1)));
    op_temp = option_count_ << 8;
    x_ = tx_ + cosl(angle, op_temp);
    y_ = ty_ + sinl(angle, op_temp);
  }
    return;
  case BulletOptionKind::Stationary:
    return;
  case BulletOptionKind::ReflectX:
    if (tx_ < playfield::kWorldLeft || tx_ > playfield::kWorldRight) {
      d_ = 128 - d_;
      vx_ = -vx_;
      x_ = tx_ + cosl(d_, v_);
      y_ = ty_ + sinl(d_, v_);
      if (option_count_ == 0) {
        option_ = BulletOptionKind::None;
      } else {
        --option_count_;
      }
    } else {
      x_ = tx_;
      y_ = ty_;
    }
    return;
  case BulletOptionKind::ReflectY:
    if (ty_ < playfield::kWorldTop) {
      d_ = -d_;
      vy_ = -vy_;
      x_ = tx_ + cosl(d_, v_);
      y_ = ty_ + sinl(d_, v_);
      if (option_count_ == 0) {
        option_ = BulletOptionKind::None;
      } else {
        --option_count_;
      }
    } else {
      x_ = tx_;
      y_ = ty_;
    }
    return;
  case BulletOptionKind::ReflectXY:
    if (tx_ < playfield::kWorldLeft || tx_ > playfield::kWorldRight) {
      d_ = 128 - d_;
      vx_ = -vx_;
      x_ = tx_ + cosl(d_, v_);
      y_ = ty_ + sinl(d_, v_);
      if (option_count_ == 0) {
        option_ = BulletOptionKind::None;
      } else {
        --option_count_;
      }
    } else if (ty_ < playfield::kWorldTop) {
      d_ = -d_;
      vy_ = -vy_;
      x_ = tx_ + cosl(d_, v_);
      y_ = ty_ + sinl(d_, v_);
      if (option_count_ == 0) {
        option_ = BulletOptionKind::None;
      } else {
        --option_count_;
      }
    } else {
      x_ = tx_;
      y_ = ty_;
    }
    return;
  case BulletOptionKind::Divide: {
    x_ = tx_;
    y_ = ty_;
    uint8_t new_d = 0;
    if (tx_ < playfield::kWorldLeft || tx_ > playfield::kWorldRight) {
      op_temp = 1;
      new_d = 128 - d_;
    } else if (ty_ < playfield::kWorldTop) {
      op_temp = 1;
      new_d = -d_;
    }
    if (op_temp == 1) {
      flag_ |= TF_DELETE;
      const int cx = tx_ + cosl(new_d, v_);
      const int cy = ty_ + sinl(new_d, v_);
      constexpr uint8_t kCircleEffect = 0x50;
      const uint8_t ecmd = option_count_ | kCircleEffect;
      uint8_t n = 0, dw = 0;
      int sv = 0;
      uint8_t nd = new_d;
      switch (bullet_common::DecodePattern(ecmd)) {
      case BulletPattern::Spread:
        n = 3;
        dw = 16;
        sv = 13 - 2;
        break;
      case BulletPattern::Circle:
        n = 10;
        sv = 13;
        nd = Cast::down<uint8_t>(rnd());
        if ((ecmd & kRapidFlag) != 0) {
          sv -= 2;
        }
        break;
      case BulletPattern::Random:
        n = 4;
        dw = 128 - 32;
        sv = 13 | (std::to_underlying(BulletSpeedVariance::Medium) << 6);
        break;
      }
      if ((ecmd & kAimFlag) != 0) {
        nd = 0;
        dw -= 6;
      }
      result.division_requested = true;
      result.division_cx = cx;
      result.division_cy = cy;
      result.division_cmd = {cx,
                             cy,
                             nd,
                             dw,
                             n,
                             2,
                             static_cast<uint8_t>(sv),
                             static_cast<uint8_t>(c_ & 0x0f),
                             0,
                             0,
                             0,
                             ecmd,
                             std::to_underlying(BulletMotion::Normal),
                             0};
    }
    return;
  }
  case BulletOptionKind::Bomb:
    return;
  }
}

void Bullet::MoveByEffect() {
  switch (effect_) {
  case BulletEffect::None:
    return;
  case BulletEffect::Roll1:
  case BulletEffect::Roll2:
  case BulletEffect::Warning:
  case BulletEffect::Rock:
    return;
  case BulletEffect::Circle1:
    x_ = (tx_ += (vx_ >> 1));
    y_ = (ty_ += (vy_ >> 1));
    if (count_ >= 5 * 4 - 1) {
      effect_ = BulletEffect::None;
    }
    return;
  case BulletEffect::Circle2:
    return;
  case BulletEffect::Clearing:
    x_ += (vx_ >> 1);
    y_ += (vy_ >> 1);
    if (count_ >= 47) {
      flag_ |= TF_DELETE;
    }
    return;
  }
}

// ── Bullet: Render ───────────────────────────────────────────────

void Bullet::Render() const {
  static const PIXEL_LTRB rcExtraTama[4] = {{128, 384, 128 + 32, 384 + 32},
                                            {128 + 32, 384, 128 + 56, 384 + 24},
                                            {128 + 56, 384, 128 + 72, 384 + 16},
                                            {128 + 72, 384, 128 + 80, 384 + 8}};
  static constexpr uint8_t sizeExtraTama[4] = {16, 12, 8, 4};

  bool is_small = (c_ & 0xf0) == TAMA_SMALL;
  int x = (x_ >> 6) - (is_small ? 4 : 8);
  int y = (y_ >> 6) - (is_small ? 4 : 8);

  switch (effect_) {
  case BulletEffect::Clearing:
    if (is_small) {
      GrpSurface_Blit(
          {x, y}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{384 + ((static_cast<int>(count_) / 6) << 3), 120, 8, 8});
    } else {
      GrpSurface_Blit(
          {x, y}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{384 + ((static_cast<int>(count_) / 6) << 4), 104, 16, 16});
    }
    return;
  case BulletEffect::Circle1:
    DrawEffect();
    return;
  default:
    break;
  }

  if (is_small) {
    if (c_ != 0x25) {
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{(c_ << 3) + 384, 0, 8, 8});
    } else {
      GrpSurface_Blit(
          {x - 4, y - 4}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{((d_ + 8) & 0xf0) + 384, 24 + ((c_ & 0x0f) << 4), 16, 16});
    }
    return;
  }

  switch (c_ & 0xf0) {
  case TAMA_LARGE:
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM,
                    PIXEL_LTWH{((c_ & 0x0f) << 4) + 384, 8, 16, 16});
    break;
  case TAMA_EXTRA: {
    const uint8_t d = (c_ & 3);
    PIXEL_LTRB src = rcExtraTama[d];
    int ex = (x_ >> 6) - sizeExtraTama[d];
    int ey = (y_ >> 6) - sizeExtraTama[d];
    GrpSurface_Blit({ex, ey}, SURFACE_ID::ENEMY, src);
    break;
  }
  case TAMA_EXTRA2: {
    const auto d = Cast::down_sign<uint8_t>(d_ + 4) / 8;
    GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY,
                    PIXEL_LTWH{d * 16, 320 + ((c_ & 3) << 4), 16, 16});
    break;
  }
  case TAMA_ANGLE:
    if (c_ != 32 + 5) {
      GrpSurface_Blit(
          {x, y}, SURFACE_ID::SYSTEM,
          PIXEL_LTWH{((d_ + 8) & 0xf0) + 384, 24 + ((c_ & 0x0f) << 4), 16, 16});
    } else {
      const auto d = Cast::down_sign<uint8_t>(d_ + 4) / 8;
      int dx = (d % 8) * 32;
      int dy = (d / 8) * 32;
      GrpSurface_Blit({x - 8, y - 8}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{384 + dx, 304 + dy, 32, 32});
    }
    break;
  }
}

// ── Bullet: Lifecycle ────────────────────────────────────────────

bool Bullet::IsDead() const { return (flag_ & TF_DELETE) != 0; }

bool Bullet::IsSmall() const { return (c_ & 0xf0) == TAMA_SMALL; }

bool Bullet::IsClearing() const { return effect_ == BulletEffect::Clearing; }

bool Bullet::RegisterGraze() {
  if ((flag_ & TF_EVADE) != 0) {
    return false;
  }
  flag_ |= TF_EVADE;
  return true;
}

void Bullet::RemoveImmediately() { flag_ |= TF_DELETE; }

void Bullet::UpdateDisplayAngle() {
  const auto category = c_ & 0xf0;
  if (category == TAMA_ANGLE || category == TAMA_EXTRA2) {
    d_ += 4;
  }
}

void Bullet::Kill() {
  if (effect_ != BulletEffect::Clearing) {
    effect_ = BulletEffect::Clearing;
    count_ = 0;
    d_ = 0;
  }
}

HitResult Bullet::CheckHit(int player_x, int player_y,
                           int player_radius) const {
  if (effect_ == BulletEffect::Clearing || (flag_ & TF_DELETE) != 0) {
    return HitResult::Miss;
  }
  const int hit_radius = GetBulletHitRadius(c_);
  int dx = x_ - player_x;
  int dy = y_ - player_y;
  int combined = hit_radius + player_radius;
  if (dx * dx + dy * dy <= combined * combined) {
    return HitResult::Hit;
  }
  const int evade_radius = GetBulletEvadeRadius(c_);
  combined = evade_radius + player_radius;
  if (dx * dx + dy * dy <= combined * combined) {
    return HitResult::Graze;
  }
  return HitResult::Miss;
}

// ── Bullet: Debug ───────────────────────────────────────────────

void Bullet::RenderDebugHitbox(int mode) const {
  if (effect_ == BulletEffect::Clearing || (flag_ & TF_DELETE) != 0) {
    return;
  }
  auto *gp = GrpGeom_Poly();
  if (gp == nullptr) {
    return;
  }
  const int cx = x_ >> 6;
  const int cy = y_ >> 6;

  if (mode >= 2) {
    const int ev_r = GetBulletEvadeRadius(c_) >> 6;
    Geometry::CircleF_Approximated(*gp, {cx, cy}, ev_r, true);
  }

  const int r_px = GetBulletHitRadius(c_) >> 6;
  if (r_px > 0) {
    Geometry::CircleF_Approximated(*gp, {cx, cy}, r_px, true);
  }
}
