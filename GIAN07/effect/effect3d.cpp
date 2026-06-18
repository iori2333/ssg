///
/// Effect3D - 3D effect processing
///

#include "effect3d.h"
#include "game/cast.h"
#include "game/ut_math.h"
#include "gian.h"
#include "platform/graphics_backend.h"

// CIRCLE_MAX, CUBE_MAX, STAR_MAX, ROCK_MAX, FAKE_ECLSTR_MAX → effect_manager.h
// circles[], cubes[], stars[], rocks[], wf_line, fake_ecl_strs[] → effect_manager.cpp

#define _ PIXEL_POINT

WORLD_POINT PList_W[11] = {
    _{.x = 0, .y = 15},  _{.x = 15, .y = 66}, _{.x = 32, .y = 47},
    _{.x = 48, .y = 66}, _{.x = 63, .y = 14}, _{.x = 52, .y = 11},
    _{.x = 42, .y = 38}, _{.x = 32, .y = 26}, _{.x = 21, .y = 38},
    _{.x = 11, .y = 10}, _{.x = 0, .y = 15},
};

WORLD_POINT PList_A1[8] = {
    _{.x = 96, .y = 12},  _{.x = 66, .y = 61},  _{.x = 75, .y = 67},
    _{.x = 83, .y = 56},  _{.x = 107, .y = 56}, _{.x = 115, .y = 67},
    _{.x = 125, .y = 61}, _{.x = 96, .y = 12},
};

WORLD_POINT PList_A2[4] = {
    _{.x = 96, .y = 34},
    _{.x = 90, .y = 44},
    _{.x = 101, .y = 44},
    _{.x = 96, .y = 34},
};

WORLD_POINT PList_R[15 - 1] = {
    _{.x = 132, .y = 14},
    _{.x = 132, .y = 64},
    _{.x = 145, .y = 64},
    _{.x = 145, .y = 27},
    _{.x = 164, .y = 27},

    //_{ 150, 41 }, _{ 150, 43 },
    _{.x = 150, .y = 42},

    _{.x = 171, .y = 66},
    _{.x = 173, .y = 66},
    _{.x = 181, .y = 57},
    _{.x = 167, .y = 43},
    _{.x = 180, .y = 29},
    _{.x = 180, .y = 27},
    _{.x = 170, .y = 14},
    _{.x = 132, .y = 14},
};

WORLD_POINT PList_N1[9] = {
    _{.x = 189, .y = 12}, _{.x = 189, .y = 64}, _{.x = 201, .y = 64},
    _{.x = 201, .y = 40}, _{.x = 239, .y = 66}, _{.x = 239, .y = 14},
    _{.x = 227, .y = 14}, _{.x = 227, .y = 38}, _{.x = 189, .y = 12},
};

WORLD_POINT PList_N2[9] = {
    _{.x = 189, .y = 12}, _{.x = 189, .y = 64}, _{.x = 201, .y = 64},
    _{.x = 201, .y = 40}, _{.x = 239, .y = 66}, _{.x = 239, .y = 14},
    _{.x = 227, .y = 14}, _{.x = 227, .y = 38}, _{.x = 189, .y = 12},
};

WORLD_POINT PList_I[5] = {
    _{.x = 248, .y = 14}, _{.x = 248, .y = 64}, _{.x = 262, .y = 64},
    _{.x = 262, .y = 14}, _{.x = 248, .y = 14},
};

WORLD_POINT PList_G[17] = {
    _{.x = 354, .y = 11}, _{.x = 328, .y = 22}, _{.x = 328, .y = 57},
    _{.x = 354, .y = 68}, _{.x = 380, .y = 59}, _{.x = 380, .y = 34},
    _{.x = 355, .y = 34}, _{.x = 354, .y = 45}, _{.x = 367, .y = 46},
    _{.x = 367, .y = 51}, _{.x = 355, .y = 55}, _{.x = 342, .y = 50},
    _{.x = 342, .y = 29}, _{.x = 354, .y = 24}, _{.x = 372, .y = 30},
    _{.x = 377, .y = 19}, _{.x = 354, .y = 11},
};

#undef _

