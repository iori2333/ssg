///
/// LaserLong - Long laser processing
///

#include <array>
#include <cmath>
#include <ranges>

#include "long.h"

#include "audio/snd.h"
#include "enemy/actor/enemy_actor.h"
#include "enemy/ecl/ecl.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

namespace {

// ── Color tables ────────────────────────────────────────────────────
// clang-format off
inline constexpr RGB216 kTable16Bit[16] = {
    {3, 0, 3}, {0, 2, 0}, {0, 0, 4}, {4, 2, 0}, {0, 0, 1},
};
inline constexpr RGB216 kTable8BitA[16] = {
    {2, 0, 2}, {0, 2, 0}, {0, 1, 3}, {4, 2, 0}, {0, 0, 1},
};
inline constexpr RGB216 kTable8BitB[16] = {
    {3, 0, 3}, {0, 4, 0}, {0, 1, 5}, {5, 3, 0}, {2, 2, 4},
};
inline constexpr RGB216 kTable8BitC[16] = {
    {5, 4, 5}, {5, 5, 5}, {4, 4, 5}, {5, 5, 4}, {4, 4, 5},
};
// clang-format on

inline constexpr size_t kBeamVertexCount = 34;
inline constexpr auto kBeamLength = 800;
inline constexpr auto kDebugLaserEvadeWidth = 15_px;
inline constexpr auto kLongLaserEvadeWidth = 15_px;

} // namespace

// ── Spawn ────────────────────────────────────────────────────────────

void LaserLong::Spawn(const LongLaserSpawnInfo &info) {
  dx_ = info.dx;
  dy_ = info.dy;
  e_ = info.enemy;
  enemy_id_ = info.enemy_id;
  x_ = info.enemy->x + info.dx;
  y_ = info.enemy->y + info.dy;
  v_ = info.v;
  c_ = info.c;
  lx_ = 0;
  ly_ = 0;
  wx_ = 0;
  wy_ = 0;
  w_ = 0;
  wmax_ = info.w;
  angle_ = info.angle;

  if (info.type == LongLaserType::LongZ) {
    angle_ += math::AngleTo(static_cast<float>(info.player_x) - x_,
                            static_cast<float>(info.player_y) - y_);
    subtype_ = LongLaserType::Long;
  } else {
    subtype_ = info.type;
  }

  const auto beam = math::PolarVector(angle_, static_cast<float>(kBeamLength));
  infx_ = beam.x;
  infy_ = beam.y;
  count_ = 0;

  RecalcGeometry();

  state_ = LongState::Line;
}

// ── Geometry ──────────────────────────────────────────────────────

void LaserLong::RecalcGeometry() {
  auto *pp = p_;

  pp[1].x = pp[0].x = (x_ / WORLD_COORD_SCALE) + wx_ + lx_;
  pp[1].y = pp[0].y = (y_ / WORLD_COORD_SCALE) + wy_ + ly_;

  pp[2].x = pp[3].x = (x_ / WORLD_COORD_SCALE) - wx_ + lx_;
  pp[2].y = pp[3].y = (y_ / WORLD_COORD_SCALE) - wy_ + ly_;

  pp[1].x += infx_;
  pp[1].y += infy_;
  pp[2].x += infx_;
  pp[2].y += infy_;
}

// ── Update ──────────────────────────────────────────────────────────

void LaserLong::Update(const UpdateInfo &info) {
  if (info.command == LongLaserUpdateInfo::Command::Tick) {
    TickUpdate();
  } else {
    ApplyCommand(info.command, info.angle, info.delta);
  }
}

// ── Per-frame tick ──────────────────────────────────────────────────

void LaserLong::TickUpdate() {
  ++count_;

  if (e_ == nullptr) {
    if (state_ != LongState::Closing && state_ != LongState::ClosingToLine) {
      MarkDead();
      return;
    }
  } else {
    if (subtype_ == LongLaserType::SetDeg) {
      const auto enemy_angle = math::AngleFromLegacy(e_->d);
      if (angle_ != enemy_angle) {
        angle_ = enemy_angle;
        FixAngleGeometry();
      }
    }

    x_ = e_->x + dx_;
    y_ = e_->y + dy_;
    RecalcGeometry();
  }

  if (state_ == LongState::Inactive) {
    return;
  }

  switch (state_) {
  case LongState::Opening:
    UpdateOpening();
    break;

  case LongState::Closing:
  case LongState::ClosingToLine:
    UpdateClosing();
    break;

  case LongState::Active:
  case LongState::Line:
    break;

  default:
    break;
  }
}

