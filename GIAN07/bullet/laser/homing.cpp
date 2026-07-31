///
/// LaserHoming - Homing laser processing
///

#include <array>
#include <cmath>
#include <span>

#include "homing.h"

#include "audio/sfx.h"
#include "gameplay/playfield.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

namespace {

inline constexpr auto kHomingWidth = 8_px;

constexpr int GetPrev(int current, int n) {
  return (current + n) % (kHomingTrailLength * kHomingSection);
}

constexpr int GetNext(int current) {
  return (current + (kHomingTrailLength * kHomingSection) - 1) %
         (kHomingTrailLength * kHomingSection);
}

void DrawCircleA16(GraphicsGeometry &geometry, float x, float y, float r,
                   float angle) {
  std::array<VERTEX_XY, 10> src{};
  for (int j = 0, i = -64; j <= 8; j++) {
    const auto offset = math::PolarVector(
        angle + static_cast<float>(i) * math::kLegacyAngleStep, r);
    src[j].x = (x + offset.x) / WORLD_COORD_SCALE;
    src[j].y = (y + offset.y) / WORLD_COORD_SCALE;
    i += 16;
  }
  src[9] = src[0];
  geometry.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}

void DrawTriangleFanAlpha(std::span<const VERTEX_XY, 4> src) {
  Geometry().DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}

} // namespace

// ── Spawn ────────────────────────────────────────────────────────────

void LaserHoming::Spawn(const HomingSpawnInfo &info) {
  v_ = 4_px;
  a_ = 10;
  count_ = 0;
  current_ = 0;
  left_ = 1;
  c_ = info.c;
  subtype_ = info.type;
  state_ = HomingState::Normal;

  const auto angle = bullet_common::CalcSpreadAngle(
      info.bullet_index, BulletPattern::Spread, info.n, info.angle, info.dw);

  for (auto &j : p_) {
    j.x = static_cast<float>(info.x);
    j.y = static_cast<float>(info.y);
    j.angle = angle;
  }
}

// ── State machine ──────────────────────────────────────────────────

void LaserHoming::Update(const UpdateInfo &info) {
  float prev_x = p_[current_].x;
  float prev_y = p_[current_].y;
  float prev_angle = p_[current_].angle;

  count_++;
  current_ = GetNext(current_);

  switch (subtype_) {
  case HomingType::Type1: {
    const auto target =
        math::AngleTo(static_cast<float>(info.player_x) - prev_x,
                      static_cast<float>(info.player_y) - prev_y);
    const auto angle_delta = math::ShortestAngleDelta(target, prev_angle);

    if (std::abs(angle_delta) < 8.0f * math::kLegacyAngleStep) {
      subtype_ = HomingType::None;
      PlaySfx(SfxId::Hlaser, static_cast<int>(std::lround(p_[current_].x)));
    } else {
      if (v_ > 2_px) {
        v_ -= a_;
      }
      const auto turn =
          angle_delta * static_cast<float>(1 + (count_ / 32)) / 32.0f;
      prev_angle +=
          std::abs(turn) >= math::kLegacyAngleStep ? turn : angle_delta;
    }

    if (count_ > 120) {
      subtype_ = HomingType::None;
    }

    const auto velocity = math::PolarVector(prev_angle, v_);
    p_[current_].angle = prev_angle;
    p_[current_].x = prev_x + velocity.x;
    p_[current_].y = prev_y + velocity.y;
    break;
  }

  case HomingType::None:
    v_ += a_ * 2;
    {
      const auto velocity = math::PolarVector(prev_angle, v_);
      p_[current_].angle = prev_angle;
      p_[current_].x = prev_x + velocity.x;
      p_[current_].y = prev_y + velocity.y;
    }
    break;
  }

  int tail_i = GetNext(current_);
  const float tx = p_[tail_i].x;
  const float ty = p_[tail_i].y;
  if (tx < playfield::kWorldLeft - 4_px || tx > playfield::kWorldRight + 4_px ||
      ty < playfield::kWorldTop - 4_px || ty > playfield::kWorldBottom + 4_px) {
    state_ = HomingState::Dead;
  }
}

// ── Virtual overrides ──────────────────────────────────────────────

