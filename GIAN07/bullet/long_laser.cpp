///
/// LongLaser - Long laser processing
///

#include "long_laser.h"
#include "laser_manager.h"

#include "audio/snd.h"
#include "gfx/graphics_backend.h"
#include "gfx/geometry.h"
#include "player/player.h"
#include "util/ut_math.h"

// ── helper ─────────────────────────────────────────────────────

void LaserManager::SetLongPoint(LaserLong *lp) {
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

// ── LaserLong virtual overrides ─────────────────────────────

void LaserLong::Move() {
  // Angle tracking
  if (type == LLS_SETDEG && e != nullptr && d != e->d) {
    d = e->d;

    lx = cosl(d, w >> 6);
    ly = sinl(d, w >> 6);

    wx = -(ly);
    wy = lx;

    infx = cosl(d, 800);
    infy = sinl(d, 800);

    Lasers.SetLongPoint(this);
  }

  if (flag == LLF_DISABLE) { return; }

  // Position sync
  x = e->x + dx;
  y = e->y + dy;
  Lasers.SetLongPoint(this);

  switch (flag) {
  case LLF_OPEN:
    w += v;
    if (w >= wmax) {
      w = wmax;
      flag = LLF_NORM;
    }

    lx = cosl(d, w >> 6);
    ly = sinl(d, w >> 6);
    wx = -(ly);
    wy = lx;

    Lasers.SetLongPoint(this);
    break;

  case LLF_CLOSE:
  case LLF_CLOSEL:
    w -= v;
    if (w <= 0) {
      w = 0;
      if (flag == LLF_CLOSE) {
        flag = LLF_DISABLE;
        e = nullptr;
      } else {
        flag = LLF_LINE;
      }
    }

    lx = cosl(d, w >> 6);
    ly = sinl(d, w >> 6);
    wx = -(ly);
    wy = lx;

    Lasers.SetLongPoint(this);
    break;

  case LLF_LINE:
    break;

  case LLF_NORM:
    break;

  default:
    break;
  }
}

void LaserLong::Draw() const {
  const auto cval = c;
  int wx2 = wx;
  int wy2 = wy;

  static const RGB216 Table16Bit[16] = {
      {3, 0, 3}, {0, 2, 0}, {0, 0, 4}, {4, 2, 0}, {0, 0, 1}};
  static const RGB216 Table8BitA[16] = {
      {2, 0, 2}, {0, 2, 0}, {0, 1, 3}, {4, 2, 0}, {0, 0, 1}};
  static const RGB216 Table8BitB[16] = {
      {3, 0, 3}, {0, 4, 0}, {0, 1, 5}, {5, 3, 0}, {2, 2, 4}};
  static const RGB216 Table8BitC[16] = {
      {5, 4, 5}, {5, 5, 5}, {4, 4, 5}, {5, 5, 4}, {4, 4, 5}};

  constexpr size_t VERTEX_COUNT = 34;

  switch (flag) {
  case LLF_OPEN:
  case LLF_NORM:
  case LLF_CLOSE:
  case LLF_CLOSEL: {
    const int px = (x >> 6) + lx;
    const int py = (y >> 6) + ly;
    const int len = isqrt((wx2 * wx2) + (wy2 * wy2));

    if (len != 0) {
      if (auto *gp = GrpGeom_Poly()) {
        const RGBA col = Table16Bit[cval].ToRGB().WithAlpha(0xFF);
        gp->SetAlphaOne();
        GeomGrdRectA(*gp, p, col);

        std::array<VERTEX_RGBA, VERTEX_COUNT> vcs{};
        vcs[0] = {255, 255, 255, 0xFF};
        for (auto &vc : vcs | std::views::drop(1)) {
          vc = col;
        }

        std::array<VERTEX_XY, VERTEX_COUNT> p2{};
        p2[0].x = px;
        p2[0].y = py;
        p2[1].x = p[0].x;
        p2[1].y = p[0].y;
        p2[VERTEX_COUNT - 1].x = p[3].x;
        p2[VERTEX_COUNT - 1].y = p[3].y;
        for (auto n = 2; n < (VERTEX_COUNT - 1); n++) {
          p2[n].x = p2[0].x + cosl(d + 64 + (128 * (n - 1) / 32), len);
          p2[n].y = p2[0].y + sinl(d + 64 + (128 * (n - 1) / 32), len);
        }
        gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p2, vcs);
        break;
      } else if (auto *gf = GrpGeom_FB()) {
        gf->SetColor(Table8BitA[cval]);
        gf->DrawTriangleFan(p);
      }
    } else if (GrpGeom_Poly() != nullptr) {
      break;
    }

    GeomCircleF({px, py}, len);

    GrpGeom->SetColor(Table8BitB[cval]);
    if (len != 0) {
      VERTEX_XY inner[4];
      inner[0].x = inner[1].x = p[0].x - (wx2 / 8);
      inner[0].y = inner[1].y = p[0].y - (wy2 / 8);
      inner[3].x = inner[2].x = p[3].x + (wx2 / 8);
      inner[3].y = inner[2].y = p[3].y + (wy2 / 8);
      inner[1].x += infx;
      inner[1].y += infy;
      inner[2].x += infx;
      inner[2].y += infy;
      GrpGeom->DrawTriangleFan(inner);
    }
    GeomCircleF({px, py}, (len - (len / 8)));

    GrpGeom->SetColor(Table8BitC[cval]);
    if (len != 0) {
      VERTEX_XY inner[4];
      inner[0].x = inner[1].x = p[0].x - (wx2 / 4);
      inner[0].y = inner[1].y = p[0].y - (wy2 / 4);
      inner[3].x = inner[2].x = p[3].x + (wx2 / 4);
      inner[3].y = inner[2].y = p[3].y + (wy2 / 4);
      inner[1].x += infx;
      inner[1].y += infy;
      inner[2].x += infx;
      inner[2].y += infy;
      GrpGeom->DrawTriangleFan(inner);
    }
    GeomCircleF({px, py}, (len - (len / 4)));
    break;
  }

  case LLF_LINE: {
    const int px = x >> 6;
    const int py = y >> 6;
    GrpGeom->SetColor({4, 4, 4});
    GrpGeom->DrawLine(px, py, (px + infx), (py + infy));
    break;
  }

  case LLF_DISABLE:
    break;
  }
}