// LineList3D	LList_G = {354,39,PList_G,17,PWork_G};
// LineList3D	LList_I = {255,39,PList_I,9,PWork_I};
// LineList3D	LList_N = {215,39,PList_N,9,PWork_N};
// LineList3D	LList_R = {156,39,PList_R,15,PWork_R};
// LineList3D	LList_A2 = {96,39,PList_A2,4,PWork_A2};
// LineList3D	LList_A1 = {96,39,PList_A1,8,PWork_A1};
// LineList3D	LList_W = {32,39,PList_W,11,PWork_W};

// Warning[8] moved to EffectManager::warning_lines — initialized in InitWarningText()

static void RollPoint(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz);
static void Draw3DCube(const Cube3D *c); // General 3D cube drawing

void Transform3D(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz) {
  static Point3D temp;

  temp.y = p->y;
  temp.z = p->z;
  p->y = (cosl(dx, temp.y) - sinl(dx, temp.z));
  p->z = (sinl(dx, temp.y) + cosl(dx, temp.z));

  temp.x = p->x;
  temp.z = p->z;
  p->x = (cosl(dy, temp.x) + sinl(dy, temp.z));
  p->z = (-sinl(dy, temp.x) + cosl(dy, temp.z));

  temp.x = p->x;
  temp.y = p->y;
  p->x = (cosl(dz, temp.x) - sinl(dz, temp.y));
  p->y = (sinl(dz, temp.x) + cosl(dz, temp.y));
}

void ShiftRight6Bit(const Point3D *o, Point3D *p) {
  p->x = (((p->x + o->x) >> 6) + 320);
  p->y = (((p->y + o->y) >> 6) + 240);
}

void EffectManager::InitWarningText() {
  static bool bInitialized = false;

  if (bInitialized) {
    return;
  }

  // Initialize warning_lines with center positions and point arrays
  // (was: LineList3D Warning[8] = { ... } file-static initialization)
  warning_lines[0] = {.center = {.x = 192, .y = 39}, .p = PList_W};
  warning_lines[1] = {.center = {.x = 192, .y = 39}, .p = PList_A1};
  warning_lines[2] = {.center = {.x = 192, .y = 39}, .p = PList_A2};
  warning_lines[3] = {.center = {.x = 192, .y = 39}, .p = PList_R};
  warning_lines[4] = {.center = {.x = 192, .y = 39}, .p = PList_N1};
  warning_lines[5] = {.center = {.x = 192, .y = 39}, .p = PList_I};
  warning_lines[6] = {.center = {.x = (192 - (296 - 215)), .y = 39},
                      .p = PList_N2};
  warning_lines[7] = {.center = {.x = 192, .y = 39}, .p = PList_G};

  InitLineList3D(warning_lines);
  bInitialized = true;
}

void EffectManager::DrawWarningText() {
  constexpr PIXEL_LTRB src = {0, (152 + 16), 384, (232 + 16)};
  int st = 0;
  int det = 0;
  static int count;

  count += 8;

  if (warning_lines[0].DegX == 0) {
    GrpGeom->Lock();
    GrpGeom->SetAlphaNorm(Cast::down_sign<uint8_t>(128 + sinl(count, 48)));
    GrpGeom->SetColor({5, 0, 0});
    GrpGeom->DrawBoxA(129, (40 + 6), (128 + 384), (60 + 6));
    GrpGeom->DrawBoxA(129, (60 + (232 - 152) - 4), (128 + 384),
                      (80 + (232 - 152) - 4));
    GrpGeom->Unlock();
    GrpSurface_Blit({(128 + 1), (60 + 1)}, SURFACE_ID::SYSTEM, src);
  } else {
    GrpGeom->Lock();

    if (warning_lines[0].DegX < 10) {
      st = 0;
      det = 0;
      st = 0;
      det = 0;
    } else if (warning_lines[0].DegX < 20) {
      st = -4;
      det = 1;
    } else {
      // if(warning_lines[0].DegX < 40){
      st = -8;
      det = 2;
    }

    const auto w = std::span(warning_lines);
    GrpGeom->SetColor({1, 1, 5});
    MoveWarningR(st);
    DrawLineList3D(w);
    GrpGeom->SetColor({2, 2, 5});
    MoveWarningR(det);
    DrawLineList3D(w);
    GrpGeom->SetColor({3, 3, 5});
    MoveWarningR(det);
    DrawLineList3D(w);
    GrpGeom->SetColor({4, 4, 5});
    MoveWarningR(det);
    DrawLineList3D(w);
    GrpGeom->SetColor({5, 5, 5});
    MoveWarningR(det);
    DrawLineList3D(w);

    GrpGeom->Unlock();
  }
}