void LaserLong::UpdateOpening() {
  w_ += v_;
  if (w_ >= wmax_) {
    w_ = wmax_;
    state_ = LongState::Active;
  }

  const auto width =
      math::PolarVector(angle_, w_ / static_cast<float>(WORLD_COORD_SCALE));
  lx_ = width.x;
  ly_ = width.y;
  wx_ = -width.y;
  wy_ = width.x;

  RecalcGeometry();
}

void LaserLong::UpdateClosing() {
  w_ -= v_;
  if (w_ <= 0) {
    w_ = 0;
    if (state_ == LongState::Closing) {
      state_ = LongState::Inactive;
      e_ = nullptr;
    } else {
      state_ = LongState::Line;
    }
  }

  const auto width =
      math::PolarVector(angle_, w_ / static_cast<float>(WORLD_COORD_SCALE));
  lx_ = width.x;
  ly_ = width.y;
  wx_ = -width.y;
  wy_ = width.x;

  RecalcGeometry();
}

HitResult LaserLong::CheckHit(int px, int py, int player_radius) const {
  if (state_ != LongState::Opening && state_ != LongState::Active) {
    return HitResult::Miss;
  }

  const float tx = static_cast<float>(px) - x_;
  const float ty = static_cast<float>(py) - y_;
  const float angle_cos = std::cos(angle_);
  const float angle_sin = std::sin(angle_);
  const float len = angle_cos * tx + angle_sin * ty;
  const float dist = std::abs(-angle_sin * tx + angle_cos * ty);

  if (len <= 0)
    return HitResult::Miss;
  if (dist <= w_ + player_radius)
    return HitResult::Hit;
  if (dist <= w_ + kLongLaserEvadeWidth)
    return HitResult::Graze;
  return HitResult::Miss;
}

// ── Virtual overrides ──────────────────────────────────────────────

void LaserLong::Render() const {
  switch (state_) {
  case LongState::Opening:
  case LongState::Active:
  case LongState::Closing:
  case LongState::ClosingToLine:
    DrawBeam();
    break;

  case LongState::Line:
    DrawPreviewLine();
    break;

  case LongState::Inactive:
    break;
  }
}

bool LaserLong::IsDead() const { return state_ == LongState::Inactive; }

int LaserLong::X() const { return static_cast<int>(std::lround(x_)); }

void LaserLong::Kill() {
  if (state_ != LongState::Inactive) {
    state_ = LongState::Closing;
  }
}

// ── Render helpers ─────────────────────────────────────────────────

void LaserLong::DrawBeam() const {
  const auto cval = c_;

  const auto px = (x_ / WORLD_COORD_SCALE) + lx_;
  const auto py = (y_ / WORLD_COORD_SCALE) + ly_;
  const auto len = std::hypot(wx_, wy_);

  // Layer 1: outer gradient + fan cap
  if (len != 0) {
    if (auto *gp = GrpGeom_Poly()) {
      const RGBA col = kTable16Bit[cval].ToRGB().WithAlpha(0xFF);
      gp->SetAlphaOne();
      GeomGrdRectA(*gp, p_, col);

      std::array<VERTEX_RGBA, kBeamVertexCount> vcs{};
      vcs[0] = {255, 255, 255, 0xFF};
      for (auto &vc : vcs | std::views::drop(1)) {
        vc = col;
      }

      std::array<VERTEX_XY, kBeamVertexCount> p2{};
      p2[0].x = px;
      p2[0].y = py;
      p2[1].x = p_[0].x;
      p2[1].y = p_[0].y;
      p2[kBeamVertexCount - 1].x = p_[3].x;
      p2[kBeamVertexCount - 1].y = p_[3].y;
      for (auto n = 2; n < (kBeamVertexCount - 1); n++) {
        const auto cap_angle = angle_ + (math::kFullAngle / 4.0f) +
                               (math::kFullAngle / 2.0f) * (n - 1) / 32.0f;
        const auto offset = math::PolarVector(cap_angle, len);
        p2[n].x = p2[0].x + offset.x;
        p2[n].y = p2[0].y + offset.y;
      }
      gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p2, vcs);
      return;
    }
    if (auto *gf = GrpGeom_FB()) {
      gf->SetColor(kTable8BitA[cval]);
      gf->DrawTriangleFan(p_);
    }
  } else {
    if (GrpGeom_Poly() != nullptr) {
      return;
    }
  }

  const WINDOW_POINT center{static_cast<int>(std::lround(px)),
                            static_cast<int>(std::lround(py))};
  GeomCircleF(center, static_cast<int>(std::lround(len)));

  // Layer 2: middle strip
  GrpGeom->SetColor(kTable8BitB[cval]);
  if (len != 0) {
    VERTEX_XY inner[4];
    inner[0].x = inner[1].x = p_[0].x - (wx_ / 8);
    inner[0].y = inner[1].y = p_[0].y - (wy_ / 8);
    inner[3].x = inner[2].x = p_[3].x + (wx_ / 8);
    inner[3].y = inner[2].y = p_[3].y + (wy_ / 8);
    inner[1].x += infx_;
    inner[1].y += infy_;
    inner[2].x += infx_;
    inner[2].y += infy_;
    GrpGeom->DrawTriangleFan(inner);
  }
  GeomCircleF(center, static_cast<int>(std::lround(len - (len / 8.0f))));

  // Layer 3: inner core
  GrpGeom->SetColor(kTable8BitC[cval]);
  if (len != 0) {
    VERTEX_XY inner[4];
    inner[0].x = inner[1].x = p_[0].x - (wx_ / 4);
    inner[0].y = inner[1].y = p_[0].y - (wy_ / 4);
    inner[3].x = inner[2].x = p_[3].x + (wx_ / 4);
    inner[3].y = inner[2].y = p_[3].y + (wy_ / 4);
    inner[1].x += infx_;
    inner[1].y += infy_;
    inner[2].x += infx_;
    inner[2].y += infy_;
    GrpGeom->DrawTriangleFan(inner);
  }
  GeomCircleF(center, static_cast<int>(std::lround(len - (len / 4.0f))));
}

