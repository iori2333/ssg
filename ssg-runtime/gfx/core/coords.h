///
/// Coordinate systems
///
#pragma once

#include <compare>

// Pixel-space coordinates
// -----------------------
struct PixelPoint {
  int x{};
  int y{};

  constexpr explicit operator bool() const { return x > 0 && y > 0; }

  constexpr PixelPoint operator+(const PixelPoint &other) const {
    return {(x + other.x), (y + other.y)};
  }

  constexpr PixelPoint operator-(const PixelPoint &other) const {
    return {(x - other.x), (y - other.y)};
  }

  constexpr PixelPoint operator*(int factor) const {
    return {(x * factor), (y * factor)};
  }

  constexpr PixelPoint operator/(int divisor) const {
    return {(x / divisor), (y / divisor)};
  }

  constexpr PixelPoint operator/(const PixelPoint &other) const {
    return {(x / other.x), (y / other.y)};
  }

  constexpr PixelPoint &operator+=(const PixelPoint &other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  constexpr PixelPoint &operator-=(const PixelPoint &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  std::partial_ordering operator<=>(const PixelPoint &) const = default;
};
// -----------------------

// World-space coordinates
// -----------------------
// Fixed-point gameplay coordinates. One pixel is 64 raw world units: entering
// world space multiplies pixels by 64, while rendering divides by 64.

constexpr auto kWorldCoordBits = 6;

inline constexpr int kWorldCoordScale = 1 << kWorldCoordBits;

class WorldCoord {
public:
  constexpr WorldCoord() = default;

  [[nodiscard]] static constexpr WorldCoord FromPixels(int pixels) {
    return FromRaw(pixels * kWorldCoordScale);
  }

  [[nodiscard]] static constexpr WorldCoord FromRaw(int raw) {
    return WorldCoord(raw, RawTag{});
  }

  [[nodiscard]] constexpr int ToPixels() const {
    return raw_ >> kWorldCoordBits;
  }

  [[nodiscard]] constexpr int ToPixelsCeil() const {
    return raw_ / kWorldCoordScale + (raw_ % kWorldCoordScale > 0 ? 1 : 0);
  }

  [[nodiscard]] constexpr float ToPixelsFloat() const {
    return static_cast<float>(raw_) / static_cast<float>(kWorldCoordScale);
  }

  [[nodiscard]] constexpr int Raw() const { return raw_; }

  [[nodiscard]] constexpr WorldCoord Abs() const {
    return raw_ < 0 ? FromRaw(-raw_) : *this;
  }

  [[nodiscard]] constexpr WorldCoord DivPow2Floor(unsigned int bits) const {
    return FromRaw(raw_ >> bits);
  }

  [[nodiscard]] constexpr int Ratio(WorldCoord divisor) const {
    return raw_ / divisor.raw_;
  }

  [[nodiscard]] constexpr float RatioFloat(WorldCoord divisor) const {
    return static_cast<float>(raw_) / static_cast<float>(divisor.raw_);
  }

  [[nodiscard]] constexpr WorldCoord operator-() const {
    return FromRaw(-raw_);
  }

  constexpr WorldCoord &operator+=(WorldCoord other) {
    raw_ += other.raw_;
    return *this;
  }

  constexpr WorldCoord &operator-=(WorldCoord other) {
    raw_ -= other.raw_;
    return *this;
  }

  constexpr auto operator<=>(const WorldCoord &) const = default;

  friend constexpr WorldCoord operator+(WorldCoord lhs, WorldCoord rhs) {
    return FromRaw(lhs.raw_ + rhs.raw_);
  }

  friend constexpr WorldCoord operator-(WorldCoord lhs, WorldCoord rhs) {
    return FromRaw(lhs.raw_ - rhs.raw_);
  }

  friend constexpr WorldCoord operator*(WorldCoord coord, int factor) {
    return FromRaw(coord.raw_ * factor);
  }

  friend constexpr WorldCoord operator*(int factor, WorldCoord coord) {
    return coord * factor;
  }

  friend constexpr WorldCoord operator/(WorldCoord coord, int divisor) {
    return FromRaw(coord.raw_ / divisor);
  }

private:
  struct RawTag {};

  constexpr WorldCoord(int raw, RawTag) : raw_(raw) {}

  int raw_{};
};

constexpr WorldCoord PixelToWorld(int v) { return WorldCoord::FromPixels(v); }

// Pixel literals expressed in world units, e.g. 1.5_px == 96.
consteval WorldCoord operator""_px(unsigned long long pixels) {
  return WorldCoord::FromRaw(static_cast<int>(
      pixels * static_cast<unsigned long long>(kWorldCoordScale)));
}

consteval WorldCoord operator""_px(long double pixels) {
  return WorldCoord::FromRaw(static_cast<int>(pixels * kWorldCoordScale));
}

struct WorldPoint {
  WorldCoord x{};
  WorldCoord y{};

  constexpr WorldPoint() noexcept = default;

  constexpr WorldPoint(WorldCoord x, WorldCoord y) : x(x), y(y) {}

  explicit constexpr WorldPoint(const PixelPoint &pixel)
      : x(WorldCoord::FromPixels(pixel.x)), y(WorldCoord::FromPixels(pixel.y)) {
  }

  WorldPoint &operator-=(const WorldPoint &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  // Assumes this to be a centered coordinate, and calculates the
  // corresponding top-left coordinate based on the given size.
  [[nodiscard]] PixelPoint ToPixel(PixelPoint size_if_centered = {
                                       .x = 0, .y = 0}) const {
    return {
        .x = (x.ToPixels() - (size_if_centered.x >> 1)),
        .y = (y.ToPixels() - (size_if_centered.y >> 1)),
    };
  }
};
// ---------------------------
