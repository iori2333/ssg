///
/// LongLaserSubsystem - implementation (migrated from long_laser.cpp).
///

#include "long_laser.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "effect/geometry.h"
#include "enemy/enemy.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/ut_math.h"

namespace bullets {

LongLaserSubsystem::LongLaserSubsystem(world::Refs w) : world_(w) {}

bool LongLaserSubsystem::SpawnLongLaser(const LongLaserCommand &cmd,
                                        uint8_t id) {
  auto lp = std::ranges::find_if(
      long_lasers_, [](const auto &l) { return (l.flag == LLF_DISABLE); });
  if (lp == std::end(long_lasers_))
    return false;

  lp->dx = cmd.dx;
  lp->dy = cmd.dy;
  lp->e = cmd.e;
  lp->EnemyID = id;
  lp->x = lp->e->x + lp->dx;
  lp->y = lp->e->y + lp->dy;
  lp->v = cmd.v;
  lp->c = cmd.c;
  lp->lx = lp->ly = lp->wx = lp->wy = 0;
  lp->w = 0;
  lp->wmax = cmd.w;
  lp->d = cmd.d;

  if (cmd.type == LLS_LONGZ) {
    lp->d += atan8(world_.players.X() - lp->x, world_.players.Y() - lp->y);
    lp->type = LLS_LONG;
  } else {
    lp->type = cmd.type;
  }
  lp->infx = cosl(lp->d, 800);
  lp->infy = sinl(lp->d, 800);
  lp->count = 0;
  SetLongPoint(&*lp);
  lp->flag = LLF_LINE;
  return true;
}

void LongLaserSubsystem::OpenLong(const EnemyData *e, uint8_t id) {
  for (auto &it : long_lasers_) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL) &&
        lp->flag != LLF_DISABLE) {
      lp->flag = LLF_OPEN;
      Snd_SEPlay(2, lp->x, true);
    }
  }
}

void LongLaserSubsystem::CloseLong(const EnemyData *e, uint8_t id) {
  if (id == ECLCST_LLASERALL) {
    ForceCloseLong(e);
    return;
  }
  for (auto &it : long_lasers_) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id)) {
      lp->flag = LLF_CLOSE;
      Snd_SEStop(2);
    }
  }
}

void LongLaserSubsystem::LineLong(const EnemyData *e, uint8_t id) {
  for (auto &it : long_lasers_) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->flag = LLF_CLOSEL;
      Snd_SEStop(2);
    }
  }
}

void LongLaserSubsystem::UpdateLongXY(int id) {
  long_lasers_[id].x = long_lasers_[id].e->x + long_lasers_[id].dx;
  long_lasers_[id].y = long_lasers_[id].e->y + long_lasers_[id].dy;
  SetLongPoint(&long_lasers_[id]);
}

void LongLaserSubsystem::RotateLongAbs(const EnemyData *e, uint8_t d,
                                       uint8_t id) {
  for (auto &it : long_lasers_) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->d = d;
      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;
      lp->infx = cosl(lp->d, 800);
      lp->infy = sinl(lp->d, 800);
      SetLongPoint(lp);
    }
  }
}

void LongLaserSubsystem::RotateLongRel(const EnemyData *e, char d, uint8_t id) {
  for (auto &it : long_lasers_) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->d += d;
      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;
      lp->infx = cosl(lp->d, 800);
      lp->infy = sinl(lp->d, 800);
      SetLongPoint(lp);
    }
  }
}

void LongLaserSubsystem::ForceCloseLong(const EnemyData *e) {
  for (auto &it : long_lasers_) {
    auto *lp = &it;
    if (lp->e == e) {
      lp->flag = LLF_CLOSE;
      Snd_SEStop(2);
    }
  }
}