void LaserHoming::Render() const {
  constexpr RGB216 kOuterColor{1, 2, 5};
  constexpr RGB216 kInnerColor{3, 4, 5};

  // Pass 1: wide outer ribbon
  Geometry().SetColor(kOuterColor);
  Geometry().SetAlphaOne();

  int w = kHomingWidth;
  int cur = current_;
  const auto *pt = &p_[cur];
  const auto edge = [](const TrailPoint &point, float width) {
    const auto offset =
        math::PolarVector(point.angle - (math::kFullAngle / 4.0f), width);
    return std::array{VERTEX_XY{(point.x + offset.x) / WORLD_COORD_SCALE,
                                (point.y + offset.y) / WORLD_COORD_SCALE},
                      VERTEX_XY{(point.x - offset.x) / WORLD_COORD_SCALE,
                                (point.y - offset.y) / WORLD_COORD_SCALE}};
  };

  VERTEX_XY src[4];
  const auto first_edge = edge(*pt, static_cast<float>(w));
  src[0] = first_edge[0];
  src[1] = first_edge[1];

  DrawCircleA16(Geometry(), pt->x, pt->y, static_cast<float>(w), pt->angle);

  for (int i = 0; i < kHomingTrailLength - 1; i++) {
    cur = GetPrev(cur, kHomingSection);
    pt = &p_[cur];

    const auto next_edge = edge(*pt, static_cast<float>(w));
    src[2] = next_edge[1];
    src[3] = next_edge[0];
    DrawTriangleFanAlpha(src);

    src[0] = src[3];
    src[1] = src[2];

    if (w > 2_px) {
      w -= 64;
    }
  }

  // Pass 2: narrow inner highlight
  Geometry().SetColor(kInnerColor);

  w = kHomingWidth / 2;
  cur = current_;
  pt = &p_[cur];

  const auto highlight_edge = edge(*pt, static_cast<float>(w));
  src[0] = highlight_edge[0];
  src[1] = highlight_edge[1];

  DrawCircleA16(Geometry(), pt->x, pt->y, static_cast<float>(w), pt->angle);

  for (int i = 0; i < kHomingTrailLength - 1; i++) {
    cur = GetPrev(cur, kHomingSection);
    pt = &p_[cur];

    const auto next_edge = edge(*pt, static_cast<float>(w));
    src[2] = next_edge[1];
    src[3] = next_edge[0];
    DrawTriangleFanAlpha(src);

    src[0] = src[3];
    src[1] = src[2];

    if (w > 64) {
      w -= 64;
    } else {
      break;
    }
  }
}

bool LaserHoming::IsDead() const { return state_ == HomingState::Dead; }

void LaserHoming::Kill() { state_ = HomingState::Dead; }

// ── Hit detection ──────────────────────────────────────────────────

HitResult LaserHoming::CheckHit(int px, int py, int player_radius) const {
  if (state_ == HomingState::Dead) {
    return HitResult::Miss;
  }

  bool grazed = false;
  for (const auto &j : p_) {
    if (std::abs(j.x - static_cast<float>(px)) < kHomingWidth + 15_px &&
        std::abs(j.y - static_cast<float>(py)) < kHomingWidth + 15_px) {
      if (std::abs(j.x - static_cast<float>(px)) <
              kHomingWidth * 2 / 3 + player_radius &&
          std::abs(j.y - static_cast<float>(py)) <
              kHomingWidth * 2 / 3 + player_radius) {
        return HitResult::Hit;
      }
      grazed = true;
    }
  }
  return grazed ? HitResult::Graze : HitResult::Miss;
}

// ── Debug ──────────────────────────────────────────────────────────

void LaserHoming::RenderDebugHitbox(int mode) const {
  if (state_ == HomingState::Dead) {
    return;
  }
  const int hit_r = (kHomingWidth * 2 / 3) >> 6;
  const int evade_r = (kHomingWidth + 15_px) >> 6;

  int current = current_;
  for (int j = 0; j < kHomingTrailLength; j++) {
    const auto &pt = p_[current];
    const int cx = static_cast<int>(pt.x / WORLD_COORD_SCALE);
    const int cy = static_cast<int>(pt.y / WORLD_COORD_SCALE);

    if (mode >= 2) {
      geometry::DrawFilledCircle(Geometry(), {cx, cy}, evade_r, true);
    }
    geometry::DrawFilledCircle(Geometry(), {cx, cy}, hit_r, true);
    current = GetPrev(current, kHomingSection);
  }
}
