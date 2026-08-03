///
/// Common graphics interface, independent of a specific rendering API
///

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <webp/encode.h>

#include "core/constants.h"
#include "core/coords.h"
#include "core/pixelformat.h"
#include "graphics.h"
#include "image/format_bmp.h"
#include "render/graphics_sdl.h"

#include "util/guard.h"

namespace {

struct GraphicsState {
  int frame_rate_divisor = 0;
  int screenshot_effort = 0;
  unsigned int screenshot_number = 0;
  std::string screenshot_prefix;
  bool screenshot_requested = false;
};

GraphicsState &State() {
  static GraphicsState state;
  return state;
}

} // namespace

void SetFrameRateDivisor(int divisor) { State().frame_rate_divisor = divisor; }

int FrameRateDivisor() { return State().frame_rate_divisor; }

// Paletted graphics //
// ----------------- //

// Screenshots
// -----------

namespace {
void ScreenshotFindLastFor(std::string_view ext) {
  auto &state = State();
  const auto extension_matches = [ext](std::string_view candidate) {
    return std::ranges::equal(candidate, ext, [](char left, char right) {
      return std::tolower(static_cast<unsigned char>(left)) ==
             std::tolower(static_cast<unsigned char>(right));
    });
  };

  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator{state.screenshot_prefix, error}) {
    if (error || !entry.is_regular_file(error) ||
        !extension_matches(entry.path().extension().string())) {
      continue;
    }
    const auto stem = entry.path().stem().string();
    unsigned int number = 0;
    const auto [end, result] =
        std::from_chars(stem.data(), stem.data() + stem.size(), number);
    if (result == std::errc{} && end == stem.data() + stem.size() &&
        number < std::numeric_limits<unsigned int>::max()) {
      state.screenshot_number = (std::max)(state.screenshot_number, number + 1);
    }
  }
}
} // namespace

void GraphicsScreenshotSetPrefix(std::string_view prefix) {
  State().screenshot_prefix = prefix;
}

