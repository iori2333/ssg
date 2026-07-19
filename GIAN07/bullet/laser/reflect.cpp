///
/// LaserReflect — Short / reflective laser member functions
///

#include <algorithm>

#include "reflect.h"
#include "long.h"

#include "bullet/bullet_common.h"
#include "core/gian.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

namespace {
inline constexpr auto kDebugLaserEvadeWidth = 12 * 64;
} // namespace

// Laser geometry:
//   3-----------------Length---> >----------2
//  Width                      < <             |
//   +(x,y)                     > >            +
//  Width                      < <             |
//   0-----------------Length---> >----------1

// ── Local constants ─────────────────────────────────────────────────
namespace {
inline constexpr auto kLaserEvadeWidth = 12 * 64;
} // namespace

// ── Geometry ───────────────────────────────────────────────────────

void LaserReflect::SetupGeometry() {
  auto *p = p_;
  p[1].x = p[0].x = (x_ >> 6) + wx_;
  p[1].y = p[0].y = (y_ >> 6) + wy_;
  p[2].x = p[3].x = (x_ >> 6) - wx_;
  p[2].y = p[3].y = (y_ >> 6) - wy_;
  p[1].x += lx_;
  p[1].y += ly_;
  p[2].x += lx_;
  p[2].y += ly_;
}

// ── Reflection check ────────────────────────────────────────────────

LaserReflect::UpdateResult
LaserReflect::CheckLongLaser(const LaserReflect &self, const LaserLong &ll,
                             int dx, int dy) {
  if (ll.state_ != LongState::Active) {
    return {};
  }

  const long lx = self.x_ + cosl(self.d_, self.l_);
  const long ly = self.y_ + sinl(self.d_, self.l_);

  const long tx = lx - ll.x_;
  const long ty = ly - ll.y_;
  const long length = cosl(ll.d_, tx) + sinl(ll.d_, ty);
  const long width = std::abs(-sinl(ll.d_, tx) + cosl(ll.d_, ty));

  if (length <= 0 || width > ll.w_) {
    return {};
  }

  // Only reflect if the head is moving toward the beam centre.
  // If it is moving away, we are in the post-reflection tail and
  // the head merely hasn't cleared the beam's width yet.
  const long signed_width = -sinl(ll.d_, tx) + cosl(ll.d_, ty);
  const long vel_norm = -sinl(ll.d_, dx) + cosl(ll.d_, dy);

  if (signed_width * vel_norm > 0 || (vel_norm == 0 && width <= ll.w_)) {
    return {};
  }

  return UpdateResult{true, ReflectSpawnInfo{
      .no_scaling = true,
      .x = static_cast<int>(lx),
      .y = static_cast<int>(ly),
      .v = self.v_,
      .w = self.w_,
      .l = self.lmax_,
        .d = static_cast<uint8_t>(-static_cast<int>(self.d_) +
                                  (static_cast<int>(ll.d_) << 1)),
      .n = 1,
      .c = self.c_,
      .cmd = bullet_common::kCmdWay,
      .cmd_type = static_cast<uint8_t>(ReflectLaserType::Reflect),
  }};
}

// ── Spawn ────────────────────────────────────────────────────────────

void LaserReflect::Spawn(const ReflectSpawnInfo &info) {
  d_ = bullet_common::CalcSpreadDir(info.bullet_index,
                                     info.cmd & bullet_common::kCmdMask,
                                     info.n, info.base_deg, info.dw);

  if (info.l2 != 0) {
    x_ = info.x + cosl(d_, info.l2);
    y_ = info.y + sinl(d_, info.l2);
  } else {
    x_ = info.x;
    y_ = info.y;
  }

  v_ = info.v;
  a_ = info.a;

  vx_ = cosl(d_, v_);
  vy_ = sinl(d_, v_);

  w_ = info.w;
  lmax_ = info.l;

  lx_ = 0;
  ly_ = 0;
  wx_ = -sinl(d_, w_ >> 6);
  wy_ = cosl(d_, w_ >> 6);

  l_ = 0;
  count_ = 0;

  c_ = info.c;
  subtype_ = static_cast<ReflectLaserType>(info.cmd_type);

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
  case ReflectState::Idle:
    break;
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
  case ReflectState::NoMove:
    UpdateNoMove();
    break;
  case ReflectState::Clearing:
    UpdateClearing();
    break;
  case ReflectState::Dead:
    break;
  }

  return result;
}

