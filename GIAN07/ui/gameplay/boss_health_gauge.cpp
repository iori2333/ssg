///
/// Boss health and timeout HUD
///

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <utility>

#include "boss_health_gauge.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "enemy/boss/boss.h"
#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"

static constexpr auto kBossHealthGaugeWidth = 256;
static constexpr auto kBossHealthGaugeStartX = playfield::kRight;
static constexpr auto kBossHealthGaugeEndX = 260;

void BossHealthGauge::Sync(const BossHudModel &model) {
  if (encounter_revision_ != model.encounter_revision) {
    encounter_revision_ = model.encounter_revision;
    phase_revision_ = model.phase_revision;
    if (model.active) {
      Open(model.max_hp);
    } else {
      Reset();
    }
  } else if (phase_revision_ != model.phase_revision) {
    phase_revision_ = model.phase_revision;
    AddPhase(model.phase_hp);
  }

  SetCombatState(model.phase_threshold_hp, model.timer_max, model.timer_now);
  SetStageTimeout(model.stage_timeout_end);
  Update(model.current_hp);
}

void BossHealthGauge::Reset() {
  state_ = State::Hidden;
  current_hp_ = 0;
  max_hp_ = 0;
  target_hp_ = 0;
  phase_hp_ = 0;
  phase_threshold_hp_ = -1;
  timer_max_ = -1;
  timer_now_ = 0;
  previous_timer_seconds_ = -1;
  stage_timeout_end_ = -1;
}

void BossHealthGauge::SetCombatState(int phase_threshold_hp, int timer_max,
                                     int timer_now) {
  phase_threshold_hp_ = phase_threshold_hp;
  timer_max_ = timer_max;
  timer_now_ = timer_now;
}

void BossHealthGauge::SetStageTimeout(int timeout_end) {
  stage_timeout_end_ = timeout_end;
}

void BossHealthGauge::Open(uint32_t max) {
  if (max == 0) {
    Reset();
    return;
  }
  max_hp_ = max;
  current_hp_ = 0;
  target_hp_ = max;
  phase_hp_ = max;

  state_ = State::OpeningFrame;
  stage_timeout_end_ = -1;
  previous_timer_seconds_ = -1;

  for (std::size_t index = 0; index < row_x_.size(); ++index) {
    row_x_[index] = kBossHealthGaugeStartX + static_cast<int>(index * 20);
  }
}

void BossHealthGauge::AddPhase(uint32_t next) {
  phase_hp_ = next;
  state_ = State::Refilling;
}

void BossHealthGauge::Update(uint32_t now) {
  std::size_t settled_rows = 0;

  target_hp_ = now;

  switch (state_) {
  case State::OpeningFrame: {
    for (auto &it : row_x_) {
      it -= 6;
      if (it <= kBossHealthGaugeEndX) {
        it = kBossHealthGaugeEndX;
        ++settled_rows;
      }
    }

    if (settled_rows == row_x_.size()) {
      state_ = State::Filling;
    }
  } break;

  case State::Filling:
    current_hp_ += ((max_hp_ >> 7) + 1);
    if (current_hp_ >= max_hp_) {
      current_hp_ = max_hp_;
      state_ = State::Ready;
    }
    break;

  case State::Refilling:
    current_hp_ += ((max_hp_ >> 7) + 1);
    if (current_hp_ >= phase_hp_) {
      current_hp_ = phase_hp_;
      state_ = State::Ready;
    }
    break;

  case State::Ready:
    if (current_hp_ > target_hp_) {
      const auto temp =
          (std::max)(((std::max)(max_hp_, 1U) / (30 * 8 * 4)), 3U);
      if (current_hp_ - target_hp_ > temp) {
        current_hp_ -= temp;
      } else {
        current_hp_ = target_hp_;
      }
    }
    if (current_hp_ == 0) {
      Close();
    }
    break;

  case State::Closing:
    row_x_[kBossHealthGaugeHeight - 1] += 6;
    for (std::size_t index = row_x_.size() - 1; index-- > 0;) {
      row_x_[index] = std::max(row_x_[index], row_x_[index + 1] - 20);
    }
    if (row_x_[0] >= kBossHealthGaugeStartX) {
      state_ = State::Hidden;
    }
    break;

  case State::Hidden:
    return;
  }
}

