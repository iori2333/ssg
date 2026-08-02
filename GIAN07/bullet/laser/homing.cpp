///
/// LaserHoming - Homing laser processing
///

#include <array>
#include <cmath>
#include <span>

#include "homing.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "bullet/bullet_common.h"
#include "gameplay/playfield.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
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

void DrawCircleA16(float x, float y, float r, float angle) {
  std::array<VertexXy, 10> src{};
  for (int j = 0, i = -64; j <= 8; j++) {
    const auto offset = math::PolarVector(
        angle + static_cast<float>(i) * math::kLegacyAngleStep, r);
    src[j].x = (x + offset.x) / kWorldCoordScale;
    src[j].y = (y + offset.y) / kWorldCoordScale;
    i += 16;
  }
  src[9] = src[0];
  geometry::DrawTrianglesA(TrianglePrimitive::Fan, src);
}

void DrawTriangleFanAlpha(std::span<const VertexXy, 4> src) {
  geometry::DrawTrianglesA(TrianglePrimitive::Fan, src);
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

void LaserHoming::Update(audio::AudioSystem &audio, const UpdateInfo &info) {
  float const prev_x = p_[current_].x;
  float const prev_y = p_[current_].y;
  float prev_angle = p_[current_].angle;

  count_++;
  current_ = GetNext(current_);

  switch (subtype_) {
  case HomingType::Type1: {
    const auto target =
        math::AngleTo(static_cast<float>(info.player_x) - prev_x,
                      static_cast<float>(info.player_y) - prev_y);
    const auto angle_delta = math::ShortestAngleDelta(target, prev_angle);

    if (std::abs(angle_delta) < 8.0F * math::kLegacyAngleStep) {
      subtype_ = HomingType::None;
      audio.PlaySfx(SfxId::Hlaser,
                    static_cast<int>(std::lround(p_[current_].x)));
    } else {
      if (v_ > 2_px) {
        v_ -= a_;
      }
      const auto turn =
          angle_delta * (1 + static_cast<float>(count_ / 32)) / 32.0F;
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

  int const tail_i = GetNext(current_);
  const float tx = p_[tail_i].x;
  const float ty = p_[tail_i].y;
  if (tx < playfield::kWorldLeft - 4_px || tx > playfield::kWorldRight + 4_px ||
      ty < playfield::kWorldTop - 4_px || ty > playfield::kWorldBottom + 4_px) {
    state_ = HomingState::Dead;
  }
}

// ── Virtual overrides ──────────────────────────────────────────────

void LaserHoming::Render() const {
  constexpr Rgb216 kOuterColor{1, 2, 5};
  constexpr Rgb216 kInnerColor{3, 4, 5};

  // Pass 1: wide outer ribbon
  geometry::SetColor(kOuterColor);
  geometry::SetAlphaOne();

  int w = kHomingWidth;
  int cur = current_;
  const auto *pt = &p_[cur];
  const auto edge = [](const TrailPoint &point, float width) {
    const auto offset =
        math::PolarVector(point.angle - (math::kFullAngle / 4.0F), width);
    return std::array{VertexXy{(point.x + offset.x) / kWorldCoordScale,
                               (point.y + offset.y) / kWorldCoordScale},
                      VertexXy{(point.x - offset.x) / kWorldCoordScale,
                               (point.y - offset.y) / kWorldCoordScale}};
  };

  std::array<VertexXy, 4> src{};
  const auto first_edge = edge(*pt, static_cast<float>(w));
  src[0] = first_edge[0];
  src[1] = first_edge[1];

  DrawCircleA16(pt->x, pt->y, static_cast<float>(w), pt->angle);

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
  geometry::SetColor(kInnerColor);

  w = kHomingWidth / 2;
  cur = current_;
  pt = &p_[cur];

  const auto highlight_edge = edge(*pt, static_cast<float>(w));
  src[0] = highlight_edge[0];
  src[1] = highlight_edge[1];

  DrawCircleA16(pt->x, pt->y, static_cast<float>(w), pt->angle);

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
              static_cast<float>(kHomingWidth * 2 / 3) + player_radius &&
          std::abs(j.y - static_cast<float>(py)) <
              static_cast<float>(kHomingWidth * 2 / 3) + player_radius) {
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
    const int cx = static_cast<int>(pt.x / kWorldCoordScale);
    const int cy = static_cast<int>(pt.y / kWorldCoordScale);

    if (mode >= 2) {
      geometry::DrawFilledCircle({cx, cy}, evade_r, true);
    }
    geometry::DrawFilledCircle({cx, cy}, hit_r, true);
    current = GetPrev(current, kHomingSection);
  }
}
