///
/// Coordinate systems
///
#pragma once

#include <compare>

// Pixel-space coordinates
// -----------------------
// The unscaled output space in the game's native resolution.

// X or Y value in unscaled pixel space. Relative to any origin.
using PixelCoord = int;

// X/Y coordinate in unscaled pixel space. Relative to any origin.
template <typename Coord> struct PixelPointBase {
  Coord x;
  Coord y;

  // Explicit integer division.
  PixelPointBase DivInt(int scalar) const {
    return {
        .x = static_cast<Coord>(static_cast<int>(x) / scalar),
        .y = static_cast<Coord>(static_cast<int>(y) / scalar),
    };
  }

  constexpr PixelPointBase operator+(const PixelPointBase &other) const {
    return {(x + other.x), (y + other.y)};
  }

  PixelPointBase &operator-=(const PixelPointBase &other) {
    this->x -= other.x;
    this->y -= other.y;
    return *this;
  }

  std::partial_ordering operator<=>(const PixelPointBase &) const = default;
};

// Area size in unscaled pixel space.
// Using signed integers to avoid complicating the conversion into rectangle
// types, where signed coordinates often represent meaningful points outside
// the screen.
template <typename Coord> struct PixelSizeBase {
  Coord w;
  Coord h;

  explicit operator bool() const { return ((w > 0) && (h > 0)); }

  constexpr PixelSizeBase operator-(const PixelPointBase<Coord> &other) const {
    return {(w - other.x), (h - other.y)};
  }

  constexpr PixelSizeBase operator*(int factor) const {
    return {(w * factor), (h * factor)};
  }

  constexpr PixelSizeBase operator/(int divisor) const {
    return {(w / divisor), (h / divisor)};
  }

  constexpr PixelSizeBase operator/(const PixelSizeBase &other) const {
    return {(w / other.w), (h / other.h)};
  }

  PixelSizeBase &operator+=(const PixelSizeBase &other) {
    w += other.w;
    h += other.h;
    return *this;
  }

  std::partial_ordering operator<=>(const PixelSizeBase &) const = default;
};

template <typename Coord>
PixelPointBase<Coord> operator+(const PixelPointBase<Coord> &self,
                                const PixelSizeBase<Coord> &other) {
  return {.x = (self.x + other.w), .y = (self.y + other.h)};
}

template <typename Coord>
PixelPointBase<Coord> operator-(const PixelPointBase<Coord> &self,
                                const PixelSizeBase<Coord> &other) {
  return {.x = (self.x - other.w), .y = (self.y - other.h)};
}

template <typename Coord>
PixelPointBase<Coord> &operator+=(PixelPointBase<Coord> &self,
                                  const PixelSizeBase<Coord> &other) {
  self.x += other.w;
  self.y += other.h;
  return self;
}

template <typename Coord>
PixelPointBase<Coord> &operator-=(PixelPointBase<Coord> &self,
                                  const PixelSizeBase<Coord> &other) {
  self.x -= other.w;
  self.y -= other.h;
  return self;
}

// Left-top-width-height rectangle in unscaled pixel space. Relative to any
// origin.
template <typename Coord> struct PixelLtwhBase {
  Coord left = 0;
  Coord top = 0;
  Coord w = 0;
  Coord h = 0;

  constexpr PixelLtwhBase() = default;
  constexpr PixelLtwhBase(const PixelLtwhBase &) = default;
  constexpr PixelLtwhBase(PixelLtwhBase &&) = default;
  constexpr PixelLtwhBase &operator=(const PixelLtwhBase &) = default;
  constexpr PixelLtwhBase &operator=(PixelLtwhBase &&) = default;
  constexpr PixelLtwhBase(Coord left, Coord top, Coord w, Coord h)
      : left(left), top(top), w(w), h(h) {}

  constexpr PixelLtwhBase operator+(const PixelPointBase<Coord> &other) const {
    return {(left + other.x), (top + other.y), w, h};
  }
};

// Left-top-right-bottom rectangle in unscaled pixel space. Relative to any
// origin.
template <typename Coord> struct PixelLtrbBase {
  Coord left;
  Coord top;
  Coord right;
  Coord bottom;

  PixelLtrbBase() = default;
  PixelLtrbBase(const PixelLtrbBase &) = default;
  PixelLtrbBase(PixelLtrbBase &&) = default;
  PixelLtrbBase &operator=(const PixelLtrbBase &) = default;
  PixelLtrbBase &operator=(PixelLtrbBase &&) = default;
  constexpr PixelLtrbBase(decltype(left) left, decltype(top) top,
                          decltype(right) right, decltype(bottom) bottom)
      : left(left), top(top), right(right), bottom(bottom) {}
  constexpr PixelLtrbBase(const PixelPointBase<Coord> &topleft,
                          const PixelSizeBase<Coord> &size)
      : left(topleft.x), top(topleft.y), right(topleft.x + size.w),
        bottom(topleft.y + size.h) {}

  constexpr PixelLtrbBase(const PixelLtwhBase<Coord> &o)
      : left(o.left), top(o.top), right(o.left + o.w), bottom(o.top + o.h) {}

  PixelSizeBase<Coord> Size() const { return {(right - left), (bottom - top)}; }
};