// Increments the screenshot number to the next file with the given extension
// that doesn't exist yet, then opens a write stream for that file.
namespace {
SDL_IOStream *GraphicsNextScreenshotStream(std::string_view ext) {
  auto &state = State();
  if (state.screenshot_prefix.empty()) {
    return nullptr;
  }

  // Users might delete the directory while the game is running, after all.
  std::error_code error;
  std::filesystem::create_directories(state.screenshot_prefix, error);
  if (error) {
    state.screenshot_prefix.clear();
    return nullptr;
  }

  // Prevent the theoretical infinite loop...
  while (state.screenshot_number < std::numeric_limits<unsigned int>::max()) {
    const auto prefix_len = state.screenshot_prefix.size();
    state.screenshot_prefix += std::format("{:04}", state.screenshot_number++);
    state.screenshot_prefix += ext;
    auto *ret = SDL_IOFromFile(state.screenshot_prefix.c_str(), "wxb");
    state.screenshot_prefix.resize(prefix_len);
    if (ret != nullptr) {
      return ret;
    }
    if (state.screenshot_number == 1) {
      ScreenshotFindLastFor(ext);
    }
  }
  return nullptr;
}

bool ScreenshotSaveBMP(SDL_Surface *src) {
  auto *const stream = GraphicsNextScreenshotStream(".BMP");
  if (stream == nullptr) {
    return false;
  }
  auto stream_guard = util::MakeGuard(stream, SDL_CloseIO);

  // SDL_SaveBMP_IO() is very slow and unoptimized, especially on Windows
  // where SDL_IOStream still uses unbuffered writes as of SDL 3.2.24. For
  // now, we only use it if we absolutely have to.
  if (!BmpSaveSupports(src->format)) {
    return SDL_SaveBMP_IO(src, stream, false);
  }

  std::array<Bgra, kBmpPaletteSizeMax> bgra_memory{};
  const auto palette = [src, &bgra_memory]() -> std::span<Bgra> {
    const auto *palette = SDL_GetSurfacePalette(src);
    if (!palette) {
      return {};
    }
    if (std::cmp_not_equal(palette->ncolors, bgra_memory.size())) {
      return {};
    }
    for (const int i : std::views::iota(0, palette->ncolors)) {
      bgra_memory[i] = {
          .b = palette->colors[i].b,
          .g = palette->colors[i].g,
          .r = palette->colors[i].r,
          .a = palette->colors[i].a,
      };
    }
    return bgra_memory;
  }();
  if (src->format == SDL_PIXELFORMAT_INDEX8 && palette.empty()) {
    return SDL_SaveBMP_IO(src, stream, false);
  }

  const PixelPoint bmp_size = {.x = src->w, .y = -src->h};

  // SDL_BITSPERPIXEL() for `SDL_PIXELFORMAT_XRGB8888` would return 24, not
  // 32!
  const auto bpp = (SDL_BYTESPERPIXEL(src->format) * 8);

  const auto pixels = std::span(
      static_cast<uint8_t *>(src->pixels),
      (static_cast<size_t>(src->h) * static_cast<size_t>(src->pitch)));
  return BmpSave(stream, bmp_size, 1, bpp, palette, pixels);
}

bool ScreenshotSaveWebP(SDL_Surface *src, int z) {
  if ((src->w > WEBP_MAX_DIMENSION) || (src->h > WEBP_MAX_DIMENSION)) {
    return false;
  }

  WebPPicture pic;
  if (WebPPictureInit(&pic) == 0) {
    return false;
  }
  auto pic_guard = util::MakeGuard(&pic, WebPPictureFree);

  pic.width = src->w;
  pic.height = src->h;
  pic.argb_stride = src->w;

  // Must also be set to opt into lossless import!
  pic.use_argb = 1;

  decltype(WebPPictureImportRGBX) *import_func_32bpp = nullptr;
  switch (src->format) {
  case SDL_PIXELFORMAT_ARGB8888:
    // Yup, "argb" is little-endian and this is actually Bgra...
    pic.argb = static_cast<uint32_t *>(src->pixels);
    break;
  case SDL_PIXELFORMAT_XRGB8888:
    // … but these are big-endian!
    import_func_32bpp = WebPPictureImportBGRX;
    break;
  case SDL_PIXELFORMAT_ABGR8888:
    import_func_32bpp = WebPPictureImportRGBA;
    break;
  case SDL_PIXELFORMAT_INDEX8: {
    // The WebP repo has equivalent code in WebPImportColorMappedARGB(),
    // but Linux distributions typically don't package the `extras` module
    // this function belongs to.
    const auto *palette = SDL_GetSurfacePalette(src);
    if (palette == nullptr) {
      return false;
    }
    if (std::cmp_not_equal(palette->ncolors, sizeof(uint8_t) << 8)) {
      return false;
    }
    if (WebPPictureAlloc(&pic) == 0) {
      return false;
    }
    auto *src_p = static_cast<uint8_t *>(src->pixels);
    auto *dst_p = pic.argb;
    for (const auto y : std::views::iota(0, pic.height)) {
      for (const auto x : std::views::iota(0, pic.width)) {
        const auto c = palette->colors[src_p[x]];
        dst_p[x] = ((c.b << 0) | (c.g << 8) | (c.r << 16) | 0xFF000000);
      }
      src_p += src->w;
      dst_p += pic.argb_stride;
    }
    break;
  }
  default:
    // Note how SDL_ConvertPixels() doesn't take a palette parameter and
    // therefore can't cover the 8-bit case.
    if ((WebPPictureAlloc(&pic) == 0) ||
        !SDL_ConvertPixels(
            src->w, src->h, src->format, src->pixels, src->pitch,
            SDL_PIXELFORMAT_ARGB8888, pic.argb,
            (pic.argb_stride * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ARGB8888)))) {
      return false;
    }
    break;
  }
  if (import_func_32bpp != nullptr) {
    const auto *bytes = static_cast<const uint8_t *>(src->pixels);
    if (import_func_32bpp(&pic, bytes, (src->w * sizeof(uint32_t))) == 0) {
      return false;
    }
  }

  WebPConfig config;
  if (WebPConfigInit(&config) == 0) {
    return false;
  }
  if (WebPConfigLosslessPreset(&config, z) == 0) {
    return false;
  }
  config.thread_level = 1;

  WebPMemoryWriter wrt;
  WebPMemoryWriterInit(&wrt);
  auto wrt_guard = util::MakeGuard(&wrt, WebPMemoryWriterClear);
  pic.writer = WebPMemoryWrite;
  pic.custom_ptr = &wrt;

  const auto ret = WebPEncode(&config, &pic);
  if (ret == 0) {
    return false;
  }
  auto *const stream = GraphicsNextScreenshotStream(".webp");
  if (stream == nullptr) {
    return false;
  }
  auto stream_guard2 = util::MakeGuard(stream, SDL_CloseIO);
  return SDL_WriteIO(stream, wrt.mem, wrt.size) == wrt.size;
}
} // namespace

