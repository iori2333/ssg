///
/// Platform-specific graphics backend interface
///

#pragma once

#include <string_view>

#include "graphics.h"

#include "gfx/constants.h"

/// Enumeration and pre-initialization queries
/// ------------------------------------------

// Should initialize everything needed for device and API queries.
bool GraphicsBackendEnum();

// Rendering APIs.
int8_t GraphicsBackendAPICount();
std::string_view GraphicsBackendAPILabel(std::string_view api);

// Maps an API string back to its ID. Returns -1 for an unavailable API.
int GraphicsBackendAPIID(std::string_view api);

// Maps an API ID to its string representation. Returns the empty string for
// -1.
std::string_view GraphicsBackendAPIString(int8_t id);

// Returns the maximum usable display size in windowed or fullscreen mode.
PixelSize GraphicsBackendDisplaySize(bool fullscreen);
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

// Tries initializing the backend with the closest available configuration for
// the given [params], which can be assumed to be valid. Called after
// GraphicsBackendEnum(). Returns the actual configuration the backend is
// running and whether the call site must reinitialize any surfaces, or
// `std::nullopt` if no valid format could be found.
//
// If [maybe_prev] is valid, this call is supposed to reinitialize an already
// running backend with new parameters.
std::optional<GraphicsInitResult>
GraphicsBackendInit(std::optional<const GraphicsParams> maybe_prev,
                    GraphicsParams params);

// Normal cleanup (abnormal termination on failure)
void GraphicsBackendCleanup();
/// --------------------------

/// General
/// -------

// Clears the backbuffer with the given palettized or channeled color,
// depending on the mode.
void GraphicsBackendClear(
    uint8_t col_palettized = Rgb216{0, 0, 0}.PaletteIndex(),
    Rgb col_channeled = Rgb{.r = 0, .g = 0, .b = 0});

// Sets the clipping rectangle.
void GraphicsBackendSetClip(const WindowLtrb &rect);

// Returns the currently active rendering API.
std::string_view GraphicsBackendAPIString();

struct FileStreamWrite;
void GraphicsBackendFlip(bool take_screenshot);
/// -------

/// Surfaces
/// --------
struct BmpOwned;

// (Re-)creates the texture in the given surface slot with the given size
// and with undefined initial contents.
bool GraphicsSurfaceCreateUninitialized(SurfaceId sid, const PixelSize &size);

// Consumes the given .BMP file and sets the given surface to its contents,
// re-creating it in the correct size if necessary.
bool GraphicsSurfaceLoad(SurfaceId sid, BmpOwned bmp);

// Uploads [pixels] (consisting of a pointer and a row pitch) to a [subrect] of
// [sid]. [subrect] can be a `nullptr` to overwrite the entire texture. The
// [pixels] have to match the surface's format.
bool GraphicsSurfaceUpdate(
    SurfaceId sid, const PixelLtwh *subrect,
    std::tuple<const std::byte *, size_t> pixels) noexcept;

// Returns the size of the given surface.
PixelSize GraphicsSurfaceSize(SurfaceId sid);

// Blits the given [src] rectangle inside [sid] to the given top-left point
// on the backbuffer while clipping the destination rectangle to the clipping
// area. Returns `true` if any part of the sprite was blitted.
bool GraphicsSurfaceBlit(WindowPoint topleft, SurfaceId sid,
                         const PixelLtrb &src);

// Like GraphicsSurfaceBlit(), but ignores [sid]'s color key.
void GraphicsSurfaceBlitOpaque(WindowPoint topleft, SurfaceId sid,
                               const PixelLtrb &src);

// Temporarily tint all subsequent blits from [sid].
void GraphicsSurfaceSetColorMod(SurfaceId sid, uint8_t r, uint8_t g, uint8_t b);

/// --------

/// Geometry
/// --------

// Vertex types
// ------------

using VertexCoord = float;

using VertexXy = WindowPointBase<VertexCoord>;

struct VertexRgba {
  float r;
  float g;
  float b;
  float a;

  VertexRgba() = default;
  VertexRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r / 255.0F), g(g / 255.0F), b(b / 255.0F), a(a / 255.0F) {}
  VertexRgba(const Rgba &o)
      : r(o.r / 255.0F), g(o.g / 255.0F), b(o.b / 255.0F), a(o.a / 255.0F) {}
};

template <size_t N = std::dynamic_extent>
using VertexXySpan = std::span<const VertexXy, N>;
template <size_t N = std::dynamic_extent>
using VertexRgbaSpan = std::span<const VertexRgba, N>;

enum class TrianglePrimitive : uint8_t { Fan, Strip, Count };
// ------------

/// --------

/// Software rendering with pixel access
/// ------------------------------------
/// Separate rendering mode that provides read and write access to backbuffer
/// pixels before it's presented.

// Enters the software-rendered pixel access mode if necessary, and returns
// `true` if successful. Does nothing if the renderer is already in this mode.
// If a mode change occurred, all surfaces are invalidated.
bool GraphicsBackendPixelAccessStart();

// Leaves the software-rendered pixel access mode and returns to regular
// hardware-accelerated rendering if necessary, and returns `true` if
// successful. Does nothing if the renderer is already in hardware mode.
// If a mode change occurred, all surfaces are invalidated.
bool GraphicsBackendPixelAccessEnd();

// Locks the backbuffer, returning a pointer to its pixels and the row pitch.
// Should return a pitch of 0 on failure.
// On success, the returned buffer has a size of [kGameResolution.h] times the
// returned pitch, and always uses a 32-bit Bgra pixel format.
std::tuple<std::byte *, size_t> GraphicsBackendPixelAccessLock();

// Unlocks the backbuffer.
void GraphicsBackendPixelAccessUnlock();

// Calls [func] with a locked backbuffer.
void GraphicsBackendPixelAccessEdit(auto func) {
  const auto [pixels, pitch] = GraphicsBackendPixelAccessLock();
  if (pitch == 0) {
    return;
  }
  func.template operator()<uint32_t>(pixels, pitch);
  GraphicsBackendPixelAccessUnlock();
}
/// ------------------------------------
