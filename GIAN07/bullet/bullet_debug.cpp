///
/// BulletDebug - Debug bullet hitbox overlay rendering
///
#include "bullet_debug.h"

#include "bullet.h"
#include "bullet_manager.h"

#include "core/gian.h"
#include "gfx/graphics_sdl.h"

void BulletDebug_DrawHitboxes(int mode) {
  constexpr uint8_t kAlpha = 204;

  GrpGeom->SetColor({0, 0, 0});
  GrpGeom->SetAlphaNorm(kAlpha);

  for (const auto i : std::views::iota(0U, Bullets.count_small)) {
    auto *t = &Bullets.bullets[Bullets.indices_small[i]];
    if ((t->flag & TF_DELETE) != 0) {
      continue;
    }

    const int cx = (t->x >> 6);
    const int cy = (t->y >> 6);

    if (mode >= 2) {
      const int gx1 = cx - (TAMA_EVX_SMALL >> 6);
      const int gy1 = cy - (TAMA_EVY_SMALL >> 6);
      const int gx2 = cx + (TAMA_EVX_SMALL >> 6);
      const int gy2 = cy + (TAMA_EVY_SMALL >> 6);
      GrpGeom->DrawBoxA(gx1, gy1, gx2 + 1, gy2 + 1);
    }

    const int hx1 = cx - (TAMA_HITX >> 6);
    const int hy1 = cy - (TAMA_HITY >> 6);
    const int hx2 = cx + (TAMA_HITX >> 6);
    const int hy2 = cy + (TAMA_HITY >> 6);
    GrpGeom->DrawBoxA(hx1, hy1, hx2 + 1, hy2 + 1);
  }

  for (const auto i : std::views::iota(0U, Bullets.count_large)) {
    auto *t = &Bullets.bullets[Bullets.indices_large[i]];
    if ((t->flag & TF_DELETE) != 0) {
      continue;
    }

    const int cx = (t->x >> 6);
    const int cy = (t->y >> 6);

    if (mode >= 2) {
      const int gx1 = cx - (TAMA_EVX_LARGE >> 6);
      const int gy1 = cy - (TAMA_EVY_LARGE >> 6);
      const int gx2 = cx + (TAMA_EVX_LARGE >> 6);
      const int gy2 = cy + (TAMA_EVY_LARGE >> 6);
      GrpGeom->DrawBoxA(gx1, gy1, gx2 + 1, gy2 + 1);
    }

    const int hx1 = cx - (TAMA_HITX >> 6);
    const int hy1 = cy - (TAMA_HITY >> 6);
    const int hx2 = cx + (TAMA_HITX >> 6);
    const int hy2 = cy + (TAMA_HITY >> 6);
    GrpGeom->DrawBoxA(hx1, hy1, hx2 + 1, hy2 + 1);
  }
}
