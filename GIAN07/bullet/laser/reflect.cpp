///
/// LaserReflect — Short / reflective laser member functions
///

#include <array>
#include <cmath>
#include <span>
#include <utility>

#include "long.h"
#include "reflect.h"

#include "bullet/bullet_common.h"
#include "gameplay/playfield.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

namespace {
inline constexpr auto kDebugLaserEvadeWidth = 12_px;
} // namespace

// Laser geometry:
//   3-----------------Length---> >----------2
//  Width                      < <             |
//   +(x,y)                     > >            +
//  Width                      < <             |
//   0-----------------Length---> >----------1

// ── Local constants ─────────────────────────────────────────────────
namespace {
inline constexpr auto kLaserEvadeWidth = 12_px;
} // namespace

// ── Geometry ───────────────────────────────────────────────────────

void LaserReflect::SetupGeometry() {
  auto *p = p_;
  p[1].x = p[0].x = (x_ / WORLD_COORD_SCALE) + wx_;
  p[1].y = p[0].y = (y_ / WORLD_COORD_SCALE) + wy_;
  p[2].x = p[3].x = (x_ / WORLD_COORD_SCALE) - wx_;
  p[2].y = p[3].y = (y_ / WORLD_COORD_SCALE) - wy_;
  p[1].x += lx_;
  p[1].y += ly_;
  p[2].x += lx_;
  p[2].y += ly_;
}

// ── Reflection check ────────────────────────────────────────────────

LaserReflect::UpdateResult
LaserReflect::CheckLongLaser(const LaserReflect &self, const LaserLong &ll,
                             float dx, float dy) {
  if (ll.state_ != LongState::Active) {
    return {};
  }

  const auto head = math::PolarVector(self.angle_, static_cast<float>(self.l_));
  const float lx = self.x_ + head.x;
  const float ly = self.y_ + head.y;

  const float tx = lx - static_cast<float>(ll.x_);
  const float ty = ly - static_cast<float>(ll.y_);
  const auto long_angle = ll.angle_;
  const float angle_cos = std::cos(long_angle);
  const float angle_sin = std::sin(long_angle);
  const float length = angle_cos * tx + angle_sin * ty;
  const float width = std::abs(-angle_sin * tx + angle_cos * ty);

  if (length <= 0 || width > ll.w_) {
    return {};
  }

  // Only reflect if the head is moving toward the beam centre.
  // If it is moving away, we are in the post-reflection tail and
  // the head merely hasn't cleared the beam's width yet.
  const float signed_width = -angle_sin * tx + angle_cos * ty;
  const float vel_norm = -angle_sin * dx + angle_cos * dy;

  if (signed_width * vel_norm > 0 || (vel_norm == 0 && width <= ll.w_)) {
    return {};
  }

  return UpdateResult{true, ReflectSpawnInfo{
                                .no_scaling = true,
                                .x = static_cast<int>(std::lround(lx)),
                                .y = static_cast<int>(std::lround(ly)),
                                .v = self.v_,
                                .w = static_cast<int>(std::lround(self.w_)),
                                .l = static_cast<int>(std::lround(self.lmax_)),
                                .angle = -self.angle_ + (long_angle * 2.0f),
                                .n = 1,
                                .c = self.c_,
                                .pattern = BulletPattern::Spread,
                                .type = ReflectLaserType::Reflect,
                            }};
}

// ── Spawn ────────────────────────────────────────────────────────────

void LaserReflect::Spawn(const ReflectSpawnInfo &info) {
  angle_ = bullet_common::CalcSpreadAngle(info.bullet_index, info.pattern,
                                          info.n, info.base_angle, info.dw);

  if (info.l2 != 0) {
    const auto offset = math::PolarVector(angle_, static_cast<float>(info.l2));
    x_ = static_cast<float>(info.x) + offset.x;
    y_ = static_cast<float>(info.y) + offset.y;
  } else {
    x_ = static_cast<float>(info.x);
    y_ = static_cast<float>(info.y);
  }

  v_ = info.v;
  const auto velocity = math::PolarVector(angle_, v_);
  vx_ = velocity.x;
  vy_ = velocity.y;

  w_ = info.w;
  lmax_ = info.l;

  lx_ = 0;
  ly_ = 0;
  const auto width =
      math::PolarVector(angle_, static_cast<float>(w_) / WORLD_COORD_SCALE);
  wx_ = -width.y;
  wy_ = width.x;

  l_ = 0;
  count_ = 0;
  grazed_ = false;

  c_ = info.c;
  subtype_ = info.type;

  if (subtype_ == ReflectLaserType::Reflect) {
    state_ = ReflectState::Shooting;
  } else {
    state_ = ReflectState::Growing;
  }

  SetupGeometry();
}

