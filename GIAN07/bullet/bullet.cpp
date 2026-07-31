/// Enemy bullet entity behavior.

#include <cmath>
#include <span>
#include <utility>

#include "bullet.h"
#include "bullet_common.h"

#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/cast.h"
#include "util/math_utils.h"

int GetBulletHitRadius(uint8_t c) {
  switch (c & kBulletVisualCategoryMask) {
  case kSmallBulletVisual:
    return kSmallBulletHitRadius;
  case kLargeBulletVisual:
  case kLargeExtraBulletVisual:
    return kMediumBulletHitRadius;
  case kDirectionalBulletVisual:
    return c == kSpecialDirectionalBulletVisual ? kMediumBulletHitRadius
                                                : kSmallBulletHitRadius;
  case kExtraBulletVisual: {
    constexpr int radii[4] = {kExtraLargeBulletHitRadius, kLargeBulletHitRadius,
                              kMediumBulletHitRadius, kSmallBulletHitRadius};
    return radii[c & 3];
  }
  default:
    return kMediumBulletHitRadius;
  }
}

int GetBulletEvadeRadius(uint8_t c) {
  return (c & kBulletVisualCategoryMask) == kSmallBulletVisual
             ? kSmallBulletGrazeRadius
             : kLargeBulletGrazeRadius;
}

namespace {
constexpr auto RCSET(int x, int y, int w) -> PIXEL_LTRB {
  return {x, y, x + w, y + w};
}

constexpr auto kRapidFlag = 0x04;
constexpr auto kAimFlag = 0x08;

int WorldToPixel(float value) {
  return static_cast<int>(std::floor(value / WORLD_COORD_SCALE));
}

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
  int x = WorldToPixel(x_) - Width[ptn];
  int y = WorldToPixel(y_) - Width[ptn];
  PIXEL_LTRB temp;
  if (c_ >= 16 * 3) {
    temp = Target[3][ptn];
  } else {
    temp = Target[c_][ptn];
  }
  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, temp);
}

// ── Bullet spawn setup ───────────────────────────────────────────

void ScaleBulletSpawnInfo(BulletSpawnInfo &info, const GameSession &game) {
  switch (game.EffectiveLevel()) {
  case GameLevel::Easy:
    bullet_common::ApplyEasyCountSpread(info.pattern, info.count, info.spread);
    bullet_common::ApplyEasyRapid(info.rapid_count);
    break;
  case GameLevel::Hard:
  case GameLevel::Extra:
    bullet_common::ApplyHardCountSpread(info.pattern, info.count, info.spread);
    bullet_common::ApplyHardRapid(info.rapid_count);
    break;
  case GameLevel::Lunatic:
    bullet_common::ApplyLunaticCountSpread(info.pattern, info.count,
                                           info.spread);
    bullet_common::ApplyLunaticRapid(info.rapid_count);
    break;
  case GameLevel::Normal:
  default:
    break;
  }

  if (info.motion == BulletMotion::Normal) {
    info.speed = bullet_common::ScaleVelocityByRank(info.speed, game.rank);
  }
}

BulletSpawnInfo MakeBulletSpawnInfo(const EclBulletState &cmd, int ox, int oy,
                                    bool scaling, const GameSession &game,
                                    BulletSpawnType spawn_type) {
  BulletSpawnInfo info{
      .x = cmd.x + ox,
      .y = cmd.y + oy,
      .speed = static_cast<float>(bullet_common::DecodeSpeed(cmd.v)),
      .acceleration = static_cast<float>(cmd.a),
      .angle = math::AngleFromLegacy(cmd.d),
      .spread = cmd.dw,
      .count = cmd.n,
      .rapid_count = cmd.ns,
      .visual = cmd.c,
      .speed_variance = static_cast<BulletSpeedVariance>((cmd.v & 0xc0) >> 6),
      .option = DecodeOption(cmd.option),
      .option_count = static_cast<uint8_t>(cmd.option & 0x0f),
      .motion = DecodeMotion(cmd.type),
      .repeat = cmd.rep,
      .angular_velocity = cmd.vd,
      .effect = DecodeEffect(cmd.cmd),
      .pattern = bullet_common::DecodePattern(cmd.cmd),
      .rapid = (cmd.cmd & kRapidFlag) != 0,
      .aimed = (cmd.cmd & kAimFlag) != 0,
      .spawn_type = spawn_type,
  };
  if (scaling) {
    ScaleBulletSpawnInfo(info, game);
  }
  return info;
}

