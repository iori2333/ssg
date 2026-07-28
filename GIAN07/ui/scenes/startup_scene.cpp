/// Application startup scene.

#include <cstdint>

#include "startup_scene.h"

#include "data/graphics_loader.h"
#include "effect/lens.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "util/ut_math.h"

bool StartupScene::Enter() {
  GrpBackend_PixelAccessStart();
  if (!graphics_.LoadProjectScreen()) {
    return false;
  }
  lens_ = GrpCreateLensBall(70, 36);
  timer_ = 0;
  return lens_.has_value();
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