void LaserReflect::UpdateGrowing() {
  if (l_ < lmax_) {
    l_ += v_;
    lx_ = cosl(d_, l_ >> 6);
    ly_ = sinl(d_, l_ >> 6);
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

auto LaserReflect::UpdateFlying(std::span<const LaserLong *> longs) -> UpdateResult {
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

auto LaserReflect::UpdateShooting(std::span<const LaserLong *> longs) -> UpdateResult {
  l_ += v_;
  lx_ = cosl(d_, l_ >> 6);
  ly_ = sinl(d_, l_ >> 6);
  auto *p = p_;
  p[1].x = p[0].x + lx_;
  p[1].y = p[0].y + ly_;
  p[2].x = p[3].x + lx_;
  p[2].y = p[3].y + ly_;

  if (l_ >= lmax_) {
    state_ = ReflectState::Flying;
    return {};
  }

  const int dx = cosl(d_, v_);
  const int dy = sinl(d_, v_);

  for (const auto *ll : longs) {
    if (auto hit = CheckLongLaser(*this, *ll, dx, dy); hit.spawn_requested) {
      ltemp_ = l_;
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
  lx_ = cosl(d_, l_ >> 6);
  ly_ = sinl(d_, l_ >> 6);
  auto *p = p_;
  p[0].x = p[1].x - lx_;
  p[0].y = p[1].y - ly_;
  p[3].x = p[2].x - lx_;
  p[3].y = p[2].y - ly_;
}

void LaserReflect::UpdateNoMove() {
  ltemp_ += v_;
  if (ltemp_ >= lmax_) {
    state_ = ReflectState::Reflected;
  }
}

void LaserReflect::UpdateClearing() {
  if (l_ < lmax_) {
    l_ += v_;
    w_ += 16;
    lx_ = cosl(d_, l_ >> 6);
    ly_ = sinl(d_, l_ >> 6);
    auto *p = p_;
    p[1].x = p[0].x + lx_;
    p[1].y = p[0].y + ly_;
    p[2].x = p[3].x + lx_;
    p[2].y = p[3].y + ly_;
  } else {
    w_ += 64;
  }

  wx_ = -sinl(d_, w_ >> 6);
  wy_ = cosl(d_, w_ >> 6);
  SetupGeometry();

  if (count_ > 30) {
    state_ = ReflectState::Dead;
  }
}

// ── Hit detection ───────────────────────────────────────────────────

HitResult LaserReflect::CheckHit(int px, int py) const {
  if (state_ == ReflectState::Dead || state_ == ReflectState::Clearing) {
    return HitResult::Miss;
  }

  const int tx = px - x_;
  const int ty = py - y_;
  const int len = cosl(d_, tx) + sinl(d_, ty);
  const int dist = std::abs(-sinl(d_, tx) + cosl(d_, ty));

  if (len <= 0 || len > l_) return HitResult::Miss;
  if (dist <= w_ + PLAYER_HITBOX_RADIUS) return HitResult::Hit;
  if (dist <= w_ + kLaserEvadeWidth) return HitResult::Graze;
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

bool LaserReflect::IsDead() const {
  return state_ == ReflectState::Dead;
}

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
    const int bx = x_ >> 6;
    const int by = y_ >> 6;
    const int scale = (w_ + kDebugLaserEvadeWidth);
    const int wx2 = wx_ * scale / w_;
    const int wy2 = wy_ * scale / w_;
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
