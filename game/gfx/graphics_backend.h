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
bool GrpBackend_Enum(void);

// Rendering APIs.
int8_t GrpBackend_APICount(void);
std::string_view GrpBackend_APILabel(std::string_view api);

// Maps an API string back to its ID. Returns -1 for an unavailable API.
int8_t GrpBackend_APIID(std::string_view api);

// Maps an API ID to its string representation. Returns the empty string for
// -1.
std::string_view GrpBackend_APIString(int8_t id);

// Returns the maximum usable display size in windowed or fullscreen mode.
PIXEL_SIZE GrpBackend_DisplaySize(bool fullscreen);
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

// Tries initializing the backend with the closest available configuration for
// the given [params], which can be assumed to be valid. Called after
// GrpBackend_Enum(). Returns the actual configuration the backend is running
// and whether the call site must reinitialize any surfaces, or `std::nullopt`
// if no valid format could be found.
//
// If [maybe_prev] is valid, this call is supposed to reinitialize an already
// running backend with new parameters.
std::optional<GRAPHICS_INIT_RESULT>
GrpBackend_Init(std::optional<const GRAPHICS_PARAMS> maybe_prev,
                GRAPHICS_PARAMS params);

// Normal cleanup (abnormal termination on failure)
void GrpBackend_Cleanup(void);
/// --------------------------

/// General
/// -------

// Clears the backbuffer with the given palettized or channeled color,
// depending on the mode.
void GrpBackend_Clear(uint8_t col_palettized = RGB216{0, 0, 0}.PaletteIndex(),
                      RGB col_channeled = RGB{0, 0, 0});

// Sets the clipping rectangle.
void GrpBackend_SetClip(const WINDOW_LTRB &rect);

// Returns the currently active rendering API.
std::string_view GrpBackend_APIString(void);

struct FILE_STREAM_WRITE;
void GrpBackend_Flip(bool take_screenshot);
/// -------

/// Surfaces
/// --------
struct BMP_OWNED;

// (Re-)creates the texture in the given surface slot with the given size
// and with undefined initial contents.
bool GrpSurface_CreateUninitialized(SURFACE_ID sid, const PIXEL_SIZE &size);

// Consumes the given .BMP file and sets the given surface to its contents,
// re-creating it in the correct size if necessary.
bool GrpSurface_Load(SURFACE_ID sid, BMP_OWNED &&bmp);

// Uploads [pixels] (consisting of a pointer and a row pitch) to a [subrect] of
// [sid]. [subrect] can be a `nullptr` to overwrite the entire texture. The
// [pixels] have to match the surface's format.
bool GrpSurface_Update(SURFACE_ID sid, const PIXEL_LTWH *subrect,
                       std::tuple<const std::byte *, size_t> pixels) noexcept;

// Returns the size of the given surface.
PIXEL_SIZE GrpSurface_Size(SURFACE_ID sid);

// Blits the given [src] rectangle inside [sid] to the given top-left point
// on the backbuffer while clipping the destination rectangle to the clipping
// area. Returns `true` if any part of the sprite was blitted.
bool GrpSurface_Blit(WINDOW_POINT topleft, SURFACE_ID sid,
                     const PIXEL_LTRB &src);

// Like GrpSurface_Blit(), but ignores [sid]'s color key.
void GrpSurface_BlitOpaque(WINDOW_POINT topleft, SURFACE_ID sid,
                           const PIXEL_LTRB &src);

// Temporarily tint all subsequent blits from [sid].
void GrpSurface_SetColorMod(SURFACE_ID sid, uint8_t r, uint8_t g, uint8_t b);

/// --------

/// Geometry
/// --------

// Vertex types
// ------------

using VERTEX_COORD = float;

using VERTEX_XY = WINDOW_POINT_BASE<VERTEX_COORD>;

struct VERTEX_RGBA {
  float r;
  float g;
  float b;
  float a;

  VERTEX_RGBA() = default;
  VERTEX_RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {}
  VERTEX_RGBA(const RGBA &o)
      : r(o.r / 255.0f), g(o.g / 255.0f), b(o.b / 255.0f), a(o.a / 255.0f) {}
};

template <size_t N = std::dynamic_extent>
using VERTEX_XY_SPAN = std::span<const VERTEX_XY, N>;
template <size_t N = std::dynamic_extent>
using VERTEX_RGBA_SPAN = std::span<const VERTEX_RGBA, N>;

enum class TRIANGLE_PRIMITIVE : uint8_t { FAN, STRIP, COUNT };
// ------------

class GraphicsGeometry;

// Must be kept in sync with the hardcoded ones in the SDL_GL_ResetAttributes()
// implementation.
#define OPENGL_TARGET_CORE_MAJ 2
#define OPENGL_TARGET_CORE_MIN 1
#define OPENGL_TARGET_ES1_MIN 1
#define OPENGL_TARGET_ES2_MIN 0

extern GraphicsGeometry GrpGeomSDL;
inline GraphicsGeometry *const GrpGeom = &GrpGeomSDL;
/// --------

/// Software rendering with pixel access
/// ------------------------------------
/// Separate rendering mode that provides read and write access to backbuffer
/// pixels before it's presented.

// Enters the software-rendered pixel access mode if necessary, and returns
// `true` if successful. Does nothing if the renderer is already in this mode.
// If a mode change occurred, all surfaces are invalidated.
bool GrpBackend_PixelAccessStart(void);

// Leaves the software-rendered pixel access mode and returns to regular
// hardware-accelerated rendering if necessary, and returns `true` if
// successful. Does nothing if the renderer is already in hardware mode.
// If a mode change occurred, all surfaces are invalidated.
bool GrpBackend_PixelAccessEnd(void);

// Locks the backbuffer, returning a pointer to its pixels and the row pitch.
// Should return a pitch of 0 on failure.
// On success, the returned buffer has a size of [GRP_RES.h] times the returned
// pitch, and always uses a 32-bit BGRA pixel format.
std::tuple<std::byte *, size_t> GrpBackend_PixelAccessLock(void);

// Unlocks the backbuffer.
void GrpBackend_PixelAccessUnlock(void);

// Calls [func] with a locked backbuffer.
void GrpBackend_PixelAccessEdit(auto func) {
  const auto [pixels, pitch] = GrpBackend_PixelAccessLock();
  if (pitch == 0) {
    return;
  }
  func.template operator()<uint32_t>(pixels, pitch);
  GrpBackend_PixelAccessUnlock();
}
/// ------------------------------------

#include "graphics_sdl.h"