void MoveWarningR(char count) {
  if (count == 0) {
    return;
  }

  for (auto &llist : Effects.warning_lines) {
    llist.DegX += (count * 2);
    llist.DegY += (count * 1);
    llist.DegZ += (count * 4);
  }
}

void EffectManager::MoveWarningText(uint8_t count) {
  for (auto &llist : warning_lines) {
    llist.DegX = ((count < 64) ? ((64 - count) * 2) : 0);
    llist.DegY = ((count < 64) ? ((64 - count) * 1) : 0);
    llist.DegZ = ((count < 64) ? ((64 - count) * 4) : 0);
  }
}

void InitLineList3D(std::span<LineList3D> w) {
  for (auto &llist : w) {
    for (auto &point : llist.p) {
      point -= llist.center;
    }
  }
}

void DrawLineList3D(std::span<const LineList3D> w) {
  const auto roll = [](const LineList3D &llist, const WORLD_POINT &point) {
    Point3D temp = {.x = point.x, .y = point.y, .z = 0};
    RollPoint(&temp, llist.DegX, llist.DegY, llist.DegZ);
    return WORLD_POINT{&temp.x, &temp.y};
  };

  for (const auto &llist : w) {
    WORLD_POINT line_p[2] = {roll(llist, llist.p.front())};
    for (const auto &point : (llist.p | std::views::drop(1))) {
      line_p[1] = roll(llist, point);
      const auto p1 = (PIXEL_POINT{.x = 320, .y = 100} + line_p[0].ToPixel());
      const auto p2 = (PIXEL_POINT{.x = 320, .y = 100} + line_p[1].ToPixel());
      GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
      std::swap(line_p[1], line_p[0]);
    }
  }
}

static void RollPoint(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz) {
  Point3D temp{};

  temp.y = p->y;
  temp.z = p->z;
  p->y = cosl(dx, temp.y) - sinl(dx, temp.z);
  p->z = sinl(dx, temp.y) + cosl(dx, temp.z);

  temp.x = p->x;
  temp.z = p->z;
  p->x = cosl(dy, temp.x) + sinl(dy, temp.z);
  p->z = -sinl(dy, temp.x) + cosl(dy, temp.z);

  temp.x = p->x;
  temp.y = p->y;
  p->x = cosl(dz, temp.x) - sinl(dz, temp.y);
  p->y = sinl(dz, temp.x) + cosl(dz, temp.y);
}

void EffectManager::Init3DCubes() {
  int i = 0;

  for (i = 0; i < CUBE_MAX; i++) {
    cubes[i].l = 30 * 64;
    cubes[i].d.dx = rnd();
    cubes[i].d.dy = rnd();
    cubes[i].d.dz = rnd();
    cubes[i].p.x = cosl(i * 256 / CUBE_MAX, 200 * 64);
    cubes[i].p.y = sinl(i * 256 / CUBE_MAX, 200 * 64);
    cubes[i].p.z = 0;
  }

  for (auto &it : stars) {
    it.x = ((rnd() % (640 - 256)) + 128);
    it.y = -(rnd() % 480);
    it.vy = ((rnd() % 10) + 10);
  }
}

void EffectManager::Draw3DCubes() {
  for (const auto &it : stars) {
    constexpr PIXEL_LTWH rc = {136, 272, 16, 24};
    GrpSurface_Blit({it.x, it.y}, SURFACE_ID::SYSTEM, rc);
  }

  GrpGeom->Lock();
  for (const auto &it : cubes) {
    Draw3DCube(&it);
  }
  GrpGeom->Unlock();
}

