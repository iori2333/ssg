///
/// HomingLaser - Long laser processing
///

#include "homing_laser.h"

#include "game/snd.h"
#include "game/ut_math.h"
#include "geometry.h"
#include "gian.h"
#include "laser_manager.h"
#include "platform/graphics_backend.h"
#include "player.h"
#include <utility>

static constexpr auto HOMINGL_WIDTH = (8 * 64);

// Access directly via homing_count

// [Macros]
constexpr int HLASER_GETNEXT(int current) {
  // Change mod -> and later
  return (current + (HLASER_LEN * HLASER_SECTION) - 1) %
         (HLASER_LEN * HLASER_SECTION);
}

constexpr int HLASER_GETPREV(int current, int n) {
  // Change mod -> and later
  return (current + n) % (HLASER_LEN * HLASER_SECTION);
}

// Initialize homing laser
void LaserManager::InitHoming() {
  int i = 0;

  homing_count = 0;

  active.Next = nullptr;
  free_list.Next = homing_buf.data();

  for (i = 0; i < HLASER_MAX - 2; i++) {
    homing_buf[i].Next = &homing_buf[i + 1];
  }

  homing_buf[HLASER_MAX - 1].Next = nullptr;
}

// Set up homing laser
void LaserManager::SpawnHoming(const HomingLaserInfo *hinfo) {
  int i = 0;
  int j = 0;
  uint8_t deg = 0;
  HomingLaserData *p = nullptr;

  // Using 1-n for angle setting...
  for (i = 1; std::cmp_less_equal(i, (hinfo->n)); i++) {
    p = free_list.Next;
    if (p == nullptr) {
      return; // Cannot allocate data
    }

    // Link pointers
    free_list.Next = free_list.Next->Next;
    p->Next = active.Next;
    active.Next = p;
    homing_count++;

    p->v = 64 * 4;  // Set acceleration
    p->a = 10;      // Set speed
    p->Count = 0;   // Frame counter
    p->Current = 0; // Head
    p->Left = 1;    // Remaining homing count

    p->c = hinfo->c;       // Color
    p->Type = hinfo->type; // Type
    p->State = HLS_NORM;   // State

    if ((hinfo->n & 1) != 0) {
      deg = hinfo->d + ((i >> 1) * (hinfo->dw) * (1 - ((i & 1) << 1)));
    } else {
      deg = hinfo->d - ((hinfo->dw) >> 1) +
            ((i >> 1) * (hinfo->dw) * (1 - ((i & 1) << 1)));
    }

    // Initialize tail info
    for (j = 0; j < HLASER_LEN * HLASER_SECTION; j++) {
      p->p[j].x = hinfo->x;
      p->p[j].y = hinfo->y;
      p->p[j].d = deg;
    }
  }
}