using WindowCoord = PixelCoord;

// X/Y coordinate in unscaled game window space. The visible area ranges from
// (0, 0) inclusive to [kGameResolution] exclusive.
template <typename Coord>
struct WindowPointBase : public PixelPointBase<Coord> {
  constexpr WindowPointBase
  operator+(const PixelPointBase<Coord> &other) const {
    return {(this->x + other.x), (this->y + other.y)};
  }

  WindowPointBase operator/(Coord scalar) const {
    return {(this->x / scalar), (this->y / scalar)};
  }
};

// Area size in unscaled game window space.
template <typename Coord> using WindowSizeBase = PixelSizeBase<Coord>;

// Left-top-width-height rectangle in unscaled game window space. The visible
// area ranges from (0, 0) inclusive to [kGameResolution] exclusive.
template <typename Coord> struct WindowLtwhBase : public PixelLtwhBase<Coord> {
  using PixelLtwhBase<Coord>::PixelLtwhBase;
};

// Left-top-right-bottom rectangle in unscaled game window space. The visible
// area ranges from (0, 0) inclusive to [kGameResolution] exclusive.
template <typename Coord> struct WindowLtrbBase : public PixelLtrbBase<Coord> {
  using PixelLtrbBase<Coord>::PixelLtrbBase;

  constexpr WindowLtrbBase(const WindowLtwhBase<Coord> &o)
      : PixelLtrbBase<Coord>(o) {}
};

using PixelPoint = PixelPointBase<PixelCoord>;
using PixelSize = PixelSizeBase<PixelCoord>;
using PixelLtwh = PixelLtwhBase<PixelCoord>;
using PixelLtrb = PixelLtrbBase<PixelCoord>;
using WindowPoint = WindowPointBase<WindowCoord>;
using WindowSize = WindowSizeBase<WindowCoord>;
using WindowLtwh = WindowLtwhBase<WindowCoord>;
using WindowLtrb = WindowLtrbBase<WindowCoord>;
// -----------------------

// World-space coordinates
// -----------------------
// Not exclusively used for the playfield.

constexpr auto kWorldCoordBits = 6;

using WorldCoord = int;

inline constexpr WorldCoord kWorldCoordScale = 1 << kWorldCoordBits;

inline constexpr WorldCoord PixelToWorld(PixelCoord v) {
  return v * kWorldCoordScale;
}

// Pixel literals expressed in world units, e.g. 1.5_px == 96.
consteval WorldCoord operator""_px(unsigned long long pixels) {
  return static_cast<WorldCoord>(pixels * kWorldCoordScale);
}

consteval WorldCoord operator""_px(long double pixels) {
  return static_cast<WorldCoord>(pixels * kWorldCoordScale);
}

struct WorldPoint {
  WorldCoord x{};
  WorldCoord y{};

  constexpr WorldPoint() noexcept = default;

  // World-space points should never be constructed from integer literals.
  // These literals may or may not be pixels, and usage code should not
  // assume or bother with [kWorldCoordBits]. To construct a WorldPoint
  // from a pixel literal, explicitly construct a PixelPoint first.
  WorldPoint(int x, int y) = delete;

  // TODO: Keeping this one around so that we can at least pass structure
  // fields while we gradually migrate the game to this structure, but it
  // should be `delete`d once we're done.
  WorldPoint(const WorldCoord *x, const WorldCoord *y) : x(*x), y(*y) {}

  WorldPoint(const PixelPoint &pixel)
      : x(pixel.x << kWorldCoordBits), y(pixel.y << kWorldCoordBits) {}

  [[nodiscard]] static constexpr WorldPoint FromWorld(WorldCoord x,
                                                      WorldCoord y) {
    WorldPoint point;
    point.x = x;
    point.y = y;
    return point;
  }

  WorldPoint &operator-=(const WorldPoint &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  // Assumes this to be a centered coordinate, and calculates the
  // corresponding top-left coordinate based on the given size.
  PixelPoint ToPixel(PixelSize size_if_centered = {0, 0}) const {
    return {
        .x = ((x >> kWorldCoordBits) - (size_if_centered.w >> 1)),
        .y = ((y >> kWorldCoordBits) - (size_if_centered.h >> 1)),
    };
  }
};
// ---------------------------
