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
  if (len == 0) {
    return;
  }

  const RGBA col = kTable16Bit[cval].ToRGB().WithAlpha(0xFF);
  GrpGeom->SetAlphaOne();
  GeomGrdRectA(*GrpGeom, p_, col);

  std::array<VERTEX_RGBA, kBeamVertexCount> vcs{};
  vcs[0] = {255, 255, 255, 0xFF};
  for (auto &vc : vcs | std::views::drop(1)) {
    vc = col;
  }

  std::array<VERTEX_XY, kBeamVertexCount> points{};
  points[0] = {px, py};
  points[1] = p_[0];
  points[kBeamVertexCount - 1] = p_[3];
  for (auto n = 2; n < (kBeamVertexCount - 1); n++) {
    const auto cap_angle = angle_ + (math::kFullAngle / 4.0f) +
                           (math::kFullAngle / 2.0f) * (n - 1) / 32.0f;
    const auto offset = math::PolarVector(cap_angle, len);
    points[n] = {points[0].x + offset.x, points[0].y + offset.y};
  }
  GrpGeom->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, points, vcs);
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
  const std::array<VERTEX_XY, 4> strip = {p_[0], p_[3], p_[1], p_[2]};
  GrpGeom->DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, strip);

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
    GrpGeom->DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, estrip);
  }
}
