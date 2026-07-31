///
/// GameplayHud - Player status and game telemetry presentation.
///

#include <algorithm>
#include <format>
#include <utility>

#include "gameplay_hud.h"

#include "gameplay/playfield.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/time.h"

void GameplayHud::DrawTop(const GameplayHudModel &model) const {
  constexpr PIXEL_LTRB graze_frame = {0, 80, 128, 104};

  GrpGeom->SetColor({0, 0, 0});
  GrpGeom->SetAlphaNorm(128);
  GrpGeom->DrawBoxA(playfield::kLeft, playfield::kTop, playfield::kRight + 1,
                    40);

  if (model.graze_wait_time != 0U) {
    GrpGeom->SetColor({5, 1, 0});
    GrpGeom->SetAlphaOne();
    for (int i = 0; i <= 10; i++) {
      const int right = 128 + 9 + (model.graze_wait_time >> 2) + (5 - i);
      if (right > 128 + 8) {
        GrpGeom->DrawBoxA(128 + 8, 16 + 3 + i, right, 16 + 3 + i + 1);
      }
    }
  }

  GrpSurface_Blit({128, 16}, SURFACE_ID::SYSTEM, graze_frame);
  GrpPut57(128 + 95, 16 + 91 - 80,
           std::format("{:3}", model.graze_count).c_str());
  GrpPut16(128, 0, std::format("{:9}", model.score).c_str());

  constexpr PIXEL_LTWH life_icon = {448, 272, 16, 16};
  constexpr PIXEL_LTWH bomb_icon = {512, 272, 16, 16};
  constexpr PIXEL_LTWH star_icon = {624, 432, 16, 16};
  for (int i = 0; std::cmp_less(i, model.lives); i++) {
    GrpSurface_Blit({280 + i * 14, 0}, SURFACE_ID::SYSTEM, life_icon);
  }
  for (int i = 0; std::cmp_less(i, model.bombs); i++) {
    GrpSurface_Blit({280 + i * 14, 0}, SURFACE_ID::SYSTEM, bomb_icon);
  }
  GrpSurface_Blit({408, 0}, SURFACE_ID::SYSTEM, star_icon);
  GrpPut16(424, 0,
           std::format("{:<4}", std::min(model.star_counter, 9999U)).c_str());
}

void GameplayHud::DrawSidebars(const GameplayHudModel &model) {
  constexpr WINDOW_COORD left_column = 0;
  const WINDOW_COORD right_column = GRP_RES.w - 128;

  const auto now = util::SteadyTicksMs();
  if (now - fps_sample_start_ <= 1000) {
    frame_count_++;
  } else {
    fps_ = frame_count_;
    frame_count_ = 0;
    fps_sample_start_ = now;
  }

  GrpPut16(left_column, 0, std::format("FPS   {:3}", fps_).c_str());
  GrpPut16(left_column, 40, std::format("R {:7}", model.rank).c_str());
  GrpPut16(left_column, 60, std::format("L {:>7}", model.level_name).c_str());
  GrpPut16(left_column, 100,
           std::format("Miss {:4}", model.miss_count).c_str());
  GrpPut16(left_column, 120, std::format("Bomb {:4}", model.bomb_used).c_str());
  GrpPut16(left_column, 140,
           std::format("DthB {:4}", model.deathbomb_count).c_str());
  GrpPut16(left_column, 180, "Stars");
  GrpPut16(left_column, 200,
           std::format("{:4}/{:4}", std::min(model.star_counter, 9999U),
                       std::min(model.star_threshold, 9999U))
               .c_str());

  const auto local_time = util::LocalTime();
  GrpPut16(right_column, 0, "Date");
  GrpPut16(right_column, 20,
           std::format("{:02}/{:02}/{:02}", local_time.month, local_time.day,
                       local_time.year % 100U)
               .c_str());
  GrpPut16(right_column, 50, "Time");
  GrpPut16(right_column, 70,
           std::format("{:02}:{:02}:{:02}", local_time.hour, local_time.minute,
                       local_time.second)
               .c_str());

#ifdef PBG_DEBUG
  GrpPut16(right_column, 360, "Debug  ON");
#endif

  if (model.practice_mode == PracticeMode::AutoBomb) {
    GrpPut16(right_column, 380, "Prac AUTO");
  } else if (model.practice_mode == PracticeMode::Invincible) {
    GrpPut16(right_column, 380, "Prac  INV");
  }

  GrpPut16(right_column, 420, std::format("Bomb {:4}", model.bombs).c_str());
  GrpPut16(right_column, 440, std::format("Left {:4}", model.lives).c_str());
  GrpPut16(right_column, 460,
           std::format("Credit {:2}", model.credits).c_str());
}
