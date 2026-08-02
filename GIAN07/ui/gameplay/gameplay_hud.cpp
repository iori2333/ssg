///
/// GameplayHud - Player status and game telemetry presentation.
///

#include <algorithm>
#include <format>
#include <utility>

#include "gameplay_hud.h"

#include "gameplay/game_rules.h"
#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/time_api.h"

void GameplayHud::DrawTop(const GameplayHudModel &model) {
  constexpr PixelLtrb graze_frame = {0, 80, 128, 104};

  geometry::SetColor({0, 0, 0});
  geometry::SetAlphaNorm(128);
  geometry::DrawBoxA(playfield::kLeft, playfield::kTop, playfield::kRight + 1,
                     40);

  if (model.graze_wait_time != 0) {
    geometry::SetColor({5, 1, 0});
    geometry::SetAlphaOne();
    for (int i = 0; i <= 10; i++) {
      const int right = 128 + 9 + (model.graze_wait_time >> 2) + (5 - i);
      if (right > 128 + 8) {
        geometry::DrawBoxA(128 + 8, 16 + 3 + i, right, 16 + 3 + i + 1);
      }
    }
  }

  GraphicsSurfaceBlit({128, 16}, SurfaceId::System, graze_frame);
  DrawFont57(128 + 95, 16 + 91 - 80,
             std::format("{:3}", model.graze_count).c_str());
  DrawFont16(128, 0, std::format("{:9}", model.score).c_str());

  constexpr PixelLtwh life_icon = {448, 272, 16, 16};
  constexpr PixelLtwh bomb_icon = {512, 272, 16, 16};
  constexpr PixelLtwh star_icon = {624, 432, 16, 16};
  for (int i = 0; i < model.lives; i++) {
    GraphicsSurfaceBlit({280 + i * 14, 0}, SurfaceId::System, life_icon);
  }
  for (int i = 0; i < model.bombs; i++) {
    GraphicsSurfaceBlit({280 + i * 14, 0}, SurfaceId::System, bomb_icon);
  }
  GraphicsSurfaceBlit({408, 0}, SurfaceId::System, star_icon);
  DrawFont16(424, 0,
             std::format("{:<4}", std::min(model.star_counter, 9999)).c_str());
}

void GameplayHud::DrawSidebars(const GameplayHudModel &model) {
  constexpr WindowCoord left_column = 0;
  const WindowCoord right_column = kGameResolution.w - 128;

  const auto now = util::SteadyTicksMs();
  if (now - fps_sample_start_ <= 1000) {
    frame_count_++;
  } else {
    fps_ = frame_count_;
    frame_count_ = 0;
    fps_sample_start_ = now;
  }

  DrawFont16(left_column, 0, std::format("FPS   {:3}", fps_).c_str());
  DrawFont16(left_column, 40, std::format("R {:7}", model.rank).c_str());
  DrawFont16(left_column, 60, std::format("L {:>7}", model.level_name).c_str());
  DrawFont16(left_column, 100,
             std::format("Miss {:4}", model.miss_count).c_str());
  DrawFont16(left_column, 120,
             std::format("Bomb {:4}", model.bomb_used).c_str());
  DrawFont16(left_column, 140,
             std::format("DthB {:4}", model.deathbomb_count).c_str());
  DrawFont16(left_column, 180, "Stars");
  DrawFont16(left_column, 200,
             std::format("{:4}/{:4}", std::min(model.star_counter, 9999),
                         std::min(model.star_threshold, 9999))
                 .c_str());

  const auto local_time = util::LocalTime();
  DrawFont16(right_column, 0, "Date");
  DrawFont16(right_column, 20,
             std::format("{:02}/{:02}/{:02}", local_time.month, local_time.day,
                         local_time.year % 100)
                 .c_str());
  DrawFont16(right_column, 50, "Time");
  DrawFont16(right_column, 70,
             std::format("{:02}:{:02}:{:02}", local_time.hour,
                         local_time.minute, local_time.second)
                 .c_str());

#ifdef PBG_DEBUG
  DrawFont16(right_column, 360, "Debug  ON");
#endif

  if (model.practice_mode == PracticeMode::AutoBomb) {
    DrawFont16(right_column, 380, "Prac AUTO");
  } else if (model.practice_mode == PracticeMode::Invincible) {
    DrawFont16(right_column, 380, "Prac  INV");
  }

  DrawFont16(right_column, 420, std::format("Bomb {:4}", model.bombs).c_str());
  DrawFont16(right_column, 440, std::format("Left {:4}", model.lives).c_str());
  DrawFont16(right_column, 460,
             std::format("Credit {:2}", model.credits).c_str());
}
