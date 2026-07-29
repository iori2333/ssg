/// Application startup scene.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "startup_scene.h"

#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "util/ut_math.h"

StartupScene::Lens StartupScene::Lens::Create(uint16_t radius, uint16_t bulge) {
  assert(radius > 0);
  assert(radius > bulge);

  const auto diameter = static_cast<uint16_t>(radius * 2);
  const auto area = static_cast<std::size_t>(diameter) * diameter;
  Lens lens{
      .radius = radius,
      .diameter = diameter,
      .table = std::vector<uint32_t>(area),
      .field_of_view = std::vector<std::byte>(area * sizeof(uint32_t)),
  };

  auto *table = lens.table.data();
  const auto radius_squared = Cast::up_sign<int32_t>(radius) * radius;
  const auto sphere_radius =
      isqrt(radius_squared - Cast::up_sign<int32_t>(bulge) * bulge);
  for (auto row = -Cast::up_sign<int32_t>(radius); std::cmp_less(row, radius);
       ++row) {
    auto half_width = sphere_radius * sphere_radius - row * row;
    int width = 0;
    if (half_width > 0) {
      half_width = isqrt(half_width);
      width = half_width * 2;
      *table++ = static_cast<uint32_t>(width);
      *table++ =
          static_cast<uint32_t>((radius - half_width) * sizeof(uint32_t));
    } else {
      half_width = 0;
      *table++ = 0;
      *table++ = 0;
    }

    while (width-- != 0) {
      const auto column = half_width - width;
      const auto depth = isqrt(radius_squared - column * column - row * row);
      const auto source_y = ((row * bulge) / depth) + radius;
      const auto source_x = ((column * bulge) / depth) + radius;
      *table++ = static_cast<uint32_t>(source_y * diameter + source_x);
    }
  }
  return lens;
}

void StartupScene::Lens::Draw(WINDOW_POINT center) {
  const WINDOW_COORD left = center.x - radius;
  const WINDOW_COORD top = center.y - radius;
  if (left < 0 || top < 0 || left + diameter > GRP_RES.w - 1 ||
      top + diameter > GRP_RES.h - 1) {
    return;
  }

  GrpBackend_PixelAccessEdit([&]<class Pixel>(std::byte *pixels,
                                              std::size_t pitch) {
    auto *captured = reinterpret_cast<Pixel *>(field_of_view.data());
    const std::span field{captured,
                          static_cast<std::size_t>(diameter) * diameter};
    auto *table_cursor = table.data();
    auto *destination = &pixels[top * pitch + left * sizeof(Pixel)];

    const auto *source = destination;
    for (auto row = field.begin(); row != field.end(); row += diameter) {
      std::copy_n(reinterpret_cast<const Pixel *>(source), diameter, row);
      source += pitch;
    }

    for (uint16_t row = 0; row < diameter; ++row) {
      auto width = *table_cursor++;
      auto *output = reinterpret_cast<Pixel *>(destination + *table_cursor++);
      while (width-- != 0U) {
        *output++ = field[*table_cursor++];
      }
      destination += pitch;
    }
  });
}

bool StartupScene::Enter() {
  GrpBackend_PixelAccessStart();
  if (!graphics_.LoadProjectScreen()) {
    return false;
  }
  lens_ = Lens::Create(70, 36);
  timer_ = 0;
  return true;
}

StartupSceneResult StartupScene::Update(bool should_draw) {
  constexpr PIXEL_SIZE logo_size = {.w = 320, .h = 42};
  constexpr WINDOW_LTRB logo = WINDOW_LTWH{
      (320 - (logo_size.w / 2)), (240 + 40), logo_size.w, logo_size.h};

  timer_++;
  if (timer_ >= 256) {
    lens_.reset();
    return StartupSceneResult::Complete;
  }
  if (!should_draw) {
    return StartupSceneResult::Running;
  }

  GrpBackend_Clear();
  constexpr PIXEL_LTRB source = {0, 0, logo_size.w, logo_size.h};
  GrpSurface_Blit({logo.left, logo.top}, SURFACE_ID::SPROJECT, source);

  const auto fade = [logo](uint8_t black_alpha) {
    if (auto *geometry = GrpGeom_Poly()) {
      geometry->Lock();
      geometry->SetAlphaNorm(black_alpha);
      geometry->SetColor({0, 0, 0});
      geometry->DrawBoxA(logo.left, logo.top, logo.right, logo.bottom);
      geometry->Unlock();
    }
  };

  if (timer_ < 64) {
    fade((255 - timer_) * 4);
  } else if (timer_ > 192) {
    fade(timer_ * 4);
  } else if (lens_) {
    const uint8_t offset = timer_ - 64;
    lens_->Draw({320 + sinl(offset - 64, 240), 295 + sinl(offset * 2, 20)});
  }
  Grp_Flip();
  return StartupSceneResult::Running;
}