void LongLaserSubsystem::MoveLong() {
  for (int i = 0; i < LLASER_MAX; ++i) {
    auto *lp = &long_lasers_[i];

    if (lp->type == LLS_SETDEG && (lp->e != nullptr) && lp->d != lp->e->d) {
      lp->d = lp->e->d;
      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;
      lp->infx = cosl(lp->d, 800);
      lp->infy = sinl(lp->d, 800);
      SetLongPoint(lp);
    }

    switch (lp->flag) {
    case LLF_OPEN:
      UpdateLongXY(i);
      lp->w += lp->v;
      if ((lp->w) >= (lp->wmax)) {
        lp->w = lp->wmax;
        lp->flag = LLF_NORM;
      }
      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;
      SetLongPoint(lp);
      HitCheckLong(lp, world_);
      break;
    case LLF_CLOSE:
    case LLF_CLOSEL:
      UpdateLongXY(i);
      lp->w -= lp->v;
      if ((lp->w) <= 0) {
        lp->w = 0;
        if (lp->flag == LLF_CLOSE) {
          lp->flag = LLF_DISABLE;
          lp->e = nullptr;
        } else {
          lp->flag = LLF_LINE;
        }
      }
      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;
      SetLongPoint(lp);
      break;
    case LLF_LINE:
      UpdateLongXY(i);
      break;
    case LLF_NORM:
      UpdateLongXY(i);
      HitCheckLong(lp, world_);
      break;
    case LLF_DISABLE:
      break;
    }
  }
}

void LongLaserSubsystem::DrawLong() {
  static const RGB216 Table16Bit[16] = {
      {3, 0, 3}, {0, 2, 0}, {0, 0, 4}, {4, 2, 0}, {0, 0, 1}};
  static const RGB216 Table8BitA[16] = {
      {2, 0, 2}, {0, 2, 0}, {0, 1, 3}, {4, 2, 0}, {0, 0, 1}};
  static const RGB216 Table8BitB[16] = {
      {3, 0, 3}, {0, 4, 0}, {0, 1, 5}, {5, 3, 0}, {2, 2, 4}};
  static const RGB216 Table8BitC[16] = {
      {5, 4, 5}, {5, 5, 5}, {4, 4, 5}, {5, 5, 4}, {4, 4, 5}};

  constexpr size_t VERTEX_COUNT = 34;
  std::array<VERTEX_XY, VERTEX_COUNT> p2{};
  std::array<VERTEX_RGBA, VERTEX_COUNT> vcs{};

  int x = 0, y = 0, wx = 0, wy = 0, len = 0;
  VERTEX_XY p[4];

  // Original GameDraw calls Lasers.DrawLong() once when FB is present,
  // then again when Poly is present.  DrawLong() itself iterates lasers
  // and gates each draw pass on the active backend here.
  GrpGeom->Lock();

  for (const auto &it : long_lasers_) {
    const auto *lp = &it;
    const auto c = lp->c;
    switch (lp->flag) {
    case LLF_OPEN:
    case LLF_NORM:
    case LLF_CLOSE:
    case LLF_CLOSEL:
      x = ((lp->x) >> 6) + lp->lx;
      y = ((lp->y) >> 6) + lp->ly;
      wx = lp->wx;
      wy = lp->wy;
      len = isqrt((wx * wx) + (wy * wy));

      if (len != 0) {
        if (auto *gp = GrpGeom_Poly()) {
          const RGBA col = Table16Bit[c].ToRGB().WithAlpha(0xFF);
          gp->SetAlphaOne();
          GeomGrdRectA(*gp, lp->p, col);

          vcs[0] = {255, 255, 255, 0xFF};
          for (auto &vc : vcs | std::views::drop(1))
            vc = col;

          p2[0].x = x;
          p2[0].y = y;
          p2[1].x = lp->p[0].x;
          p2[1].y = lp->p[0].y;
          p2[VERTEX_COUNT - 1].x = lp->p[3].x;
          p2[VERTEX_COUNT - 1].y = lp->p[3].y;
          for (auto n = 2; n < (VERTEX_COUNT - 1); ++n) {
            p2[n].x = p2[0].x + cosl(lp->d + 64 + (128 * (n - 1) / 32), len);
            p2[n].y = p2[0].y + sinl(lp->d + 64 + (128 * (n - 1) / 32), len);
          }
          gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p2, vcs);
          break;
        }
        if (auto *gf = GrpGeom_FB()) {
          gf->SetColor(Table8BitA[c]);
          gf->DrawTriangleFan(lp->p);
        }
      } else if (GrpGeom_Poly() != nullptr) {
        break;
      }

      GeomCircleF({x, y}, len);
      GrpGeom->SetColor(Table8BitB[c]);
      if (len != 0) {
        p[0].x = p[1].x = lp->p[0].x - (wx / 8);
        p[0].y = p[1].y = lp->p[0].y - (wy / 8);
        p[3].x = p[2].x = lp->p[3].x + (wx / 8);
        p[3].y = p[2].y = lp->p[3].y + (wy / 8);
        p[1].x += lp->infx;
        p[1].y += lp->infy;
        p[2].x += lp->infx;
        p[2].y += lp->infy;
        GrpGeom->DrawTriangleFan(p);
      }
      GeomCircleF({x, y}, (len - (len / 8)));

      GrpGeom->SetColor(Table8BitC[c]);
      if (len != 0) {
        p[0].x = p[1].x = lp->p[0].x - (wx / 4);
        p[0].y = p[1].y = lp->p[0].y - (wy / 4);
        p[3].x = p[2].x = lp->p[3].x + (wx / 4);
        p[3].y = p[2].y = lp->p[3].y + (wy / 4);
        p[1].x += lp->infx;
        p[1].y += lp->infy;
        p[2].x += lp->infx;
        p[2].y += lp->infy;
        GrpGeom->DrawTriangleFan(p);
      }
      GeomCircleF({x, y}, (len - (len / 4)));
      break;

    case LLF_LINE:
      x = (lp->x) >> 6;
      y = (lp->y) >> 6;
      GrpGeom->SetColor({4, 4, 4});
      GrpGeom->DrawLine(x, y, (x + lp->infx), (y + lp->infy));
      break;
    case LLF_DISABLE:
      break;
    }
  }

  GrpGeom->Unlock();
}

