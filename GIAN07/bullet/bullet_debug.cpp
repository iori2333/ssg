///
/// BulletDebug - Debug bullet hitbox overlay rendering
///
#include <array>
#include <cmath>
#include <span>

#include "bullet.h"
#include "bullet_debug.h"
#include "bullet_manager.h"
#include "laser/homing.h"
#include "laser/reflect.h"
#include "laser/long.h"
#include "laser_manager.h"

#include "core/gian.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"

namespace {

inline constexpr auto kLaserEvadeWidth = 12 * 64;
inline constexpr auto kLongLaserEvadeWidth = 15 * 64;
inline constexpr auto kHomingLaserWidth = 8 * 64;
constexpr int HlGetPrev(int current, int n) {
  return (current + n) % (kHomingLen * kHomingSection);
}

void DrawQuadFilled(GRAPHICS_GEOMETRY_SDL &gp,
                    std::span<const VERTEX_XY, 4> p) {
  const std::array<VERTEX_XY, 4> strip = {p[0], p[3], p[1], p[2]};
  gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, strip);
}

} // namespace

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
  for (const auto &lp : Lasers.reflect) {
    if (lp.State() == ReflectState::Dead || lp.State() == ReflectState::Clearing) {
      continue;
    }

    DrawQuadFilled(*gp, lp.P());

    if (mode >= 2 && lp.W() > 0) {
      const int bx = lp.X() >> 6;
      const int by = lp.Y() >> 6;
      const int scale = (lp.W() + kLaserEvadeWidth);
      const int wx2 = lp.WX() * scale / lp.W();
      const int wy2 = lp.WY() * scale / lp.W();
      VERTEX_XY ep[4];
      ep[1].x = ep[0].x = static_cast<float>(bx + wx2);
      ep[1].y = ep[0].y = static_cast<float>(by + wy2);
      ep[2].x = ep[3].x = static_cast<float>(bx - wx2);
      ep[2].y = ep[3].y = static_cast<float>(by - wy2);
      ep[1].x += static_cast<float>(lp.LX());
      ep[1].y += static_cast<float>(lp.LY());
      ep[2].x += static_cast<float>(lp.LX());
      ep[2].y += static_cast<float>(lp.LY());
      DrawQuadFilled(*gp, ep);
    }
  }

  // --- Long lasers ---
  for (const auto &ll : Lasers.long_lasers) {
    if (ll.State() != LongState::Active && ll.State() != LongState::Opening) {
      continue;
    }

    DrawQuadFilled(*gp, ll.P());

    if (mode >= 2 && ll.W() > 0) {
      const int bx = ll.X() >> 6;
      const int by = ll.Y() >> 6;
      const int scale = (ll.W() + kLongLaserEvadeWidth);
      const int wx2 = ll.WX() * scale / ll.W();
      const int wy2 = ll.WY() * scale / ll.W();
      const int lx2 = ll.LX() * scale / ll.W();
      const int ly2 = ll.LY() * scale / ll.W();
      VERTEX_XY ep[4];
      ep[1].x = ep[0].x = static_cast<float>(bx + wx2 + lx2);
      ep[1].y = ep[0].y = static_cast<float>(by + wy2 + ly2);
      ep[2].x = ep[3].x = static_cast<float>(bx - wx2 + lx2);
      ep[2].y = ep[3].y = static_cast<float>(by - wy2 + ly2);
      ep[1].x += static_cast<float>(ll.InfX());
      ep[1].y += static_cast<float>(ll.InfY());
      ep[2].x += static_cast<float>(ll.InfX());
      ep[2].y += static_cast<float>(ll.InfY());
      DrawQuadFilled(*gp, ep);
    }
  }

  // --- Homing lasers ---
  for (const auto &hl : Lasers.homing) {
    if (hl.State() == HomingState::Dead) {
      continue;
    }

    const int hit_r = (kHomingLaserWidth * 2 / 3) >> 6;
    const int evade_r = (kHomingLaserWidth + 15 * 64) >> 6;

    int current = hl.Current();
    for (int j = 0; j < kHomingLen; j++) {
      const auto &pt = hl.Segments()[current];
      const int cx = pt.x >> 6;
      const int cy = pt.y >> 6;

      if (mode >= 2) {
        Geometry::CircleF_Approximated(*gp, {cx, cy}, evade_r, true);
      }

      Geometry::CircleF_Approximated(*gp, {cx, cy}, hit_r, true);

      current = HlGetPrev(current, kHomingSection);
    }
  }

  // --- Player hitbox ---
  const int px = Players.X() >> 6;
  const int py = Players.Y() >> 6;
  const int pr = std::ceil(PLAYER_HITBOX_RADIUS / 64.0);

  Geometry::CircleF_Approximated(*gp, {px, py}, pr, true);
}