void LaserLong::DrawPreviewLine() const {
  const auto px = x_ / WORLD_COORD_SCALE;
  const auto py = y_ / WORLD_COORD_SCALE;
  GrpGeom->SetColor({4, 4, 4});
  GrpGeom->DrawLine(px, py, (px + infx_), (py + infy_));
}

// ── Command dispatch ─────────────────────────────────────────────────

void LaserLong::ApplyCommand(LongLaserUpdateInfo::Command cmd, float angle,
                             float delta) {
  using Cmd = LongLaserUpdateInfo::Command;
  switch (cmd) {
  case Cmd::Open:
    if (state_ == LongState::Inactive) {
      return;
    }
    state_ = LongState::Opening;
    break;
  case Cmd::Close:
    if (state_ == LongState::Inactive) {
      return;
    }
    state_ = LongState::Closing;
    break;
  case Cmd::CloseToLine:
    if (state_ == LongState::Inactive) {
      return;
    }
    state_ = LongState::ClosingToLine;
    break;
  case Cmd::ForceClose:
    state_ = LongState::Closing;
    e_ = nullptr;
    break;
  case Cmd::SetAngle:
    angle_ = angle;
    FixAngleGeometry();
    break;
  case Cmd::AdjustAngle:
    angle_ += delta;
    FixAngleGeometry();
    break;
  default:
    break;
  }
}

void LaserLong::FixAngleGeometry() {
  const auto width =
      math::PolarVector(angle_, w_ / static_cast<float>(WORLD_COORD_SCALE));
  lx_ = width.x;
  ly_ = width.y;
  wx_ = -width.y;
  wy_ = width.x;
  const auto beam = math::PolarVector(angle_, static_cast<float>(kBeamLength));
  infx_ = beam.x;
  infy_ = beam.y;
  RecalcGeometry();
}

bool LaserLong::BelongsTo(const EnemyActor *e, uint8_t id) const {
  return e_ == e && e != nullptr &&
         (enemy_id_ == id || id == ECL_ALL_LONG_LASERS);
}

// ── Debug ──────────────────────────────────────────────────────────

void LaserLong::RenderDebugHitbox(int mode) const {
  if (state_ != LongState::Active && state_ != LongState::Opening) {
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
    const float lx2 = lx_ * scale / w_;
    const float ly2 = ly_ * scale / w_;
    VERTEX_XY ep[4];
    ep[1].x = ep[0].x = static_cast<float>(bx + wx2 + lx2);
    ep[1].y = ep[0].y = static_cast<float>(by + wy2 + ly2);
    ep[2].x = ep[3].x = static_cast<float>(bx - wx2 + lx2);
    ep[2].y = ep[3].y = static_cast<float>(by - wy2 + ly2);
    ep[1].x += static_cast<float>(infx_);
    ep[1].y += static_cast<float>(infy_);
    ep[2].x += static_cast<float>(infx_);
    ep[2].y += static_cast<float>(infy_);
    const std::array<VERTEX_XY, 4> estrip = {ep[0], ep[3], ep[1], ep[2]};
    gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, estrip);
  }
}
