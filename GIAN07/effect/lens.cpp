///
/// Lens - Lens effect
///

#include <cassert>
#include <utility>

#include "lens.h"

#include "core/constants.h"
#include "gfx/graphics_backend.h"
#include "util/cast.h"
#include "util/ut_math.h"

// Create a lens with radius:r and bulge:m
std::optional<LensInfo> GrpCreateLensBall(uint16_t r, uint16_t m) {
  int dx = 0;
  int z = 0;
  int w = 0;

  // Since the surface pitch can be different than its with, [Table] will
  // still contain byte offsets, regardless of our main pixel format.
  const auto BitWeight = sizeof(uint32_t);

  assert(r > 0); // Invalid radius
  assert(r > m); // Bulge must be smaller than radius!

  const uint16_t Diameter = (r * 2);
  const uint16_t area = (Diameter * Diameter);

  LensInfo NewLens = {.r = r, .Height = Diameter};

  NewLens.FOV = std::unique_ptr<std::byte[]>(new (std::nothrow)
                                                 std::byte[area * BitWeight]);
  if (NewLens.FOV == nullptr) {
    return std::nullopt;
  }

// Not even restricting [Diameter] to int8_t would convince Visual Studio's
// static analyzer that this can't overflow.
#pragma warning(suppress : 26831)
  NewLens.Data = std::unique_ptr<uint32_t[]>(new (std::nothrow) uint32_t[area]);
  if (NewLens.Data == nullptr) {
    return std::nullopt;
  }

  auto *Table = NewLens.Data.get();
  const auto r2 = (Cast::up_sign<int32_t>(r) * r);
  const auto s = isqrt(r2 - (Cast::up_sign<int32_t>(m) * m));

  for (auto i = -Cast::up_sign<int32_t>(r); std::cmp_less(i, r); i++) {
    // Measure x coordinate
    dx = (s * s) - (i * i);

    if (dx > 0) { // Inside the circle
      dx = isqrt(dx);
      *Table = w = dx * 2;
      Table++; // Width
      *Table = (r - dx) * BitWeight;
      Table++; // Dx
    } else {   // Outside the circle
      w = 0;
      *Table = 0;
      Table++; // Width
      *Table = 0;
      Table++; // Dx
    }

    while ((w--) != 0) {
      z = (dx - w) * (dx - w);
      z = isqrt(r2 - z - (i * i));

      *Table = ((i * m) / z) + r;                     // y-coordinate
      *Table = (*Table * Diameter);                   // Multiply by width
      *Table = (*Table + ((((dx - w) * m) / z) + r)); // x-coordinate

      Table++;
    }
  }

  return NewLens;
}

// GrpLock()-family functions : Draw the lens ball
void LensInfo::Draw(WINDOW_POINT center) {
  // Adjust so (x,y) becomes the center
  const WINDOW_COORD left = (center.x - r);
  const WINDOW_COORD top = (center.y - r);

  if ((left < 0) || (top < 0) || ((left + Height) > (GRP_RES.w - 1)) ||
      ((top + Height) > (GRP_RES.h - 1))) {
    return;
  }

  GrpBackend_PixelAccessEdit([&]<class P>(std::byte *pixels, size_t pitch) {
    const auto fov_buffer = reinterpret_cast<P *>(FOV.get());
    const auto fov_size = (static_cast<size_t>(Height) * Height);
    const std::span fov = {fov_buffer, fov_size};

    auto *Table = Data.get(); // For table lookup
    auto *Dest = &pixels[(top * pitch) + (left * sizeof(P))];

    // Capture the pixels under the lens
    const auto *back_p = Dest;
    for (auto fov_p = fov.begin(); fov_p != fov.end(); fov_p += Height) {
      std::copy_n(reinterpret_cast<const P *>(back_p), Height, fov_p);
      back_p += pitch;
    }

    for (decltype(Height) row = 0; row < Height; row++) {
      auto Width = *(Table++);
      auto *p = reinterpret_cast<P *>(Dest + *(Table++));

      while (Width--) {
        *(p++) = fov[*(Table++)];
      }
      Dest += pitch;
    }
  });
}