void LaserLong::HitCheck() {
  if (Players.IsInvincible() != 0U) return;

  const int tx = Players.X() - x;
  const int ty = Players.Y() - y;
  const int length = cosl(d, tx) + sinl(d, ty);
  const int width  = abs(-sinl(d, tx) + cosl(d, ty));

  if (length > 0 && width <= (w + (64 * 15))) {
    Players.AddEvade(LLASER_EVADE);
  }
  if (length > 0 && width <= (w + PLAYER_HITBOX_RADIUS)) {
    Players.OnHit();
  }
}

bool LaserLong::IsDead() const {
  return flag == LLF_DISABLE;
}

void LaserLong::StartClear() {
  if (flag != LLF_DISABLE)
    flag = LLF_CLOSE;
}

// ── LaserManager long-laser public methods ─────────────────────

bool LaserManager::SpawnLongLaser(uint8_t id) {
  auto *lp = long_lasers.Alloc();
  if (lp == nullptr) return false;

  lp->dx = long_cmd.dx;
  lp->dy = long_cmd.dy;
  lp->e = long_cmd.e;

  lp->EnemyID = id;

  lp->x = lp->e->x + lp->dx;
  lp->y = lp->e->y + lp->dy;

  lp->v = long_cmd.v;

  lp->c = long_cmd.c;

  lp->lx = 0;
  lp->ly = 0;
  lp->wx = 0;
  lp->wy = 0;

  lp->w = 0;
  lp->wmax = long_cmd.w;

  lp->d = long_cmd.d;

  if (long_cmd.type == LLS_LONGZ) {
    lp->d += atan8(Players.X() - lp->x, Players.Y() - lp->y);
    lp->type = LLS_LONG;
  } else {
    lp->type = long_cmd.type;
  }

  lp->infx = cosl(lp->d, 800);
  lp->infy = sinl(lp->d, 800);

  lp->count = 0;

  SetLongPoint(lp);

  lp->flag = LLF_LINE;

  return true;
}

void LaserManager::OpenLong(const EnemyData *e, uint8_t id) {
  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL) &&
        lp->flag != LLF_DISABLE) {
      lp->flag = LLF_OPEN;
      Snd_SEPlay(static_cast<SfxId>(2), lp->x, true);
    }
  }
}

void LaserManager::CloseLong(const EnemyData *e, uint8_t id) {
  if (id == ECLCST_LLASERALL) {
    ForceCloseLong(e);
    return;
  }

  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
    if ((lp->e == e) && (lp->EnemyID == id)) {
      lp->flag = LLF_CLOSE;
      Snd_SEStop(2);
    }
  }
}

void LaserManager::LineLong(const EnemyData *e, uint8_t id) {
  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->flag = LLF_CLOSEL;
      Snd_SEStop(2);
    }
  }
}

void LaserManager::RotateLongAbs(const EnemyData *e, uint8_t d, uint8_t id) {
  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
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

void LaserManager::RotateLongRel(const EnemyData *e, char d, uint8_t id) {
  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
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

void LaserManager::ForceCloseLong(const EnemyData *e) {
  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
    if (lp->e == e) {
      lp->flag = LLF_CLOSE;
      Snd_SEStop(2);
    }
  }
}

void LaserManager::MoveLong() {
  for (uint16_t i = 0; i < long_lasers.count; i++) {
    auto *lp = &long_lasers.Active(i);
    lp->Move();                  // virtual
    lp->count++;
    if (lp->flag == LLF_OPEN || lp->flag == LLF_NORM)
      lp->HitCheck();
  }
  long_lasers.Compact([](const LaserLong &l) { return l.flag == LLF_DISABLE; });
}

void LaserManager::DrawLong() const {
  GrpGeom->Lock();

  for (uint16_t i = 0; i < long_lasers.count; i++)
    long_lasers.Active(i).Draw();

  GrpGeom->Unlock();
}

void LaserManager::ClearLong() {
  for (uint16_t i = 0; i < long_lasers.count; i++)
    long_lasers.Active(i).flag = LLF_CLOSE;
  Snd_SEStop(2);
}

void LaserManager::SetupLong() {
  long_lasers.Init();
  Snd_SEStop(2);
}
