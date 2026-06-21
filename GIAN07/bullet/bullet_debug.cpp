///
/// BulletDebug - Debug bullet hitbox overlay rendering
///
#include <cmath>
#include <span>

#include "bullet.h"
#include "bullet_debug.h"
#include "bullet_manager.h"

#include "core/gian.h"
#include "effect/geometry.h"
#include "gfx/graphics_sdl.h"

void BulletDebug_DrawHitboxes(int mode) {
  const RGB216 kBlack{0, 0, 0};
  constexpr uint8_t kAlpha = 204;

  auto *gp = GrpGeom_Poly();
  if (gp == nullptr) {
    return;
  }
  gp->SetColor(kBlack);
  gp->SetAlphaNorm(kAlpha);

  for (const auto i : std::views::iota(0U, Bullets.count_small)) {
    auto *t = &Bullets.bullets[Bullets.indices_small[i]];
    if ((t->flag & TF_DELETE) != 0) {
      continue;
    }

    const int cx = (t->x >> 6);
    const int cy = (t->y >> 6);

    if (mode >= 2) {
      Geometry::CircleF_Approximated(*gp, {cx, cy},
                                     TAMA_EVADE_RADIUS_SMALL >> 6, true);
    }

    const int r_px = GetBulletHitRadius(t->c) >> 6;
    if (r_px > 0) {
      Geometry::CircleF_Approximated(*gp, {cx, cy}, r_px, true);
    }
  }

  for (const auto i : std::views::iota(0U, Bullets.count_large)) {
    auto *t = &Bullets.bullets[Bullets.indices_large[i]];
    if ((t->flag & TF_DELETE) != 0) {
      continue;
    }

    const int cx = (t->x >> 6);
    const int cy = (t->y >> 6);

    if (mode >= 2) {
      Geometry::CircleF_Approximated(*gp, {cx, cy},
                                     TAMA_EVADE_RADIUS_LARGE >> 6, true);
    }

    const int r_px = GetBulletHitRadius(t->c) >> 6;
    if (r_px > 0) {
      Geometry::CircleF_Approximated(*gp, {cx, cy}, r_px, true);
    }
  }
}