void EffectManager::Move3DCubes() {
  int i = 0;
  int l = 0;
  int d2 = 0;
  static uint16_t d;
  static uint16_t dx;
  static uint16_t dy;
  static uint16_t dz;

  d += 64 * 4;

  dx += 32 * 4;
  dy -= 16 * 4;

  d2 = sinl(d >> 8, 512 / CUBE_MAX);
  l = sinl(d >> 7, 100 * 64) + ((200 - 20) * 64);

  for (i = 0; i < CUBE_MAX; i++) {
    cubes[i].l = (15 * 64) + (l >> 4) + (i * 128);
    cubes[i].d.dx += 4;
    cubes[i].d.dy -= 4;
    // cubes[i].p.x = cosl(i*256/CUBE_MAX+d2, l);
    // cubes[i].p.y = sinl(i*256/CUBE_MAX+d2, l);
    cubes[i].p.x = cosl((i * 500 / CUBE_MAX) + d2, l);
    cubes[i].p.y = sinl((i * 500 / CUBE_MAX) + d2, l);
    cubes[i].p.z = (i - (CUBE_MAX / 2)) * 64 * 40;
    Transform3D(&cubes[i].p, dx >> 8, dy >> 8, dz >> 8);
  }

  for (auto &it : stars) {
    it.y += it.vy;
    if (it.y > 480) {
      it.x = ((rnd() % (640 - 256)) + 128);
      it.y = 0;
      it.vy = ((rnd() % 10) + 10);
    }
  }
}

// General 3D cube drawing
static void Draw3DCube(const Cube3D *c) {
  // Would be faster with 3D alpha-blended or alpha-test polygons, but...
  // for 8-bit support this will have to do

  int x = 0;
  int y = 0;
  int z = 0;
  int l = 0;
  int l2 = 0;
  Point3D p1{};
  Point3D p2{};
  Point3D o{};

  o = c->p;
  l = c->l;
  const uint8_t dx = c->d.dx;
  const uint8_t dy = c->d.dy;
  const uint8_t dz = c->d.dz;

  l2 = l;

  // GrpGeom->SetColor({ 1, 1, 2 });
  GrpGeom->SetColor({1, 1, 3});
  for (x = -1; x <= 1; x++) {
    for (y = -1; y <= 1; y++) {
      p1.x = x * l;
      p1.y = y * l;
      p1.z = -l2;
      Transform3D(&p1, dx, dy, dz);
      ShiftRight6Bit(&o, &p1);

      p2.x = x * l;
      p2.y = y * l;
      p2.z = l2;
      Transform3D(&p2, dx, dy, dz);
      ShiftRight6Bit(&o, &p2);

      GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
    }
  }

  GrpGeom->SetColor({0, 0, 3});
  for (y = -1; y <= 1; y++) {
    for (z = -1; z <= 1; z++) {
      p1.x = -l2;
      p1.y = y * l;
      p1.z = z * l;
      Transform3D(&p1, dx, dy, dz);
      ShiftRight6Bit(&o, &p1);

      p2.x = l2;
      p2.y = y * l;
      p2.z = z * l;
      Transform3D(&p2, dx, dy, dz);
      ShiftRight6Bit(&o, &p2);

      GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
    }
  }

  // GrpGeom->SetColor({ 1, 1, 3 });
  GrpGeom->SetColor({1, 1, 4});
  for (x = -1; x <= 1; x++) {
    for (z = -1; z <= 1; z++) {
      p1.x = x * l;
      p1.y = -l2;
      p1.z = z * l;
      Transform3D(&p1, dx, dy, dz);
      ShiftRight6Bit(&o, &p1);

      p2.x = x * l;
      p2.y = l2;
      p2.z = z * l;
      Transform3D(&p2, dx, dy, dz);
      ShiftRight6Bit(&o, &p2);

      GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
    }
  }
}

void EffectManager::InitFakeECL() {
  int v = 0;

  wf_line.d = 0;
  wf_line.ox = 640 * 64 / 2;
  wf_line.oy = 480 * 64 / 2;
  wf_line.w = 30;

  for (auto &it : fake_ecl_strs) {
    const uint8_t d = (rnd() % 128);
    v = (rnd() % (64 * 5)) + (64 * 5);

    it.SrcX = (((rnd() % 7) * 9) * 8);
    it.SrcY = ((rnd() % 16) * 16);
    it.x = ((28 + (rnd() % 484)) * 64);
    it.y = -((rnd() % 640) * 64);
    it.vx = 0; // cosl(d, v);
    it.vy = v; // sinl(d, v);
  }
}