void LongLaserSubsystem::ClearLong() {
  for (auto &it : long_lasers_) {
    if (it.flag != LLF_DISABLE)
      it.flag = LLF_CLOSE;
  }
  Snd_SEStop(2);
}

void LongLaserSubsystem::Setup() {
  for (auto &it : long_lasers_) {
    it.flag = LLF_DISABLE;
    it.e = nullptr;
  }
  Snd_SEStop(2);
}

void LongLaserSubsystem::SetLongPoint(LongLaserData *lp) {
  auto *pp = lp->p;
  pp[1].x = pp[0].x = (lp->x >> 6) + lp->wx + lp->lx;
  pp[1].y = pp[0].y = (lp->y >> 6) + lp->wy + lp->ly;
  pp[2].x = pp[3].x = (lp->x >> 6) - lp->wx + lp->lx;
  pp[2].y = pp[3].y = (lp->y >> 6) - lp->wy + lp->ly;
  pp[1].x += lp->infx;
  pp[1].y += lp->infy;
  pp[2].x += lp->infx;
  pp[2].y += lp->infy;
}

void LongLaserSubsystem::HitCheckLong(const LongLaserData *lp, world::Refs w) {
  if (w.players.IsInvincible() != 0U)
    return;

  int tx = w.players.X() - lp->x;
  int ty = w.players.Y() - lp->y;
  int length = cosl(lp->d, tx) + sinl(lp->d, ty);
  int width = abs(-sinl(lp->d, tx) + cosl(lp->d, ty));

  if (length > 0 && width <= (lp->w + (64 * 15)))
    w.players.AddEvade(LLASER_EVADE);
  if (length > 0 && width <= (lp->w))
    w.players.OnHit();
}

} // namespace bullets