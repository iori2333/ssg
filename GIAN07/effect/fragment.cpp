///
/// Fragment - Fragment processing functions
///

#include "fragment.h"
#include "effect_manager.h"

#include "data/gfx_manager.h"
#include "data/sfx_manager.h"
#include "gfx/graphics_backend.h"
#include "gfx/geometry.h"
#include "util/ut_math.h"

// fragments[], fragment_ptr moved to EffectManager in effect_manager.cpp

static void FDraw(const FragmentData *f);

void EffectManager::SpawnFragment(int x, int y, uint8_t cmd) {
  int i = 0;
  int l = 0;
  uint8_t d = 0;
  FragmentData *f = fragments.data() + fragment_ptr;

  if (cmd == FRG_ESCAPE) {
    for (i = 0; i < FRAGMENT_MAX; i++) {
      f = fragments.data() + i;
      if (f->count != 0U) {
        f->vx = ((f->x - x) / 16); // f->count;
        f->vy = ((f->y - y) / 16); // f->count;
      }
    }
  } else if (cmd == FRG_APPROACH) {
    for (i = 0; i < FRAGMENT_MAX; i++) {
      f = fragments.data() + i;
      if (f->count != 0U) {
        f->vx = (x - f->x) / f->count;
        f->vy = (x - f->y) / f->count;
      }
    }
  }

  f->cmd = cmd;
  f->x = x;
  f->y = y;

  switch (cmd) {
  case FRG_HIT:
    d = rnd() & 0xff;
    l = 64 + (rnd() % (64 * 3));
    f->count = 24;
    f->vx = cosl(d, l);
    f->vy = sinl(d, l);
    break;

  case FRG_EVADE:
    d = rnd() & 0xff;
    l = (64 * 4) + (rnd() % (64 * 3));
    f->count = 24;
    f->vx = cosl(d, l);
    f->vy = sinl(d, l);
    break;

  case FRG_SMOKE:
    f->count = 24;
    f->vx = 0; // cosl((BYTE)rnd(),rnd()&0xff);//cosl((BYTE)rnd()%256,64*6);
    f->vy = 0; // sinl((BYTE)rnd(),rnd()&0xff);//sinl((BYTE)rnd()%256,64*6);
    break;

  case FRG_STAR1:
    f->count = 64;
    d = rnd() & 0xff;
    l = (64 * 5) + (rnd() % (64 * 3));
    f->vx = cosl(d, l);
    f->vy = sinl(d, l);
    break;

  case FRG_STAR2:
    f->count = 64;
    d = rnd() & 0xff;
    l = (64 * 4) + (rnd() % (64 * 3));
    f->vx = cosl(d, l);
    f->vy = sinl(d, l);
    break;

  case FRG_STAR3:
    f->count = 64;
    d = -64 - 48 + (rnd() % 96);
    l = (64 * 6) + (rnd() % (64 * 4));
    f->vx = cosl(d, l);
    f->vy = sinl(d, l);
    break;

  case FRG_HEART:
    f->count = 105;
    d = rnd() & 0xff;
    l = (64 * 2) + (rnd() % (64 * 5));
    f->vx = cosl(d, l);
    f->vy = sinl(d, l);
    break;

  case FRG_FATCIRCLE:
    f->count = 60;
    f->vx = 0;
    f->vy = 0;
    break;

  default:
    // It's a bug, but...
    break;
  }

  fragment_ptr = (fragment_ptr + 1) % FRAGMENT_MAX;
}

void EffectManager::MoveFragments() {
  for (auto &it : fragments) {
    auto *f = &it;
    if (f->count != 0U) {
      f->x += f->vx;
      f->y += f->vy;
      f->count--;
    }
  }
}

void EffectManager::DrawFragments() {
  for (const auto &it : fragments) {
    if (it.count != 0U) {
      FDraw(&it);
    }
  }
}

void EffectManager::InitFragments() {
  for (auto &it : fragments) {
    // memset(fragments+i,0,sizeof(FRAGMENT_DATA));
    it.count = 0;
  }

  fragment_ptr = 0;
}

static void FDraw(const FragmentData *f) {
  int x = 0;
  int y = 0;
  PIXEL_LTRB src;

  switch (f->cmd) {
  case FRG_EVADE:
    x = (f->x >> 6) - 4;
    y = (f->y >> 6) - 4;
    src = PIXEL_LTWH{(592 + (((24 - f->count) >> 2) << 3)), 8, 8, 8};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_HIT:
    x = (f->x >> 6) - 4;
    y = (f->y >> 6) - 4;
    src = PIXEL_LTWH{(592 + (((24 - f->count) >> 2) << 3)), (8 + 8), 8, 8};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_SMOKE:
    x = (f->x >> 6) - 4;
    y = (f->y >> 6) - 4;
    src = PIXEL_LTWH{(592 + (((24 - f->count) >> 2) << 3)), 0, 8, 8};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_STAR1:
    x = (f->x >> 6) - 8;
    y = (f->y >> 6) - 8;
    src = PIXEL_LTWH{624, 432, 16, 16};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_STAR2:
    x = (f->x >> 6) - 16;
    y = (f->y >> 6) - 16;
    src = PIXEL_LTWH{608, 448, 32, 32};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_STAR3:
    x = (f->x >> 6) - 16;
    y = (f->y >> 6) - 16;
    src = PIXEL_LTWH{608, 448, 32, 32};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_HEART:
    x = (f->x >> 6) - 16;
    y = (f->y >> 6) - 16;
    src = PIXEL_LTWH{576, 448, 32, 32};
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
    break;

  case FRG_FATCIRCLE:
    if (auto *gp = GrpGeom_Poly()) {
      gp->Lock();
      gp->SetColor({4, 0, 0});
      gp->SetAlphaOne();
      GeomFatCircleA(*gp, {(f->x >> 6), (f->y >> 6)}, ((60 - f->count) * 6), 5);
      gp->Unlock();
    }
    break;

  default:
    // Shouldn't reach here, but...
    break;
  }
}