void EffectManager::MoveFakeECL() {
  int v = 0;

  wf_line.ox = (wf_line.ox + 1) % 64;
  wf_line.oy = (wf_line.oy + 62) % 64;

  for (auto &it : fake_ecl_strs) {
    it.x += it.vx;
    it.y += it.vy;

    if (it.y >= (480 * 64)) {
      const uint8_t d = (rnd() % 128);
      v = (rnd() % (64 * 5)) + (64 * 5);

      it.SrcX = (((rnd() % 7) * 9) * 8);
      it.SrcY = ((rnd() % 16) * 16);
      it.x = ((28 + (rnd() % 484)) * 64);
      it.y = -((rnd() % 640) * 64);
      it.vx = 0; // cosl(d, v);
      it.vy = v; // sinl(d, v);
    }
  }
}

void EffectManager::DrawFakeECL() {
  PIXEL_LTRB src;
  int i = 0;
  int j = 0;

  GrpGeom->Lock();

  // GrpGeom->SetColor({ 3, 2, 0 });	// For latter half of battle
  GrpGeom->SetColor({0, 2, 0});
  // GrpGeom->SetColor({ 0, 0, 3 });

  for (i = 128 - (wf_line.ox / 2); i < 640 - 128; i += 32) {
    GrpGeom->DrawLine(i, 0, i, 480);
  }

  for (j = wf_line.oy / 2; j < 480; j += 32) {
    GrpGeom->DrawLine(128, j, (640 - 128), j);
  }

  // GrpGeom->SetColor({ 5, 3, 0 });	// For latter half of battle
  GrpGeom->SetColor({0, 3, 0});
  // GrpGeom->SetColor({ 0, 0, 4 });

  for (i = 128 - wf_line.ox; i < 640 - 128; i += 64) {
    GrpGeom->DrawLine(i, 0, i, 480);
  }

  for (j = -wf_line.oy; j < 480; j += 64) {
    GrpGeom->DrawLine(128, j, (640 - 128), j);
  }

  GrpGeom->Unlock();

  for (const auto &it : fake_ecl_strs) {
    src = PIXEL_LTWH{it.SrcX, it.SrcY, 72, 16};
    GrpSurface_Blit({(it.x >> 6), (it.y >> 6)}, SURFACE_ID::MAPCHIP, src);
  }

  src = {0, 272, 416, 352};
  GrpSurface_Blit({128, 400}, SURFACE_ID::MAPCHIP, src);
}

// Stage 3 cloud init
// void InitStg3Cloud(void)
// {
//         Cloud2D		*p;		//Cloud[CLOUD_MAX];
//         int			i;
// 
//         p = Cloud;
// 
//         for(i=0; i<CLOUD_MAX; i++, p++){
//                 if(rnd()&1) p->x = 128*64+(rnd()>>1)%(100*64);
//                 else        p->x = 512*64-(rnd()>>1)%(100*64);
//                 p->y    = ((i*680*64)/CLOUD_MAX)-200*64;	//GY_RND;
// 
//                 p->type = (rnd()>>2)%5;
//                 if(p->type == 2) p->type = 5;
// 
//                 p->vy   = rnd()%(64 * 6) + 64 * 11;
//         }
// }
// 
// 
// Stage 3 cloud movement
// void MoveStg3Cloud(void)
// {
//         Cloud2D		*p;		//Cloud[CLOUD_MAX];
//         int			i;
// 
//         p = Cloud;
// 
//         for(i=0; i<CLOUD_MAX; i++, p++){
//                 p->y += p->vy;
// 
//                 if(p->y > (480+200)*64){
//                         if(rnd()&1) p->x = 128*64+(rnd()>>1)%(100*64);
//                         else        p->x = 512*64-(rnd()>>1)%(100*64);
//                         p->y    = -200*64;
// 
//                         p->type = (rnd()>>2)%5;
//                         if(p->type == 2) p->type = 5;
// 
//                         p->vy   = rnd()%(64 * 6) + 64 * 11;
//                 }
//         }
// }
// 
// 
// Stage 3 cloud draw
// void DrawStg3Cloud(void)
// {
// constexpr auto RsetMacro(int x, int y, int w, int h) -> PIXEL_LTRB { return {x,
// y, x + w, y + h}; } static PIXEL_LTRB Data[6] = { RsetMacro(  0, 288, 144, 160),
// // Large_1 RsetMacro(144, 288, 144, 112),			// Large_2
//                 RsetMacro(288, 288, 144, 176),			// Large_3
// 
//                 //RsetMacro(480, 288,  32,  48),			//
// Small_1 RsetMacro(144, 400,  32,  48),			// Small_2
//                 RsetMacro(176, 400,  48,  32),			// Small_3
//                 RsetMacro(224, 400,  48,  48),			// Small_4
//         };
// #undef _RsetMacro
// 
//         static PIXEL_LTRB	Size[6] = {
//                 144/2, 160/2, 144/2, 112/2, 144/2, 176/2,		// Large
//                  //32,  48,
//                  32/2,  48/2,  48/2,  32/2,  48/2,  48/2		// Small
//         };
// 
//         PIXEL_LTRB	src;
//         Cloud2D		*p;		//Cloud[CLOUD_MAX];
//         int			i, j;
//         int			x, y;
// 
//         p = Cloud;
// 
//         for(i=0; i<CLOUD_MAX; i++, p++){
//                 j   = p->type;
//                 x   = (p->x >> 6) - Size[j].x;
//                 y   = (p->y >> 6) - Size[j].y;
//                 src = Data[j];
// 
//                 GrpSurface_Blit({ x, y }, SURFACE_ID::ENEMY, src);
//         }
// }