// Move homing laser
void LaserManager::MoveHoming() {
  HomingLaserData *hl = nullptr;
  HomingLaserData *temp = nullptr;
  int x = 0;
  int y = 0;
  int i = 0;
  int j = 0;
  int deg = 0;
  int deg2 = 0;

  // Advance to next frame
  for (hl = active.Next; hl != nullptr; hl = hl->Next) {
    // Save previous head temporarily
    x = hl->p[hl->Current].x;
    y = hl->p[hl->Current].y;
    deg = hl->p[hl->Current].d;

    // Do common update
    hl->Count++;
    hl->Current = HLASER_GETNEXT(hl->Current);

    // Type-specific movement
    switch (hl->Type) {
    case HL_TYPE1:
      deg2 = -deg + atan8(Players.viv.x - x, Players.viv.y - y);
      if (deg2 < -128) {
        deg2 += 256;
      } else if (deg2 > 128) {
        deg2 -= 256;
      }

      if (abs(deg2) < 8) {
        hl->Type = HL_NONE;
        Snd_SEPlay(17, hl->p[hl->Current].x);
      } else {
        if (hl->v > 2 * 64) {
          hl->v -= hl->a;
        }
        i = 1 + ((hl->Count) / 32);
        i = (deg2 * i) / 32;
        if (i != 0) {
          deg = deg + i;
        } else {
          deg = deg + deg2;
        }
      }

      if (hl->Count > 120) {
        hl->Type = HL_NONE;
      }

      hl->p[hl->Current].d = deg;
      hl->p[hl->Current].x = x + cosl(deg, hl->v);
      hl->p[hl->Current].y = y + sinl(deg, hl->v);
      break;

    case HL_NONE:
      hl->v += hl->a * 2;
      hl->p[hl->Current].d = deg;
      hl->p[hl->Current].x = x + cosl(deg, hl->v);
      hl->p[hl->Current].y = y + sinl(deg, hl->v);
      break;

    default:
      break;
    }

    // Save current tail temporarily
    i = HLASER_GETNEXT(hl->Current);
    x = hl->p[i].x;
    y = hl->p[i].y;

    // Out-of-range check
    if (x < GX_MIN - (4 * 64) || x > GX_MAX + (4 * 64) ||
        y < GY_MIN - (4 * 64) || y > GY_MAX + (4 * 64)) {
      hl->State = HLS_DEAD;
      continue;
    }

    if (Players.viv.muteki != 0U) {
      continue;
    }

    auto ev_flag = false;
    for (j = 0; j < HLASER_LEN * HLASER_SECTION; j++) {
      x = hl->p[j].x;
      y = hl->p[j].y;

      // Graze check
      if (HITCHK(x, Players.viv.x, HOMINGL_WIDTH + (15 * 64)) &&
          HITCHK(y, Players.viv.y, HOMINGL_WIDTH + (15 * 64))) {
        ev_flag = true;
      }

      // Hit check
      if (HITCHK(x, Players.viv.x, HOMINGL_WIDTH * 2 / 3) &&
          HITCHK(y, Players.viv.y, HOMINGL_WIDTH * 2 / 3)) {
        //	hl->State = HLS_DEAD;	// Delete this one
        MaidHit(); // Kill it
      }
    }
    if (ev_flag) {
      evade_add(1);
    }
  }

  // Remove unnecessary data
  for (hl = &active; (hl->Next) != nullptr;) {
    if (hl->Next->State == HLS_DEAD) {
      temp = hl->Next->Next;
      hl->Next->Next = free_list.Next;
      free_list.Next = hl->Next;
      hl->Next = temp;

      homing_count--;
    }
    // Otherwise advance pointer
    else {
      hl = hl->Next;
    }
  }
}

void CircleA16(GRAPHICS_GEOMETRY_POLY auto &gp, int x, int y, int r,
               uint8_t d) {
  VERTEX_XY src[9 + 1];
  int i = 0;
  int j = 0;

  for (j = 0, i = -64; j <= 8; j++) {
    src[j].x = (x + cosl(d + i, r)) >> 6;
    src[j].y = (y + sinl(d + i, r)) >> 6;
    i += 16;
  }

  src[9] = src[0];

  gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}