// ── Bullet: Spawn ────────────────────────────────────────────────

void Bullet::Spawn(const BulletSpawnInfo &info) {
  x_ = tx_ = info.x;
  y_ = ty_ = info.y;
  v_ = v0_ = info.speed;
  a_ = info.acceleration;
  angle_ = info.angle;
  vd_ = info.angular_velocity;
  c_ = info.visual;
  rep_ = info.repeat;
  motion_ = info.motion;
  option_ = info.option;
  option_count_ = info.option_count;
  effect_ = info.effect;
  const auto velocity = math::PolarVector(angle_, v_);
  vx_ = velocity.x;
  vy_ = velocity.y;
  count_ = 0;
  flags_ = Flags::None;
}

// ── Bullet: Movement ─────────────────────────────────────────────

BulletUpdateInfo::UpdateResult Bullet::Update(const BulletUpdateInfo &info) {
  UpdateResult result;
  if (effect_ == BulletEffect::None) {
    MoveByType(info, result);
    MoveByOption(result);
    if (!HasFlag(Flags::KeepOutsidePlayfield) &&
        (x_ < playfield::kWorldLeft - 4_px ||
         x_ > playfield::kWorldRight + 4_px ||
         y_ < playfield::kWorldTop - 4_px ||
         y_ > playfield::kWorldBottom + 4_px)) {
      SetFlag(Flags::PendingRemoval, true);
    }
  } else {
    MoveByEffect();
  }
  count_++;
  return result;
}

void Bullet::RevertToNormal() {
  motion_ = BulletMotion::Normal;
  SetFlag(Flags::KeepOutsidePlayfield, false);
  const auto velocity = math::PolarVector(angle_, v_);
  vx_ = velocity.x;
  vy_ = velocity.y;
}