void EffectManager::InitStg4Rocks() {
  int i = 0;
  int id = 2;
  int y = 0;
  constexpr int dy = (500 * (64 / 4));
  constexpr int dy2 = (dy / 2);

  for (i = 0; i < ROCK_MAX; i++) {
    y = ((i % 4) * dy) + (rnd() % dy2);
    //((380*64/16) * (i%(ROCK_MAX/16+1))) + rnd()%(380*64/16);

    rocks[i].x = ((rnd() % (500 * 64)) - (250 * 64));
    rocks[i].y = (-250 * 64) - y;                     // Up above
    rocks[i].z = ((rnd() % (500 * 64)) - (250 * 64));

    if (i == ROCK_MAX * 5 / 8) {
      id--;
    }
    if (i == ROCK_MAX * 7 / 8) {
      id--;
    }

    rocks[i].GrpID = id;

    rocks[i].vx = 0;
    rocks[i].vy = (4 - rocks[i].GrpID) * 16;
    rocks[i].v = rocks[i].vy;

    rocks[i].count = 0;
    rocks[i].a = 0;
    rocks[i].d = 64;
    rocks[i].State = STG4ROCK_STDMOVE;
  }
}

void EffectManager::MoveStg4Rocks() {
  constexpr int dy = (500 * (64 / 4));
  constexpr int dy2 = (dy / 2);

  for (auto &it : rocks) {
    auto *p = &it;
    p->count++;

    switch (p->State) {
    case STG4ROCK_STDMOVE:
      p->y += p->vy;
      if (p->y > (250 + 40) * 64) {
        p->x = ((rnd() % (500 * 64)) - (250 * 64));
        p->y = (-250 - 40 - (rnd() % 250)) * 64;
        p->vy = ((4 - p->GrpID) * 16);
        p->v = p->vy;
      }
      break;

    case STG4ROCK_ACCMOVE1:
      p->v += p->a;
      p->vy = p->v;
      p->y += p->vy;

      if (p->y > (250 + 40) * 64) {
        p->x = ((rnd() % (500 * 64)) - (250 * 64));
        p->y = (-250 - 40 - (rnd() % 250)) * 64;
        p->vy = ((4 - p->GrpID) * 32 * 3);
        p->v = p->vy;
        p->a = 0;
      }
      break;

    case STG4ROCK_ACCMOVE2:
      p->v -= p->a;
      p->vy = p->v;
      p->y += p->vy;

      if (p->count > 60 * 2) {
        if (p->y > (250 + 40) * 64 || p->y < (-250 - 40) * 64) {
          p->x = ((rnd() % (500 * 64)) - (250 * 64));
          p->y = (-250 - 40 - (rnd() % 250)) * 64;
          p->vy = ((4 - p->GrpID) * 32);
          p->v = p->vy;
        }

        p->vy = ((4 - p->GrpID) * 32);
        p->a = 2;
        p->v = p->vy;
        p->State = STG4ROCK_ACCMOVE1;

        break;
      }

      if (p->y > (250 + 40) * 64 || p->y < (-250 - 40) * 64) {
        p->x = ((rnd() % (500 * 64)) - (250 * 64));
        p->y = ((250 + 40) * 64) + (rnd() % 250);
        p->vy = (-(4 - p->GrpID) * 32 * 3);
        p->v = p->vy;
        p->a = 0;
      }

      //			p->v += p->a;
      //                         p->x += cosl(p->d, p->v);
      //                         p->y += (p->vy + sinl(p->d, p->v));
      // 
      //                         if(p->count > 60){
      //                                 if(p->y > (250+40)*64 || p->y <
      //    (-250-40)*64){
      //                                         //p->x     =
      //    (rnd()%(500*64)-250*64); y = (i%4)*dy + (rnd()%dy2); p->x =
      //    (rnd()%(500*64)-250*64);		// p->y = -250*64-y;
      //    // Upper part
      // 
      //                                         p->vy    = ((4 - p->GrpID) * 32 *
      //    3); p->v     = p->vy; p->a     = 0; p->State = STG4ROCK_ACCMOVE1;
      //                                 }
      //                                 else{
      //                                         p->v = p->vy = ((4 - p->GrpID) *
      //    32 * 3); p->a = 0; p->State = STG4ROCK_ACCMOVE1;
      //                                 }
      // 
      //                                 break;
      //                         }
      // 
      //                         if(p->y > (250+40)*64 || p->y < (-250-40)*64){
      //                                 p->x     = (rnd()%(700*64)-350*64);
      //                                 p->y     = (250+40)*64;
      //                                 p->vy    = ((4 - p->GrpID) * 32 * 3);
      //                                 p->v     = 10;
      //                                 p->a     = -4;
      //                         }
      break;

    case STG4ROCK_LEAVE:
      if (p->y > (500 + 40) * 64) {
        break;
      } else {
        p->y += p->vy;
      }
      break;

    case STG4ROCK_END:
      if (p->y > (500 + 40) * 64) {
        {
          break;
        }
      } else {
        p->y += ((4 - p->GrpID) * 32 * 6);
      }
      break;

    default:
      break;
    }
  }
}