// ── State machine ───────────────────────────────────────────────────

auto LaserReflect::Update(const UpdateInfo &info) -> UpdateResult {
  ++count_;

  UpdateResult result;

  switch (state_) {
  case ReflectState::Growing:
    UpdateGrowing();
    break;
  case ReflectState::Flying:
    result = UpdateFlying(info.longs);
    break;
  case ReflectState::Shooting:
    result = UpdateShooting(info.longs);
    break;
  case ReflectState::Reflected:
    UpdateReflected();
    break;
  case ReflectState::Clearing:
    UpdateClearing();
    break;
  case ReflectState::Dead:
    break;
  }

  if (x_ < playfield::kWorldLeft || x_ > playfield::kWorldRight ||
      y_ < playfield::kWorldTop || y_ > playfield::kWorldBottom) {
    MarkDead();
  }

  return result;
}

void LaserReflect::UpdateGrowing() {
  if (l_ < lmax_) {
    l_ += v_;
    const auto length =
        math::PolarVector(angle_, static_cast<float>(l_) / WORLD_COORD_SCALE);
    lx_ = length.x;
    ly_ = length.y;
    auto *p = p_;
    p[1].x = p[0].x + lx_;
    p[1].y = p[0].y + ly_;
    p[2].x = p[3].x + lx_;
    p[2].y = p[3].y + ly_;
  } else {
    x_ += vx_;
    y_ += vy_;
    SetupGeometry();
    state_ = ReflectState::Flying;
  }
}

auto LaserReflect::UpdateFlying(std::span<const LaserLong *> longs)
    -> UpdateResult {
  x_ += vx_;
  y_ += vy_;
  SetupGeometry();

  for (const auto *ll : longs) {
    if (auto hit = CheckLongLaser(*this, *ll, vx_, vy_); hit.spawn_requested) {
      state_ = ReflectState::Reflected;
      return hit;
    }
  }

  return {};
}

auto LaserReflect::UpdateShooting(std::span<const LaserLong *> longs)
    -> UpdateResult {
  l_ += v_;
  const auto length =
      math::PolarVector(angle_, static_cast<float>(l_) / WORLD_COORD_SCALE);
  lx_ = length.x;
  ly_ = length.y;
  auto *p = p_;
  p[1].x = p[0].x + lx_;
  p[1].y = p[0].y + ly_;
  p[2].x = p[3].x + lx_;
  p[2].y = p[3].y + ly_;

  if (l_ >= lmax_) {
    state_ = ReflectState::Flying;
    return {};
  }

  const auto velocity = math::PolarVector(angle_, v_);

  for (const auto *ll : longs) {
    if (auto hit = CheckLongLaser(*this, *ll, velocity.x, velocity.y);
        hit.spawn_requested) {
      state_ = ReflectState::Reflected;
      return hit;
    }
  }

  return {};
}

void LaserReflect::UpdateReflected() {
  if (l_ <= v_) {
    state_ = ReflectState::Dead;
    return;
  }
  l_ -= v_;
  x_ += vx_;
  y_ += vy_;
  const auto length =
      math::PolarVector(angle_, static_cast<float>(l_) / WORLD_COORD_SCALE);
  lx_ = length.x;
  ly_ = length.y;
  auto *p = p_;
  p[0].x = p[1].x - lx_;
  p[0].y = p[1].y - ly_;
  p[3].x = p[2].x - lx_;
  p[3].y = p[2].y - ly_;
}

void LaserReflect::UpdateClearing() {
  if (l_ < lmax_) {
    l_ += v_;
    w_ += 16;
    const auto length =
        math::PolarVector(angle_, static_cast<float>(l_) / WORLD_COORD_SCALE);
    lx_ = length.x;
    ly_ = length.y;
    auto *p = p_;
    p[1].x = p[0].x + lx_;
    p[1].y = p[0].y + ly_;
    p[2].x = p[3].x + lx_;
    p[2].y = p[3].y + ly_;
  } else {
    w_ += 64;
  }

  const auto width =
      math::PolarVector(angle_, static_cast<float>(w_) / WORLD_COORD_SCALE);
  wx_ = -width.y;
  wy_ = width.x;
  SetupGeometry();

  if (count_ > 30) {
    state_ = ReflectState::Dead;
  }
}

