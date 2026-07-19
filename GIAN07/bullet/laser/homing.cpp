///
/// LaserHoming - Homing laser processing
///

#include <array>
#include <cmath>

#include "homing.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "gfx/graphics_backend.h"
#include "gfx/geometry.h"
#include "player/player.h"
#include "util/ut_math.h"

namespace {

inline constexpr auto kHomingWidth = 8 * 64;

constexpr int GetPrev(int current, int n) {
  return (current + n) % (kHomingLen * kHomingSection);
}

constexpr int GetNext(int current) {
  return (current + (kHomingLen * kHomingSection) - 1) %
         (kHomingLen * kHomingSection);
}

template <GRAPHICS_GEOMETRY_POLY Gp>
void DrawCircleA16(Gp &gp, int x, int y, int r, uint8_t d) {
  std::array<VERTEX_XY, 10> src{};
  for (int j = 0, i = -64; j <= 8; j++) {
    src[j].x = (x + cosl(d + i, r)) >> 6;
    src[j].y = (y + sinl(d + i, r)) >> 6;
    i += 16;
  }
  src[9] = src[0];
  gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}

void DrawTriangleFanAlpha(std::span<const VERTEX_XY, 4> src) {
  if (auto *gp = GrpGeom_Poly()) {
    gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
  } else if (auto *gf = GrpGeom_FB()) {
    gf->DrawTriangleFan(src);
  }
}

} // namespace

uint8_t ComputeDeg(const HomingSpawnInfo &info, int i) {
  if ((info.n & 1) != 0) {
    return info.d + ((i >> 1) * info.dw * (1 - ((i & 1) << 1)));
  }
  return info.d - (info.dw >> 1) +
         ((i >> 1) * info.dw * (1 - ((i & 1) << 1)));
}

// ── Spawn ────────────────────────────────────────────────────────────

void LaserHoming::Spawn(const HomingSpawnInfo &info) {
  v_ = 64 * 4;
  a_ = 10;
  count_ = 0;
  current_ = 0;
  left_ = 1;
  c_ = info.c;
  subtype_ = static_cast<HomingType>(info.type);
  state_ = HomingState::Normal;

  auto deg = ComputeDeg(info, info.bullet_index);

  for (auto &j : p_) {
    j.x = info.x;
    j.y = info.y;
    j.d = deg;
  }
}

// ── State machine ──────────────────────────────────────────────────

void LaserHoming::Update() {
  int prev_x = p_[current_].x;
  int prev_y = p_[current_].y;
  int prev_deg = p_[current_].d;

  count_++;
  current_ = GetNext(current_);

  switch (subtype_) {
  case HomingType::Type1: {
    int deg2 =
        -prev_deg + atan8(Players.X() - prev_x, Players.Y() - prev_y);
    if (deg2 < -128) {
      deg2 += 256;
    } else if (deg2 > 128) {
      deg2 -= 256;
    }

    if (std::abs(deg2) < 8) {
      subtype_ = HomingType::None;
      Snd_SEPlay(static_cast<SfxId>(17), p_[current_].x);
    } else {
      if (v_ > 2 * 64) {
        v_ -= a_;
      }
      int i = 1 + (static_cast<int>(count_) / 32);
      i = (deg2 * i) / 32;
      if (i != 0) {
        prev_deg = prev_deg + i;
      } else {
        prev_deg = prev_deg + deg2;
      }
    }

    if (count_ > 120) {
      subtype_ = HomingType::None;
    }

    p_[current_].d = prev_deg;
    p_[current_].x = prev_x + cosl(prev_deg, v_);
    p_[current_].y = prev_y + sinl(prev_deg, v_);
    break;
  }

  case HomingType::None:
    v_ += a_ * 2;
    p_[current_].d = prev_deg;
    p_[current_].x = prev_x + cosl(prev_deg, v_);
    p_[current_].y = prev_y + sinl(prev_deg, v_);
    break;
  }

  int tail_i = GetNext(current_);
  int tx = p_[tail_i].x;
  int ty = p_[tail_i].y;
  if (tx < GX_MIN - (4 * 64) || tx > GX_MAX + (4 * 64) ||
      ty < GY_MIN - (4 * 64) || ty > GY_MAX + (4 * 64)) {
    state_ = HomingState::Dead;
  }
}

// ── Virtual overrides ──────────────────────────────────────────────

void LaserHoming::Render() const {
  constexpr RGB216 kOuterColor{1, 2, 5};
  constexpr RGB216 kInnerColor{3, 4, 5};

  // Pass 1: wide outer ribbon
  if (auto *gp = GrpGeom_Poly()) {
    gp->SetColor(kOuterColor);
    gp->SetAlphaOne();
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({2, 2, 5});
  }

  int w = kHomingWidth;
  int cur = current_;
  const DegPoint *pt = &p_[cur];

  VERTEX_XY src[4];
  src[0].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
  src[0].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
  src[1].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
  src[1].y = (pt->y - sinl(pt->d - 64, w)) >> 6;

  if (auto *gp = GrpGeom_Poly()) {
    DrawCircleA16(*gp, pt->x, pt->y, w, pt->d);
  } else {
    GeomCircleF({(pt->x >> 6), (pt->y >> 6)}, (w >> 6));
  }

  for (int i = 0; i < kHomingLen - 1; i++) {
    cur = GetPrev(cur, kHomingSection);
    pt = &p_[cur];

    src[2].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
    src[2].y = (pt->y - sinl(pt->d - 64, w)) >> 6;
    src[3].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
    src[3].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
    DrawTriangleFanAlpha(src);

    src[0] = src[3];
    src[1] = src[2];

    if (w > 64 * 2) {
      w -= 64;
    }
  }

  // Pass 2: narrow inner highlight
  if (auto *gp = GrpGeom_Poly()) {
    gp->SetColor(kInnerColor);
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({5, 5, 5});
  }

  w = kHomingWidth / 2;
  cur = current_;
  pt = &p_[cur];

  src[0].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
  src[0].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
  src[1].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
  src[1].y = (pt->y - sinl(pt->d - 64, w)) >> 6;

  if (auto *gp = GrpGeom_Poly()) {
    DrawCircleA16(*gp, pt->x, pt->y, w, pt->d);
  } else {
    GeomCircleF({(pt->x >> 6), (pt->y >> 6)}, (w >> 6));
  }

  for (int i = 0; i < kHomingLen - 1; i++) {
    cur = GetPrev(cur, kHomingSection);
    pt = &p_[cur];

    src[2].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
    src[2].y = (pt->y - sinl(pt->d - 64, w)) >> 6;
    src[3].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
    src[3].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
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

HitResult LaserHoming::CheckHit(int px, int py) const {
  if (state_ == HomingState::Dead) {
    return HitResult::Miss;
  }

  for (auto &j : p_) {
    if (HITCHK(j.x, px, kHomingWidth + 15 * 64) &&
        HITCHK(j.y, py, kHomingWidth + 15 * 64)) {
      if (HITCHK(j.x, px,
                 kHomingWidth * 2 / 3 + PLAYER_HITBOX_RADIUS) &&
          HITCHK(j.y, py,
                 kHomingWidth * 2 / 3 + PLAYER_HITBOX_RADIUS)) {
        return HitResult::Hit;
      }
      return HitResult::Graze;
    }
  }
  return HitResult::Miss;
}