void EffectManager::DrawStg4Rocks() {
  constexpr auto sid = SURFACE_ID::MAPCHIP;
  static PIXEL_LTRB src[3] = {
      {0, 224, 80, 288}, {0, 288, 48, 336}, {48, 288, 80, 320}};
  static int dx[3] = {80 / 2, 48 / 2, 32 / 2};
  static int dy[3] = {64 / 2, 48 / 2, 32 / 2};

  int x = 0;
  int y = 0;

  for (const auto &it : rocks) {
    const auto *p = &it;
    x = (p->x + GX_MID) >> 6;
    y = (p->y + GY_MID) >> 6;
    GrpSurface_Blit({(x - dx[p->GrpID]), (y - dy[p->GrpID])}, sid,
                    src[p->GrpID]);
  }
}

void EffectManager::SendCmdStg4Rocks(uint8_t Cmd, uint8_t Param) {
  switch (Cmd) {
  case STG4ROCK_LEAVE: {
    for (auto &it : rocks) {
      it.State = STG4ROCK_LEAVE;
    }
  } break;

  case STG4ROCK_END: {
    for (auto &it : rocks) {
      it.State = STG4ROCK_END;
    }
  } break;

  case STG4ROCK_ACCMOVE1: {
    for (auto &it : rocks) {
      it.State = STG4ROCK_ACCMOVE1;
      it.a = (it.v / 24); // ((3 - p->GrpID) * 3);
      it.count = 0;
    }
  } break;

  case STG4ROCK_ACCMOVE2: {
    for (auto &it : rocks) {
      it.State = STG4ROCK_ACCMOVE2;
      it.a = (it.v / 12); // 24; // ((3 - p->GrpID) * 3);
      it.count = 0;
    }
    //		for(auto& it : rocks) {
    //                         it.State = STG4ROCK_ACCMOVE2;
    //                         it.a = -4;
    //                         it.count = 0;
    //                         it.v = 10;
    //                         it.d = Param;
    //                 }
  } break;

  default:
    break;
  }
}

// S6RASTER_MAX, S6STAR_MAX, S3STAR_MAX, Stg6Raster, Stg6Star → effect_manager.h
// s6_ras[], s6_stars[] → effect_manager.cpp