// ── Hit detection ───────────────────────────────────────────────────

HitResult LaserReflect::CheckHit(int px, int py, int player_radius) const {
  if (state_ == ReflectState::Dead || state_ == ReflectState::Clearing) {
    return HitResult::Miss;
  }

  const float tx = static_cast<float>(px) - x_;
  const float ty = static_cast<float>(py) - y_;
  const float angle_cos = std::cos(angle_);
  const float angle_sin = std::sin(angle_);
  const float len = angle_cos * tx + angle_sin * ty;
  const float dist = std::abs(-angle_sin * tx + angle_cos * ty);

  if (len <= 0 || len > l_)
    return HitResult::Miss;
  if (dist <= w_ + player_radius)
    return HitResult::Hit;
  if (dist <= w_ + kLaserEvadeWidth)
    return HitResult::Graze;
  return HitResult::Miss;
}

// ── Virtual overrides ───────────────────────────────────────────────

void LaserReflect::Render() const {
  if (state_ == ReflectState::Clearing) {
    DrawClearing();
    return;
  }

  DrawOuter();
}

void LaserReflect::DrawClearing() const {
  constexpr RGB216 col = {1, 0, 5};
  GrpGeom->SetColor(col);
  GrpGeom->DrawLine(p_[0].x, p_[0].y, p_[1].x, p_[1].y);
  GrpGeom->DrawLine(p_[3].x, p_[3].y, p_[2].x, p_[2].y);
}

void LaserReflect::DrawOuter() const {
  if (auto *gp = GrpGeom_Poly()) {
    GeomGrdRect(*gp, p_, RGB216{1, 0, 5}.ToRGB());
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({1, 0, 5});
    gf->DrawTriangleFan(p_);

    gf->SetColor({5, 5, 5});

    VERTEX_XY inner[4];
    inner[0].x = inner[1].x = p_[0].x - (wx_ * 3 / 4);
    inner[0].y = inner[1].y = p_[0].y - (wy_ * 3 / 4);
    inner[3].x = inner[2].x = p_[3].x + (wx_ * 3 / 4);
    inner[3].y = inner[2].y = p_[3].y + (wy_ * 3 / 4);
    inner[1].x += lx_;
    inner[1].y += ly_;
    inner[2].x += lx_;
    inner[2].y += ly_;
    gf->DrawTriangleFan(inner);
  }
}

bool LaserReflect::IsDead() const { return state_ == ReflectState::Dead; }

int LaserReflect::X() const { return static_cast<int>(std::lround(x_)); }

int LaserReflect::Y() const { return static_cast<int>(std::lround(y_)); }

bool LaserReflect::RegisterGraze() { return !std::exchange(grazed_, true); }

void LaserReflect::Kill() {
  if (state_ != ReflectState::Clearing && state_ != ReflectState::Dead) {
    state_ = ReflectState::Clearing;
    count_ = 0;
  }
}

// ── Debug ─────────────────────────────────────────────────────────

void LaserReflect::RenderDebugHitbox(int mode) const {
  if (state_ == ReflectState::Dead || state_ == ReflectState::Clearing) {
    return;
  }
  auto *gp = GrpGeom_Poly();
  if (gp == nullptr) {
    return;
  }
  const std::array<VERTEX_XY, 4> strip = {p_[0], p_[3], p_[1], p_[2]};
  gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, strip);

  if (mode >= 2 && w_ > 0) {
    const float bx = x_ / WORLD_COORD_SCALE;
    const float by = y_ / WORLD_COORD_SCALE;
    const float scale = w_ + kDebugLaserEvadeWidth;
    const float wx2 = wx_ * scale / w_;
    const float wy2 = wy_ * scale / w_;
    VERTEX_XY ep[4];
    ep[1].x = ep[0].x = static_cast<float>(bx + wx2);
    ep[1].y = ep[0].y = static_cast<float>(by + wy2);
    ep[2].x = ep[3].x = static_cast<float>(bx - wx2);
    ep[2].y = ep[3].y = static_cast<float>(by - wy2);
    ep[1].x += static_cast<float>(lx_);
    ep[1].y += static_cast<float>(ly_);
    ep[2].x += static_cast<float>(lx_);
    ep[2].y += static_cast<float>(ly_);
    const std::array<VERTEX_XY, 4> estrip = {ep[0], ep[3], ep[1], ep[2]};
    gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, estrip);
  }
}