void Bullet::MoveByType(const BulletUpdateInfo &info,
                        BulletUpdateInfo::UpdateResult &result) {
  float angle_delta = 0.0f;
  switch (motion_) {
  case BulletMotion::Normal:
    tx_ += vx_;
    ty_ += vy_;
    return;
  case BulletMotion::Accelerating:
    v_ += a_;
    {
      const auto velocity = math::PolarVector(angle_, v_);
      tx_ += velocity.x;
      ty_ += velocity.y;
    }
    if (rep_ == static_cast<uint8_t>(count_)) {
      RevertToNormal();
    }
    return;
  case BulletMotion::Retargeting:
    v_ += a_;
    {
      const auto velocity = math::PolarVector(angle_, v_);
      tx_ += velocity.x;
      ty_ += velocity.y;
    }
    if (a_ > 0 && v_ >= v0_) {
      a_ = -a_;
      if (--rep_ == 0) {
        RevertToNormal();
      }
    }
    if (a_ < 0 && v_ <= 0) {
      a_ = -a_;
      angle_ = math::AngleTo(static_cast<float>(info.player_x) - x_,
                             static_cast<float>(info.player_y) - y_);
    }
    return;
  case BulletMotion::Homing:
    if (count_ > 19 && count_ % 2 == 0) {
      const auto target = math::AngleTo(static_cast<float>(info.player_x) - x_,
                                        static_cast<float>(info.player_y) - y_);
      angle_delta = math::ShortestAngleDelta(target, angle_);
      angle_ += angle_delta * static_cast<float>(vd_) / 255.0f;
    }
    v_ += a_;
    {
      const auto velocity = math::PolarVector(angle_, v_);
      tx_ += velocity.x;
      ty_ += velocity.y;
    }
    if (rep_ == static_cast<uint8_t>(count_)) {
      RevertToNormal();
    }
    return;
  case BulletMotion::Turning:
    angle_ += static_cast<float>(vd_) * math::kLegacyAngleStep;
    {
      const auto velocity = math::PolarVector(angle_, v_);
      tx_ += velocity.x;
      ty_ += velocity.y;
    }
    if (rep_ == static_cast<uint8_t>(count_)) {
      RevertToNormal();
    }
    return;
  case BulletMotion::TurningAccelerating:
    v_ += a_;
    if (a_ > 0) {
      angle_ += static_cast<float>(vd_) * math::kLegacyAngleStep;
    }
    {
      const auto velocity = math::PolarVector(angle_, v_);
      tx_ += velocity.x;
      ty_ += velocity.y;
    }
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
    angle_ += static_cast<float>(vd_) * math::kLegacyAngleStep;
    {
      const auto velocity = math::PolarVector(angle_, v_);
      tx_ += velocity.x;
      ty_ += velocity.y;
    }
    if (a_ < 0 && v_ <= 0) {
      angle_ += math::kFullAngle / 2.0f;
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
      angle_ = math::AngleFromLegacy(Cast::sign<uint8_t>(vd_));
      RevertToNormal();
    }
    return;
  case BulletMotion::SpecialHoming:
    if ((count_ & 1) != 0) {
      result.smoke_spawn = true;
      result.smoke_x = X();
      result.smoke_y = Y();
    }
    tx_ += vx_;
    ty_ += vy_;
    if (count_ < 130 - 60 && info.enemy_homing_valid) {
      const auto target =
          math::AngleTo(static_cast<float>(info.enemy_homing_x) - x_,
                        static_cast<float>(info.enemy_homing_y) - y_);
      angle_delta = math::ShortestAngleDelta(target, angle_);
    } else if (count_ < 130 - 60) {
      const auto target = math::AngleTo(0.0f, static_cast<float>(-20_px) - y_);
      angle_delta = math::ShortestAngleDelta(target, angle_);
    } else {
      flags_ = Flags::None;
      angle_delta = 0.0f;
    }
    if (std::abs(angle_delta) < math::kLegacyAngleStep * 0.5f) {
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
    angle_ +=
        angle_delta * static_cast<float>(Cast::sign<uint8_t>(vd_)) / 255.0f;
    {
      const auto velocity = math::PolarVector(angle_, v_);
      vx_ = velocity.x;
      vy_ = velocity.y;
    }
    return;
  case BulletMotion::Bomb:
    if (count_ >= 49) {
      SetFlag(Flags::PendingRemoval, true);
    }
    return;
  }
}

void Bullet::MoveByOption(BulletUpdateInfo::UpdateResult &result) {
  float op_temp = 0.0f;
  switch (option_) {
  case BulletOptionKind::None:
    x_ = tx_;
    y_ = ty_;
    return;
  case BulletOptionKind::Wave:
    op_temp =
        std::sin(static_cast<float>(count_ << 2) * math::kLegacyAngleStep) *
        static_cast<float>(option_count_ << 7);
    {
      const auto offset = math::PolarVector(angle_, op_temp);
      x_ = tx_ - offset.y;
      y_ = ty_ + offset.x;
    }
    return;
  case BulletOptionKind::Orbit: {
    const auto angle =
        angle_ + static_cast<float>(count_ << 1) * math::kLegacyAngleStep;
    op_temp = option_count_ << 8;
    const auto offset = math::PolarVector(angle, op_temp);
    x_ = tx_ + offset.x;
    y_ = ty_ + offset.y;
  }
    return;
  case BulletOptionKind::Stationary:
    return;
  case BulletOptionKind::ReflectX:
    if (tx_ < playfield::kWorldLeft || tx_ > playfield::kWorldRight) {
      angle_ = (math::kFullAngle / 2.0f) - angle_;
      vx_ = -vx_;
      const auto velocity = math::PolarVector(angle_, v_);
      x_ = tx_ + velocity.x;
      y_ = ty_ + velocity.y;
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
      angle_ = -angle_;
      vy_ = -vy_;
      const auto velocity = math::PolarVector(angle_, v_);
      x_ = tx_ + velocity.x;
      y_ = ty_ + velocity.y;
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
      angle_ = (math::kFullAngle / 2.0f) - angle_;
      vx_ = -vx_;
      const auto velocity = math::PolarVector(angle_, v_);
      x_ = tx_ + velocity.x;
      y_ = ty_ + velocity.y;
      if (option_count_ == 0) {
        option_ = BulletOptionKind::None;
      } else {
        --option_count_;
      }
    } else if (ty_ < playfield::kWorldTop) {
      angle_ = -angle_;
      vy_ = -vy_;
      const auto velocity = math::PolarVector(angle_, v_);
      x_ = tx_ + velocity.x;
      y_ = ty_ + velocity.y;
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
    float new_angle = 0.0f;
    if (tx_ < playfield::kWorldLeft || tx_ > playfield::kWorldRight) {
      op_temp = 1.0f;
      new_angle = (math::kFullAngle / 2.0f) - angle_;
    } else if (ty_ < playfield::kWorldTop) {
      op_temp = 1.0f;
      new_angle = -angle_;
    }
    if (op_temp == 1.0f) {
      SetFlag(Flags::PendingRemoval, true);
      const auto velocity = math::PolarVector(new_angle, v_);
      const int cx = static_cast<int>(std::lround(tx_ + velocity.x));
      const int cy = static_cast<int>(std::lround(ty_ + velocity.y));
      constexpr uint8_t kCircleEffect = 0x50;
      const uint8_t ecmd = option_count_ | kCircleEffect;
      uint8_t n = 0, dw = 0;
      int sv = 0;
      switch (bullet_common::DecodePattern(ecmd)) {
      case BulletPattern::Spread:
        n = 3;
        dw = 16;
        sv = 13 - 2;
        break;
      case BulletPattern::Circle:
        n = 10;
        sv = 13;
        new_angle =
            math::AngleFromLegacy(static_cast<uint8_t>(math::RandomInt()));
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
        new_angle = 0.0f;
        dw -= 6;
      }
      result.division_requested = true;
      result.division_cx = cx;
      result.division_cy = cy;
      result.division_info = {
          .x = cx,
          .y = cy,
          .speed = static_cast<float>(bullet_common::DecodeSpeed(sv)),
          .angle = new_angle,
          .spread = dw,
          .count = n,
          .rapid_count = 2,
          .visual = static_cast<uint8_t>(c_ & 0x0f),
          .speed_variance = static_cast<BulletSpeedVariance>((sv & 0xc0) >> 6),
          .motion = BulletMotion::Normal,
          .effect = DecodeEffect(ecmd),
          .pattern = bullet_common::DecodePattern(ecmd),
          .rapid = (ecmd & kRapidFlag) != 0,
          .aimed = (ecmd & kAimFlag) != 0,
      };
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
    x_ = (tx_ += vx_ * 0.5f);
    y_ = (ty_ += vy_ * 0.5f);
    if (count_ >= 5 * 4 - 1) {
      effect_ = BulletEffect::None;
    }
    return;
  case BulletEffect::Circle2:
    return;
  case BulletEffect::Clearing:
    x_ += vx_ * 0.5f;
    y_ += vy_ * 0.5f;
    if (count_ >= 47) {
      SetFlag(Flags::PendingRemoval, true);
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

  const bool is_small = (c_ & kBulletVisualCategoryMask) == kSmallBulletVisual;
  int x = WorldToPixel(x_) - (is_small ? 4 : 8);
  int y = WorldToPixel(y_) - (is_small ? 4 : 8);
  const auto display_angle = DisplayAngle();

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
      GrpSurface_Blit({x - 4, y - 4}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{((display_angle + 8) & 0xf0) + 384,
                                 24 + ((c_ & 0x0f) << 4), 16, 16});
    }
    return;
  }

  switch (c_ & 0xf0) {
  case kLargeBulletVisual:
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM,
                    PIXEL_LTWH{((c_ & 0x0f) << 4) + 384, 8, 16, 16});
    break;
  case kExtraBulletVisual: {
    const uint8_t d = (c_ & 3);
    PIXEL_LTRB src = rcExtraTama[d];
    int ex = WorldToPixel(x_) - sizeExtraTama[d];
    int ey = WorldToPixel(y_) - sizeExtraTama[d];
    GrpSurface_Blit({ex, ey}, SURFACE_ID::ENEMY, src);
    break;
  }
  case kLargeExtraBulletVisual: {
    const auto d = Cast::down_sign<uint8_t>(display_angle + 4) / 8;
    GrpSurface_Blit({x, y}, SURFACE_ID::ENEMY,
                    PIXEL_LTWH{d * 16, 320 + ((c_ & 3) << 4), 16, 16});
    break;
  }
  case kDirectionalBulletVisual:
    if (c_ != 32 + 5) {
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{((display_angle + 8) & 0xf0) + 384,
                                 24 + ((c_ & 0x0f) << 4), 16, 16});
    } else {
      const auto d = Cast::down_sign<uint8_t>(display_angle + 4) / 8;
      int dx = (d % 8) * 32;
      int dy = (d / 8) * 32;
      GrpSurface_Blit({x - 8, y - 8}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{384 + dx, 304 + dy, 32, 32});
    }
    break;
  }
}

// ── Bullet: Lifecycle ────────────────────────────────────────────

bool Bullet::IsDead() const { return HasFlag(Flags::PendingRemoval); }

int Bullet::X() const { return static_cast<int>(std::lround(x_)); }

int Bullet::Y() const { return static_cast<int>(std::lround(y_)); }

uint8_t Bullet::DisplayAngle() const { return math::AngleToLegacy(angle_); }

bool Bullet::IsSmall() const {
  return (c_ & kBulletVisualCategoryMask) == kSmallBulletVisual;
}

bool Bullet::IsClearing() const { return effect_ == BulletEffect::Clearing; }

bool Bullet::RegisterGraze() {
  if (HasFlag(Flags::Grazed)) {
    return false;
  }
  SetFlag(Flags::Grazed, true);
  return true;
}

void Bullet::RemoveImmediately() { SetFlag(Flags::PendingRemoval, true); }

void Bullet::UpdateDisplayAngle() {
  const auto category = c_ & kBulletVisualCategoryMask;
  if (category == kDirectionalBulletVisual ||
      category == kLargeExtraBulletVisual) {
    angle_ += 4.0f * math::kLegacyAngleStep;
  }
}

void Bullet::Kill() {
  if (effect_ != BulletEffect::Clearing) {
    effect_ = BulletEffect::Clearing;
    count_ = 0;
    angle_ = 0.0f;
  }
}

HitResult Bullet::CheckHit(int player_x, int player_y,
                           int player_radius) const {
  if (effect_ == BulletEffect::Clearing || HasFlag(Flags::PendingRemoval)) {
    return HitResult::Miss;
  }
  const int hit_radius = GetBulletHitRadius(c_);
  const float dx = x_ - static_cast<float>(player_x);
  const float dy = y_ - static_cast<float>(player_y);
  float combined = static_cast<float>(hit_radius + player_radius);
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
  if (effect_ == BulletEffect::Clearing || HasFlag(Flags::PendingRemoval)) {
    return;
  }
  const int cx = WorldToPixel(x_);
  const int cy = WorldToPixel(y_);

  if (mode >= 2) {
    const int ev_r = GetBulletEvadeRadius(c_) >> 6;
    geometry::DrawFilledCircle(*GrpGeom, {cx, cy}, ev_r, true);
  }

  const int r_px = GetBulletHitRadius(c_) >> 6;
  if (r_px > 0) {
    geometry::DrawFilledCircle(*GrpGeom, {cx, cy}, r_px, true);
  }
}