// Stage 6 raster init
void EffectManager::InitStg6Rasters() {
  int i = 0;

  for (i = 0; i < S6RASTER_MAX; i++) {
    s6_ras[i].x = (rnd() % (640 - 256)) + 128;
    s6_ras[i].y = -rnd() % (480 + 160); //(480+240)-240;
    s6_ras[i].deg = rnd();
    s6_ras[i].type = i % 3;
    s6_ras[i].amp = (rnd() % 80) + 70;
    s6_ras[i].vy = 2 + (rnd() % 3);
  }

  for (i = 0; i < S6STAR_MAX; i++) {
    s6_stars[i].x = (rnd() % (640 - 256)) + 128;
    s6_stars[i].y = rnd() % 480;
    s6_stars[i].vy = (rnd() % 20) + 8;
  }
}

// Stage 6 raster movement
void EffectManager::MoveStg6Rasters() {
  int i = 0;

  for (i = 0; i < S6RASTER_MAX; i++) {
    if ((i & 1) != 0) {
      s6_ras[i].deg += 2;
    } else {
      s6_ras[i].deg -= 2;
    }

    s6_ras[i].y += s6_ras[i].vy;

    if (s6_ras[i].y > 480) {
      s6_ras[i].x = (rnd() % (640 - 256)) + 128;
      s6_ras[i].y = -160;
      s6_ras[i].deg = rnd();
      s6_ras[i].amp = (rnd() % 80) + 70;
    }
  }

  for (i = 0; i < S6STAR_MAX; i++) {
    s6_stars[i].y += s6_stars[i].vy;

    if (s6_stars[i].y > 480) {
      s6_stars[i].x = (rnd() % (640 - 256)) + 128;
      s6_stars[i].y -= 480;
      s6_stars[i].vy = (rnd() % 20) + 8;
    }
  }
}

// Stage 6 raster draw
void EffectManager::DrawStg6Rasters() {
  constexpr auto sid = SURFACE_ID::MAPCHIP;
  static PIXEL_LTRB Target[3] = {
      {608, 272, 640, 352},
      {592, 160, 640, 272},
      {576, 0, 640, 160},
  };

  PIXEL_LTRB src;
  int i = 0;
  int j = 0;
  int h = 0;
  int w = 0;
  int x1 = 0;
  int x2 = 0;
  int dx = 0;
  int oy = 0;

  for (i = 0; i < S6STAR_MAX; i++) {
    src = {624, 352, (624 + 16), (352 + 16)};
    GrpSurface_Blit({s6_stars[i].x, s6_stars[i].y}, sid, src);
  }

  for (const auto &it : s6_ras) {
    x1 = Target[it.type].left;
    x2 = Target[it.type].right;
    oy = Target[it.type].top;
    h = (Target[it.type].bottom - Target[it.type].top);
    w = (x2 - x1) / 2;
    for (j = 0; j < h; j += 2) {
      dx = sinl((it.deg + j), it.amp);
      src = {x1, (j + oy), x2, (j + 2)};
      GrpSurface_Blit({(it.x + dx - w), (it.y + j)}, sid, src);
    }
  }
}

// Stage 3 fast star init
void EffectManager::InitStg3Stars() {
  int i = 0;

  for (i = 0; i < S3STAR_MAX; i++) {
    s6_stars[i].x = (rnd() % (640 - 256)) + 128;
    s6_stars[i].y = rnd() % 480;
    s6_stars[i].vy = (rnd() % 20) + 8;
  }
}

// Stage 3 fast star movement
void EffectManager::MoveStg3Stars() {
  int i = 0;

  for (i = 0; i < S3STAR_MAX; i++) {
    s6_stars[i].y += s6_stars[i].vy;

    if (s6_stars[i].y > 480) {
      s6_stars[i].x = (rnd() % (640 - 256)) + 128;
      s6_stars[i].y -= 480;
      s6_stars[i].vy = (rnd() % 20) + 8;
    }
  }
}

// Stage 3 fast star draw
void EffectManager::DrawStg3Stars() {
  int i = 0;

  for (i = 0; i < S3STAR_MAX; i++) {
    constexpr PIXEL_LTRB src = {(640 - 16), 0, 640, 16};
    GrpSurface_Blit({s6_stars[i].x, s6_stars[i].y}, SURFACE_ID::MAPCHIP, src);
  }
}
