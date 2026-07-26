///
/// Boss health and timeout HUD
///

#include <algorithm>
#include <cstddef>
#include <format>

#include "boss_health_gauge.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"

static constexpr auto BOSS_HEALTH_GAUGE_WIDTH = 256;
static constexpr auto BOSS_HEALTH_GAUGE_START_X = X_MAX;
static constexpr auto BOSS_HEALTH_GAUGE_END_X = 260;

void BossHealthGauge::Reset() { state_ = State::Hidden; }

void BossHealthGauge::SetCombatState(int32_t phase_threshold_hp,
                                     int32_t timer_max, int32_t timer_now) {
  phase_threshold_hp_ = phase_threshold_hp;
  timer_max_ = timer_max;
  timer_now_ = timer_now;
}

void BossHealthGauge::SetStageTimeout(int32_t timeout_end) {
  stage_timeout_end_ = timeout_end;
}

void BossHealthGauge::Open(uint32_t max) {
  max_hp_ = max;
  current_hp_ = 0;
  target_hp_ = max;
  phase_hp_ = max;

  state_ = State::OpeningFrame;
  stage_timeout_end_ = -1;

  for (std::size_t index = 0; index < row_x_.size(); ++index) {
    row_x_[index] = BOSS_HEALTH_GAUGE_START_X + static_cast<int>(index * 20);
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
      if (it <= BOSS_HEALTH_GAUGE_END_X) {
        it = BOSS_HEALTH_GAUGE_END_X;
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
    row_x_[BOSS_HEALTH_GAUGE_HEIGHT - 1] += 6;
    for (std::size_t index = row_x_.size() - 1; index-- > 0;) {
      row_x_[index] = std::max(row_x_[index], row_x_[index + 1] - 20);
    }
    if (row_x_[0] >= BOSS_HEALTH_GAUGE_START_X) {
      state_ = State::Hidden;
    }
    break;

  case State::Hidden:
    return;
  }
}

void BossHealthGauge::Close() { state_ = State::Closing; }

void BossHealthGauge::Draw(uint32_t stage_frame) {
  PIXEL_LTRB src;
  int i = 0;

  switch (state_) {
  case State::OpeningFrame:
  case State::Closing:
    for (i = 0; i < BOSS_HEALTH_GAUGE_HEIGHT; i++) {
      src = {0, (104 + i), BOSS_HEALTH_GAUGE_WIDTH, (104 + i + 1)};
      GrpSurface_Blit({row_x_[i], (16 + i)}, SURFACE_ID::SYSTEM, src);
    }
    break;

  case State::Filling:
  case State::Ready:
  case State::Refilling: {
    constexpr WINDOW_COORD left = (BOSS_HEALTH_GAUGE_END_X + 3);
    constexpr WINDOW_COORD top = (16 + 3);
    constexpr WINDOW_COORD bottom = (top + 11);
    const auto x1 = (left + ((target_hp_ * 30 * 8) / max_hp_));
    const auto x2 = (left + ((current_hp_ * 30 * 8) / max_hp_));
    constexpr uint8_t alpha = (128 + 64);
    constexpr RGB216 col = {0, 1, 5};

    GrpGeom->Lock();
    GrpGeom->SetAlphaNorm(alpha);
    if (auto *gp = GrpGeom_Poly()) {
      VERTEX_XY src_vertices[4] = {
          {0, top},
          {left, top},
          {left, bottom},
          {0, bottom},
      };
      if (x1 < x2) {
        src_vertices[0].x = src_vertices[3].x = x1;
        GeomGrdRectA(*gp, src_vertices, col.ToRGB().WithAlpha(alpha));
        gp->SetColor({5, 0, 0});
        gp->DrawBoxA(x1, top, x2, bottom);
      } else {
        src_vertices[0].x = src_vertices[3].x = x2;
        GeomGrdRectA(*gp, src_vertices, col.ToRGB().WithAlpha(alpha));
      }
    } else if (auto *gf = GrpGeom_FB()) {
      constexpr auto line_top = (top + 5);
      constexpr auto line_bottom = (bottom - 4);
      gf->SetColor(col);
      if (x1 < x2) {
        gf->DrawBoxA(left, top, x2, line_top);
        gf->DrawBoxA(left, line_bottom, x2, bottom);
        gf->SetColor({5, 5, 5});
        gf->DrawBoxA(left, line_top, x1, line_bottom);
        gf->SetColor({5, 0, 0});
        gf->DrawBoxA(x1, top, x2, bottom);
      } else {
        gf->DrawBoxA(left, top, x2, line_top);
        gf->DrawBoxA(left, line_bottom, x2, bottom);
        gf->SetColor({5, 5, 5});
        gf->DrawBoxA(left, line_top, x2, line_bottom);
      }
    }

    GrpGeom->Unlock();

    src = {0, 104, BOSS_HEALTH_GAUGE_WIDTH, 128};
    GrpSurface_Blit({BOSS_HEALTH_GAUGE_END_X, 16}, SURFACE_ID::SYSTEM, src);

    if (phase_threshold_hp_ > 0 && max_hp_ > 0) {
      const auto separator_x =
          left +
          static_cast<int32_t>(
              (static_cast<uint64_t>(phase_threshold_hp_) * 30 * 8) / max_hp_);
      if (separator_x > left && separator_x < (left + 30 * 8)) {
        GrpGeom->Lock();
        GrpGeom->SetAlphaNorm(224);
        GrpGeom->SetColor({5, 5, 5});
        GrpGeom->DrawBoxA(separator_x, top, (separator_x + 3), bottom);
        GrpGeom->Unlock();
      }
    }

    if (timer_max_ > 0) {
      const int remain = std::min((timer_max_ - timer_now_) / 60, 99);
      if (remain >= 0) {
        if (remain <= 10 && remain != previous_timer_seconds_) {
          Snd_SEPlay(SfxId::Sblaser);
        }
        previous_timer_seconds_ = remain;
        if (remain < 10) {
          GrpSurface_SetColorMod(SURFACE_ID::SYSTEM, 255, 64, 64);
        }
        GrpPut16(476, 0, std::format("{:>2}", remain).c_str());
        if (remain < 10) {
          GrpSurface_SetColorMod(SURFACE_ID::SYSTEM, 255, 255, 255);
        }
      }
    } else if (stage_timeout_end_ > 0) {
      const int remain = std::min(
          (stage_timeout_end_ - static_cast<int32_t>(stage_frame)) / 60, 99);
      if (remain >= 0) {
        if (remain <= 10 && remain != previous_timer_seconds_) {
          Snd_SEPlay(SfxId::Sblaser);
        }
        previous_timer_seconds_ = remain;
        if (remain < 10) {
          GrpSurface_SetColorMod(SURFACE_ID::SYSTEM, 255, 64, 64);
        }
        GrpPut16(476, 0, std::format("{:>2}", remain).c_str());
        if (remain < 10) {
          GrpSurface_SetColorMod(SURFACE_ID::SYSTEM, 255, 255, 255);
        }
      }
    }
  } break;

  case State::Hidden:
    break;
  }
}
