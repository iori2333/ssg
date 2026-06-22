///
/// BulletDebug - Debug bullet hitbox overlay rendering
///
#include <array>
#include <cmath>
#include <span>

#include "bullet.h"
#include "bullet_debug.h"
#include "bullet_manager.h"
#include "homing_laser.h"
#include "laser.h"
#include "laser_manager.h"
#include "long_laser.h"

#include "core/gian.h"
#include "effect/geometry.h"
#include "gfx/graphics_sdl.h"

namespace {

inline constexpr auto kLaserEvadeWidth = 12 * 64;
inline constexpr auto kLongLaserEvadeWidth = 15 * 64;
inline constexpr auto kHomingLaserWidth = 8 * 64;
inline constexpr auto kLfClear = 0x01;

constexpr int HlGetPrev(int current, int n) {
  return (current + n) % (HLASER_LEN * HLASER_SECTION);
}

void DrawQuadFilled(GRAPHICS_GEOMETRY_SDL &gp,
                     std::span<const VERTEX_XY, 4> p) {
  const std::array<VERTEX_XY, 4> strip = {p[0], p[3], p[1], p[2]};
  gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, strip);
}

}  // namespace

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

  // --- Short / reflect lasers ---
  for (uint16_t i = 0; i < Lasers.count; i++) {
    const auto *lp = &Lasers.lasers[Lasers.laser_indices[i]];
    if ((lp->flag & (LF_DELETE | kLfClear)) != 0) {
      continue;
    }

    DrawQuadFilled(*gp, lp->p);

    if (mode >= 2 && lp->w > 0) {
      const int bx = lp->x >> 6;
      const int by = lp->y >> 6;
      const int scale = (lp->w + kLaserEvadeWidth);
      const int wx2 = lp->wx * scale / lp->w;
      const int wy2 = lp->wy * scale / lp->w;
      VERTEX_XY ep[4];
      ep[1].x = ep[0].x = static_cast<float>(bx + wx2);
      ep[1].y = ep[0].y = static_cast<float>(by + wy2);
      ep[2].x = ep[3].x = static_cast<float>(bx - wx2);
      ep[2].y = ep[3].y = static_cast<float>(by - wy2);
      ep[1].x += static_cast<float>(lp->lx);
      ep[1].y += static_cast<float>(lp->ly);
      ep[2].x += static_cast<float>(lp->lx);
      ep[2].y += static_cast<float>(lp->ly);
      DrawQuadFilled(*gp, ep);
    }
  }

  // --- Long lasers ---
  for (const auto &ll : Lasers.long_lasers) {
    if (ll.flag != LLF_NORM && ll.flag != LLF_OPEN) {
      continue;
    }

    DrawQuadFilled(*gp, ll.p);

    if (mode >= 2 && ll.w > 0) {
      const int bx = ll.x >> 6;
      const int by = ll.y >> 6;
      const int scale = (ll.w + kLongLaserEvadeWidth);
      const int wx2 = ll.wx * scale / ll.w;
      const int wy2 = ll.wy * scale / ll.w;
      const int lx2 = ll.lx * scale / ll.w;
      const int ly2 = ll.ly * scale / ll.w;
      VERTEX_XY ep[4];
      ep[1].x = ep[0].x = static_cast<float>(bx + wx2 + lx2);
      ep[1].y = ep[0].y = static_cast<float>(by + wy2 + ly2);
      ep[2].x = ep[3].x = static_cast<float>(bx - wx2 + lx2);
      ep[2].y = ep[3].y = static_cast<float>(by - wy2 + ly2);
      ep[1].x += static_cast<float>(ll.infx);
      ep[1].y += static_cast<float>(ll.infy);
      ep[2].x += static_cast<float>(ll.infx);
      ep[2].y += static_cast<float>(ll.infy);
      DrawQuadFilled(*gp, ep);
    }
  }

  // --- Homing lasers ---
  for (auto *hl = Lasers.active.Next; hl != nullptr; hl = hl->Next) {
    if (hl->State == HLS_DEAD) {
      continue;
    }

    const int hit_r = (kHomingLaserWidth * 2 / 3) >> 6;
    const int evade_r = (kHomingLaserWidth + 15 * 64) >> 6;

    int current = hl->Current;
    for (int j = 0; j < HLASER_LEN; j++) {
      const auto &pt = hl->p[current];
      const int cx = pt.x >> 6;
      const int cy = pt.y >> 6;

      if (mode >= 2) {
        Geometry::CircleF_Approximated(*gp, {cx, cy}, evade_r, true);
      }

      Geometry::CircleF_Approximated(*gp, {cx, cy}, hit_r, true);

      current = HlGetPrev(current, HLASER_SECTION);
    }
  }
}