bool GraphicsScreenshotSave(SDL_Surface *src) {
  if (src == nullptr || src->w <= 0 || src->h <= 0) {
    return false;
  }
  const bool must_lock = SDL_MUSTLOCK(src);
  if (must_lock && !SDL_LockSurface(src)) {
    return false;
  }
  auto unlock = util::MakeGuard([&] {
    if (must_lock) {
      SDL_UnlockSurface(src);
    }
  });

  auto ret = false;
  const auto effort = State().screenshot_effort;
  if (effort != 0) {
    ret = ScreenshotSaveWebP(src, (effort - 1));
  }
  if (!ret) {
    ret = ScreenshotSaveBMP(src);
  }

  return ret;
}

void GraphicsScreenshotSetEffort(int effort) {
  State().screenshot_effort = std::clamp(effort, 0, kScreenshotEffortMax);
}
// -----------

int GraphicsParams::Scale4x() const {
  if (fullscreen) {
    return exclusive_fullscreen ? 4 : 0;
  }
  return window_scale_quarters;
}

PixelPoint GraphicsParams::ScaledRes() const {
  if (fullscreen) {
    if (exclusive_fullscreen) {
      return kGameResolution;
    }
    const auto display_s = SdlGraphicsDisplaySize(true);
    switch (fullscreen_fit) {
    case GraphicsFullscreenFit::Integer: {
      const auto factors = (display_s / kGameResolution);
      return (kGameResolution * std::min(factors.x, factors.y));
    }
    case GraphicsFullscreenFit::Aspect: {
      const auto factor_w =
          (static_cast<float>(display_s.x) / kGameResolution.x);
      const auto factor_h =
          (static_cast<float>(display_s.y) / kGameResolution.y);
      const auto scale = std::min(factor_w, factor_h);
      return {
          .x = static_cast<int>(kGameResolution.x * scale),
          .y = static_cast<int>(kGameResolution.y * scale),
      };
    }
    case GraphicsFullscreenFit::Stretch:
      return display_s;
    case GraphicsFullscreenFit::Count:
      std::unreachable();
    }
  }
  const auto scale = ((window_scale_quarters == 0) ? GraphicsWindowScale4xMax()
                                                   : window_scale_quarters);
  return ((kGameResolution * scale) / 4);
}

int GraphicsWindowScale4xMax() {
  const auto factors = ((SdlGraphicsDisplaySize(false) * 4) / kGameResolution);
  return std::max(1, std::min(factors.x, factors.y));
}

std::optional<GraphicsInitResult>
GraphicsInit(std::optional<const GraphicsParams> maybe_prev,
             GraphicsParams params) {
  const auto api_count = GraphicsRenderDriverCount();
  if (params.render_driver < -1 ||
      ((api_count > 0) && (params.render_driver >= api_count))) {
    params.render_driver = -1;
  }
  params.exclusive_fullscreen &= params.fullscreen;
  if (params.fullscreen_fit >= GraphicsFullscreenFit::Count) {
    params.fullscreen_fit = GraphicsFullscreenFit::Integer;
  }
  params.window_scale_quarters =
      std::clamp(params.window_scale_quarters, 0, GraphicsWindowScale4xMax());
  return SdlGraphicsInit(maybe_prev, params);
}

std::optional<GraphicsInitResult>
GraphicsInitOrFallback(GraphicsParams params) {
  if (const auto ret = GraphicsInit(std::nullopt, params)) {
    return ret;
  }

  // Start with the defaults and try looking for a different working
  // configuration
  const auto api_count = GraphicsRenderDriverCount();

  const auto api_it =
      ((api_count > 0)
           ? std::views::iota(-1, api_count)
           : std::views::iota(params.render_driver, params.render_driver + 1));

  const auto failed_driver = params.render_driver;
  for (const auto api : api_it) {
    if (api == failed_driver) {
      continue;
    }
    params.render_driver = api;
    if (const auto ret = GraphicsInit(std::nullopt, params)) {
      return ret;
    }
  }
  return std::nullopt;
}

void GraphicsFlip() {
  const auto &state = State();
  SdlGraphicsFlip(state.screenshot_requested &&
                  !state.screenshot_prefix.empty());
}

void GraphicsRequestScreenshot(bool requested) {
  State().screenshot_requested = requested;
}
