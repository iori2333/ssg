///
/// GameplayHud - Player status and game telemetry presentation.
///

#include <algorithm>
#include <format>
#include <utility>

#include "gameplay_hud.h"

#include "gameplay/game_rules.h"
#include "gameplay/playfield.h"
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/graphics.h"
#include "gfx/render/geometry.h"
#include "ui/bitmap_font.h"
#include "util/time_api.h"

void GameplayHud::DrawTop(const GameplayHudModel &model) {
  constexpr Rect graze_frame = {0, 80, 128, 104};

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
  ui::DrawDigits5x7({128 + 95, 16 + 91 - 80},
                    std::format("{:3}", model.graze_count));
  ui::Draw16({128, 0}, std::format("{:9}", model.score));

  constexpr Rect life_icon = Rect::FromLtwh(448, 272, 16, 16);
  constexpr Rect bomb_icon = Rect::FromLtwh(512, 272, 16, 16);
  constexpr Rect star_icon = Rect::FromLtwh(624, 432, 16, 16);
  for (int i = 0; i < model.lives; i++) {
    GraphicsSurfaceBlit({280 + i * 14, 0}, SurfaceId::System, life_icon);
  }
  for (int i = 0; i < model.bombs; i++) {
    GraphicsSurfaceBlit({280 + i * 14, 0}, SurfaceId::System, bomb_icon);
  }
  GraphicsSurfaceBlit({408, 0}, SurfaceId::System, star_icon);
  ui::Draw16({424, 0},
             std::format("{:<4}", std::min(model.star_counter, 9999)));
}

void GameplayHud::DrawSidebars(const GameplayHudModel &model) {
  constexpr int left_column = 0;
  const int right_column = kGameResolution.x - 128;

  const auto now = util::SteadyTicksMs();
  if (now - fps_sample_start_ <= 1000) {
    frame_count_++;
  } else {
    fps_ = frame_count_;
    frame_count_ = 0;
    fps_sample_start_ = now;
  }

  ui::Draw16({left_column, 0}, std::format("FPS   {:3}", fps_));
  ui::Draw16({left_column, 40}, std::format("R {:7}", model.rank));
  ui::Draw16({left_column, 60}, std::format("L {:>7}", model.level_name));
  ui::Draw16({left_column, 100}, std::format("Miss {:4}", model.miss_count));
  ui::Draw16({left_column, 120}, std::format("Bomb {:4}", model.bomb_used));
  ui::Draw16({left_column, 140},
             std::format("DthB {:4}", model.deathbomb_count));
  ui::Draw16({left_column, 180}, "Stars");
  ui::Draw16({left_column, 200},
             std::format("{:4}/{:4}", std::min(model.star_counter, 9999),
                         std::min(model.star_threshold, 9999)));

  const auto local_time = util::LocalTime();
  ui::Draw16({right_column, 0}, "Date");
  ui::Draw16({right_column, 20}, std::format("{:%m/%d/%y}", local_time));
  ui::Draw16({right_column, 50}, "Time");
  ui::Draw16({right_column, 70}, std::format("{:%H:%M:%S}", local_time));

#ifdef PBG_DEBUG
  ui::Draw16({right_column, 360}, "Debug  ON");
#endif

  if (model.practice_mode == PracticeMode::AutoBomb) {
    ui::Draw16({right_column, 380}, "Prac AUTO");
  } else if (model.practice_mode == PracticeMode::Invincible) {
    ui::Draw16({right_column, 380}, "Prac  INV");
  }

  ui::Draw16({right_column, 420}, std::format("Bomb {:4}", model.bombs));
  ui::Draw16({right_column, 440}, std::format("Left {:4}", model.lives));
  ui::Draw16({right_column, 460}, std::format("Credit {:2}", model.credits));
}