// Draw homing laser
void LaserManager::DrawHoming() const {
  HomingLaserData *hl = nullptr;
  int i = 0;
  int w = 0;
  int current = 0;
  DegPoint *p = nullptr;
  VERTEX_XY src[4];
  auto *gp = GrpGeom_Poly();
  auto *gf = GrpGeom_FB();
  const auto AlphaPolygon = [gp, gf](VERTEX_XY_SPAN<> p) {
    if (gp) {
      gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p);
    } else if (gf) {
      gf->DrawTriangleFan(p);
    }
  };
  // void (*AlphaCircle)(WINDOW_POINT center, WINDOW_POINT radius);

  if (gp != nullptr) {
    // AlphaCircle  = GeomCircleFA;
    gp->SetColor({1, 2, 5});
    gp->SetAlphaOne();
  } else if (gf != nullptr) {
    // AlphaCircle  = GeomCircle;
    gf->SetColor({2, 2, 5});
  }

  GrpGeom->Lock();

  for (hl = active.Next; hl != nullptr; hl = hl->Next) {
    w = HOMINGL_WIDTH;
    current = hl->Current;
    p = &(hl->p[current]);

    // Optimize later
    src[0].x = (p->x + cosl(p->d - 64, w)) >> 6;
    src[0].y = (p->y + sinl(p->d - 64, w)) >> 6;
    src[1].x = (p->x - cosl(p->d - 64, w)) >> 6;
    src[1].y = (p->y - sinl(p->d - 64, w)) >> 6;

    if (gp != nullptr) {
      CircleA16(*gp, p->x, p->y, w, p->d);
    } else {
      GeomCircleF({(p->x >> 6), (p->y >> 6)}, (w >> 6));
    }

    for (i = 0; i < HLASER_LEN - 1; i++) {
      // temp    = p;
      current = HLASER_GETPREV(current, HLASER_SECTION);
      p = &(hl->p[current]);
      // GrpGeom->DrawLine(
      // 	(p->x >> 6), (p->y >> 6), (temp->x >> 6), (temp->y >> 6)
      // );

      src[2].x = (p->x - cosl(p->d - 64, w)) >> 6;
      src[2].y = (p->y - sinl(p->d - 64, w)) >> 6;
      src[3].x = (p->x + cosl(p->d - 64, w)) >> 6;
      src[3].y = (p->y + sinl(p->d - 64, w)) >> 6;
      AlphaPolygon(src);

      src[0] = src[3];
      src[1] = src[2];

      if (w > 64 * 2) {
        w -= 64;
      }
    }
  }

  if (gp != nullptr) {
    gp->SetColor({3, 4, 5});
  } else if (gf != nullptr) {
    gf->SetColor({5, 5, 5});
  }

  for (hl = active.Next; hl != nullptr; hl = hl->Next) {
    w = HOMINGL_WIDTH / 2;
    current = hl->Current;
    p = &(hl->p[current]);

    src[0].x = (p->x + cosl(p->d - 64, w)) >> 6;
    src[0].y = (p->y + sinl(p->d - 64, w)) >> 6;
    src[1].x = (p->x - cosl(p->d - 64, w)) >> 6;
    src[1].y = (p->y - sinl(p->d - 64, w)) >> 6;

    // AlphaCircle(p->x>>6, p->y>>6, w>>6);
    if (gp != nullptr) {
      CircleA16(*gp, p->x, p->y, w, p->d);
    } else {
      GeomCircleF({(p->x >> 6), (p->y >> 6)}, (w >> 6));
    }

    for (i = 0; i < HLASER_LEN - 1; i++) {
      // temp    = p;
      current = HLASER_GETPREV(current, HLASER_SECTION);
      p = &(hl->p[current]);
      // GrpGeom->DrawLine(
      // 	(p->x >> 6), (p->y >> 6), (temp->x >> 6), (temp->y >> 6)
      // );

      src[2].x = (p->x - cosl(p->d - 64, w)) >> 6;
      src[2].y = (p->y - sinl(p->d - 64, w)) >> 6;
      src[3].x = (p->x + cosl(p->d - 64, w)) >> 6;
      src[3].y = (p->y + sinl(p->d - 64, w)) >> 6;
      AlphaPolygon(src);

      src[0] = src[3];
      src[1] = src[2];

      if (w > 64) {
        w -= 64;
      } else {
        break;
      }
    }
  }

  GrpGeom->Unlock();
}

// Set homing laser clear effect
void LaserManager::ClearHoming() {
  //	HLaserData	*hl;

  InitHoming();
  // for(hl = active.Next; hl != nullptr; hl = hl->Next) {
  //         hl->State = HLS_CLEAR;
  // }
}