void BossHealthGauge::Close() { state_ = State::Closing; }

void BossHealthGauge::Draw(int stage_frame) {
  PixelLtrb src;
  int i = 0;

  switch (state_) {
  case State::OpeningFrame:
  case State::Closing:
    for (i = 0; std::cmp_less(i, kBossHealthGaugeHeight); i++) {
      src = {0, (104 + i), kBossHealthGaugeWidth, (104 + i + 1)};
      GraphicsSurfaceBlit({row_x_[i], (16 + i)}, SurfaceId::System, src);
    }
    break;

  case State::Filling:
  case State::Ready:
  case State::Refilling: {
    constexpr WindowCoord left = (kBossHealthGaugeEndX + 3);
    constexpr WindowCoord top = (16 + 3);
    constexpr WindowCoord bottom = (top + 11);
    const auto x1 = (left + ((target_hp_ * 30 * 8) / max_hp_));
    const auto x2 = (left + ((current_hp_ * 30 * 8) / max_hp_));
    constexpr uint8_t alpha = (128 + 64);
    constexpr Rgb216 col = {0, 1, 5};

    geometry::SetAlphaNorm(alpha);
    std::array<VertexXy, 4> src_vertices = {
        VertexXy{0, top},
        VertexXy{left, top},
        VertexXy{left, bottom},
        VertexXy{0, bottom},
    };
    if (x1 < x2) {
      src_vertices[0].x = src_vertices[3].x = x1;
      geometry::DrawGradientRect(src_vertices, col.ToRgb().WithAlpha(alpha),
                                 true);
      geometry::SetColor({5, 0, 0});
      geometry::DrawBoxA(x1, top, x2, bottom);
    } else {
      src_vertices[0].x = src_vertices[3].x = x2;
      geometry::DrawGradientRect(src_vertices, col.ToRgb().WithAlpha(alpha),
                                 true);
    }

    src = {0, 104, kBossHealthGaugeWidth, 128};
    GraphicsSurfaceBlit({kBossHealthGaugeEndX, 16}, SurfaceId::System, src);

    if (phase_threshold_hp_ > 0 && max_hp_ > 0) {
      const auto separator_x =
          left + static_cast<int>(
                     (static_cast<uint64_t>(phase_threshold_hp_) * 30 * 8) /
                     max_hp_);
      if (separator_x > left && separator_x < (left + 30 * 8)) {
        geometry::SetAlphaNorm(224);
        geometry::SetColor({5, 5, 5});
        geometry::DrawBoxA(separator_x, top, (separator_x + 3), bottom);
      }
    }

    if (timer_max_ > 0) {
      const int remain = std::min((timer_max_ - timer_now_) / 60, 99);
      if (remain >= 0) {
        if (remain <= 10 && remain != previous_timer_seconds_) {
          audio_.PlaySfx(SfxId::Sblaser);
        }
        previous_timer_seconds_ = remain;
        if (remain < 10) {
          GraphicsSurfaceSetColorMod(SurfaceId::System, 255, 64, 64);
        }
        DrawFont16(476, 0, std::format("{:>2}", remain).c_str());
        if (remain < 10) {
          GraphicsSurfaceSetColorMod(SurfaceId::System, 255, 255, 255);
        }
      }
    } else if (stage_timeout_end_ > 0) {
      const int remain =
          std::min((stage_timeout_end_ - stage_frame) / 60, 99);
      if (remain >= 0) {
        if (remain <= 10 && remain != previous_timer_seconds_) {
          audio_.PlaySfx(SfxId::Sblaser);
        }
        previous_timer_seconds_ = remain;
        if (remain < 10) {
          GraphicsSurfaceSetColorMod(SurfaceId::System, 255, 64, 64);
        }
        DrawFont16(476, 0, std::format("{:>2}", remain).c_str());
        if (remain < 10) {
          GraphicsSurfaceSetColorMod(SurfaceId::System, 255, 255, 255);
        }
      }
    }
  } break;

  case State::Hidden:
    break;
  }
}
