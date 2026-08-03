///
/// SDL_ttf-based text rasterizer
///

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/core/pixelformat.h"
#include "sys/log.h"
#include "util/enum_array.h"
#include "util/guard.h"
#include "util/sdl_resource.h"

constexpr auto kJapaneseFamily = "Noto Sans Mono CJK JP";
constexpr auto kSimplifiedChineseFamily = "Noto Sans Mono CJK SC";
#ifdef WIN32
constexpr auto kWindowsJapaneseFontRelativePath = "/Fonts/msgothic.ttc";
constexpr auto kWindowsSimplifiedChineseFontRelativePath = "/Fonts/simsun.ttc";
constexpr auto kWindowsJapaneseFace = 0;
constexpr auto kWindowsSimplifiedChineseFace = 0;
#else
constexpr auto kCjkFontEnvironmentVariable = "SSG_CJK_FONT";
constexpr std::array<std::string_view, 4> kLinuxCjkFontPaths = {
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/google-noto-sans-cjk-fonts/NotoSansCJK-Regular.ttc",
};
#endif
constexpr auto kDefaultDpi = 72;

struct FontSpec {
  int pixel_size;
  bool bold;
};

inline constinit const util::EnumArray<FontSpec, FontId> kFontSpecs = {
    FontSpec{.pixel_size = 14, .bold = false},
    FontSpec{.pixel_size = 16, .bold = false},
    FontSpec{.pixel_size = 24, .bold = true},
    FontSpec{.pixel_size = 10, .bold = false},
};

inline std::string NormalizeLanguage(std::string_view language) {
  return language == "zh" ? "zh-CN" : "ja";
}

constexpr bool UsesSimplifiedChineseGlyphForm(uint32_t codepoint) {
  return (codepoint >= 0x2E80U && codepoint <= 0x303FU) ||
         (codepoint >= 0x31C0U && codepoint <= 0x31EFU) ||
         (codepoint >= 0x3400U && codepoint <= 0x4DBFU) ||
         (codepoint >= 0x4E00U && codepoint <= 0x9FFFU) ||
         (codepoint >= 0xF900U && codepoint <= 0xFAFFU) ||
         (codepoint >= 0xFE10U && codepoint <= 0xFE4FU) ||
         (codepoint >= 0xFF00U && codepoint <= 0xFFEFU) ||
         (codepoint >= 0x20000U && codepoint <= 0x2FA1FU);
}

class TextState {
  std::string cjk_font_path_;
#ifdef WIN32
  std::string windows_japanese_font_path_;
  std::string windows_simplified_chinese_font_path_;
#endif
  std::string language_ = "ja";
  int japanese_face_ = -1;
  int simplified_chinese_face_ = -1;
  bool initialized_ = false;
  util::SdlResource<TTF_TextEngine, TTF_DestroySurfaceTextEngine> engine_;
  util::SdlResource<SDL_Surface, SDL_DestroySurface> scratch_;
  util::EnumArray<TTF_Font *, FontId> fonts_{};
  util::EnumArray<TTF_Font *, FontId> fallback_fonts_{};

  static TTF_Font *OpenFont(std::string_view path, int face, float size) {
    const auto properties = SDL_CreateProperties();
    if (properties == 0) {
      return nullptr;
    }
    auto properties_guard = util::MakeGuard(properties, SDL_DestroyProperties);
    if (!SDL_SetStringProperty(properties, TTF_PROP_FONT_CREATE_FILENAME_STRING,
                               path.data()) ||
        !SDL_SetFloatProperty(properties, TTF_PROP_FONT_CREATE_SIZE_FLOAT,
                              size) ||
        !SDL_SetNumberProperty(properties, TTF_PROP_FONT_CREATE_FACE_NUMBER,
                               face)) {
      return nullptr;
    }
    return TTF_OpenFontWithProperties(properties);
  }

  static std::optional<int> FindFace(std::string_view path,
                                     std::string_view wanted_family) {
    auto *first = OpenFont(path, 0, 12.0F);
    if (first == nullptr) {
      return std::nullopt;
    }
    const auto first_guard = util::MakeGuard(first, TTF_CloseFont);
    const int face_count = TTF_GetNumFontFaces(first);
    for (int face = 0; face < face_count; ++face) {
      auto *font = OpenFont(path, face, 12.0F);
      if (font == nullptr) {
        continue;
      }
      const auto font_guard = util::MakeGuard(font, TTF_CloseFont);
      const auto *family = TTF_GetFontFamilyName(font);
      if (family != nullptr && family == wanted_family) {
        return face;
      }
    }
    return std::nullopt;
  }

  void ConfigureFont(TTF_Font *font, const FontSpec &spec) const {
    const int initial_height = TTF_GetFontHeight(font);
    if (initial_height > 0) {
      int vertical_dpi = static_cast<int>(
          std::lround(static_cast<double>(kDefaultDpi * spec.pixel_size) /
                      static_cast<double>(initial_height)));
      vertical_dpi = std::max(vertical_dpi, 1);

      for (int attempt = 0; attempt < 8; ++attempt) {
        if (!TTF_SetFontSizeDPI(font, static_cast<float>(spec.pixel_size),
                                kDefaultDpi, vertical_dpi)) {
          break;
        }
        const int height = TTF_GetFontHeight(font);
        if (height == spec.pixel_size) {
          break;
        }
        vertical_dpi += (height < spec.pixel_size) ? 1 : -1;
        vertical_dpi = std::max(vertical_dpi, 1);
      }
    }
#ifdef WIN32
    TTF_SetFontHinting(font, TTF_HINTING_MONO);
#else
    TTF_SetFontHinting(font, TTF_HINTING_NORMAL);
#endif
    TTF_SetFontStyle(font, spec.bold ? TTF_STYLE_BOLD : TTF_STYLE_NORMAL);
    (void)TTF_SetFontLanguage(font, language_.c_str());
  }

  void CleanupFonts() {
    for (auto *font : fonts_) {
      if (font != nullptr) {
        TTF_ClearFallbackFonts(font);
      }
    }
    for (auto &font : fonts_) {
      TTF_CloseFont(font);
      font = nullptr;
    }
    for (auto &font : fallback_fonts_) {
      TTF_CloseFont(font);
      font = nullptr;
    }
  }

  [[nodiscard]] std::string_view ActivePrimaryFontPath() const {
#ifdef WIN32
    return windows_japanese_font_path_;
#else
    return cjk_font_path_;
#endif
  }

  [[nodiscard]] std::string_view ActiveFallbackFontPath() const {
#ifdef WIN32
    return windows_simplified_chinese_font_path_;
#else
    return cjk_font_path_;
#endif
  }

  [[nodiscard]] TTF_Font *FontForCodepoint(FontId id,
                                           uint32_t codepoint) const {
#ifdef WIN32
    if (language_ == "zh-CN" && UsesSimplifiedChineseGlyphForm(codepoint)) {
      return fallback_fonts_[id];
    }
#endif
    return fonts_[id];
  }

  template <typename Func>
  [[nodiscard]] bool ForEachFontRun(FontId id, std::string_view text,
                                    Func &&func) const {
    const char *cursor = text.data();
    size_t remaining = text.size();
    const char *run_start = cursor;
    TTF_Font *run_font = nullptr;

    while (remaining > 0) {
      const char *character_start = cursor;
      const auto codepoint = SDL_StepUTF8(&cursor, &remaining);
      auto *font = FontForCodepoint(id, codepoint);
      if (run_font == nullptr) {
        run_font = font;
      } else if (font != run_font) {
        if (!func(run_font,
                  std::string_view(run_start, character_start - run_start))) {
          return false;
        }
        run_start = character_start;
        run_font = font;
      }
    }
    return run_font == nullptr ||
           func(run_font, std::string_view(run_start, cursor - run_start));
  }

  bool LoadFonts() {
    CleanupFonts();
    const bool simplified_chinese = language_ == "zh-CN";
    int primary_face =
        simplified_chinese ? simplified_chinese_face_ : japanese_face_;
    auto primary_path = std::string_view{cjk_font_path_};
    int fallback_face =
        simplified_chinese ? japanese_face_ : simplified_chinese_face_;
    auto fallback_path = std::string_view{cjk_font_path_};
#ifdef WIN32
    primary_face = kWindowsJapaneseFace;
    primary_path = windows_japanese_font_path_;
    fallback_face = kWindowsSimplifiedChineseFace;
    fallback_path = windows_simplified_chinese_font_path_;
#endif

    for (unsigned int index = 0;
         index < static_cast<unsigned int>(FontId::Count); ++index) {
      const auto id = static_cast<FontId>(index);
      const auto &spec = kFontSpecs[id];
      fonts_[id] = OpenFont(primary_path, primary_face,
                            static_cast<float>(spec.pixel_size));
      fallback_fonts_[id] = OpenFont(fallback_path, fallback_face,
                                     static_cast<float>(spec.pixel_size));
      if (fonts_[id] == nullptr || fallback_fonts_[id] == nullptr) {
        logging::SdlError(logging::Channel::Graphics,
                          "Failed to load a text font");
        CleanupFonts();
        return false;
      }
      ConfigureFont(fonts_[id], spec);
      ConfigureFont(fallback_fonts_[id], spec);
      if (!TTF_AddFallbackFont(fonts_[id], fallback_fonts_[id])) {
        logging::SdlError(logging::Channel::Graphics,
                          "Failed to configure the fallback text font");
        CleanupFonts();
        return false;
      }
    }
    return true;
  }

public:
  [[nodiscard]] bool Initialize(std::string_view language) {
    if (initialized_) {
      return SetLanguage(language);
    }
    if (!TTF_Init()) {
      logging::SdlError(logging::Channel::Graphics,
                        "Failed to initialize SDL_ttf");
      return false;
    }
    initialized_ = true;
    language_ = NormalizeLanguage(language);

    engine_ = TTF_CreateSurfaceTextEngine();
    if (engine_ == nullptr) {
      logging::SdlError(logging::Channel::Graphics,
                        "Failed to create the SDL_ttf surface text engine");
      Cleanup();
      return false;
    }

#ifdef WIN32
    auto *environment = SDL_GetEnvironment();
    const char *windows_directory =
        SDL_GetEnvironmentVariable(environment, "SystemRoot");
    if (windows_directory == nullptr || windows_directory[0] == '\0') {
      windows_directory = SDL_GetEnvironmentVariable(environment, "windir");
    }
    if (windows_directory != nullptr && windows_directory[0] != '\0') {
      windows_japanese_font_path_ =
          std::string(windows_directory) + kWindowsJapaneseFontRelativePath;
      windows_simplified_chinese_font_path_ =
          std::string(windows_directory) +
          kWindowsSimplifiedChineseFontRelativePath;
    }

    auto *japanese_font =
        OpenFont(windows_japanese_font_path_, kWindowsJapaneseFace, 12.0F);
    if (japanese_font == nullptr) {
      logging::Critical(logging::Channel::Graphics,
                        "Failed to open MS Gothic: {}",
                        windows_japanese_font_path_);
      Cleanup();
      return false;
    }
    TTF_CloseFont(japanese_font);

    auto *simplified_chinese_font =
        OpenFont(windows_simplified_chinese_font_path_,
                 kWindowsSimplifiedChineseFace, 12.0F);
    if (simplified_chinese_font == nullptr) {
      logging::Critical(logging::Channel::Graphics, "Failed to open SimSun: {}",
                        windows_simplified_chinese_font_path_);
      Cleanup();
      return false;
    }
    TTF_CloseFont(simplified_chinese_font);
    japanese_face_ = kWindowsJapaneseFace;
    simplified_chinese_face_ = kWindowsSimplifiedChineseFace;
#else
    std::optional<std::string_view> configured_path;
    auto *environment = SDL_GetEnvironment();
    const char *override_path =
        SDL_GetEnvironmentVariable(environment, kCjkFontEnvironmentVariable);
    if (override_path != nullptr && override_path[0] != '\0') {
      configured_path = override_path;
    } else {
      for (const auto path : kLinuxCjkFontPaths) {
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error)) {
          configured_path = path;
          break;
        }
      }
    }
    if (!configured_path) {
      logging::Critical(
          logging::Channel::Graphics,
          "Noto Sans CJK was not found; set {} to its font collection path",
          kCjkFontEnvironmentVariable);
      Cleanup();
      return false;
    }
    cjk_font_path_ = *configured_path;
    const auto japanese_face = FindFace(cjk_font_path_, kJapaneseFamily);
    const auto simplified_chinese_face =
        FindFace(cjk_font_path_, kSimplifiedChineseFamily);
    if (!japanese_face || !simplified_chinese_face) {
      logging::Critical(logging::Channel::Graphics,
                        "The CJK font is missing required JP/SC faces: {}",
                        cjk_font_path_);
      Cleanup();
      return false;
    }
    japanese_face_ = *japanese_face;
    simplified_chinese_face_ = *simplified_chinese_face;
#endif
    if (!LoadFonts()) {
      Cleanup();
      return false;
    }
    logging::Info(logging::Channel::Graphics,
                  "Using text fonts: primary={}, fallback={}",
                  ActivePrimaryFontPath(), ActiveFallbackFontPath());
    return true;
  }

  [[nodiscard]] bool SetLanguage(std::string_view language) {
    const auto normalized = NormalizeLanguage(language);
    if (normalized == language_) {
      return true;
    }
    const auto previous = language_;
    language_ = normalized;
    if (!initialized_ || LoadFonts()) {
      logging::Info(logging::Channel::Graphics,
                    "Using text fonts: primary={}, fallback={}",
                    ActivePrimaryFontPath(), ActiveFallbackFontPath());
      return true;
    }
    language_ = previous;
    (void)LoadFonts();
    return false;
  }

  void Cleanup() {
    CleanupFonts();
    scratch_.Reset();
    engine_.Reset();
    if (initialized_) {
      TTF_Quit();
      initialized_ = false;
    }
    japanese_face_ = -1;
    simplified_chinese_face_ = -1;
    cjk_font_path_.clear();
#ifdef WIN32
    windows_japanese_font_path_.clear();
    windows_simplified_chinese_font_path_.clear();
#endif
  }

  [[nodiscard]] bool Initialized() const { return initialized_; }
  [[nodiscard]] SDL_Surface *Scratch() const { return scratch_; }

  [[nodiscard]] TTF_Font *Font(FontId id) const {
    return id == FontId::Count ? nullptr : fonts_[id];
  }

  [[nodiscard]] bool PrepareScratch(PixelPoint size) {
    if (scratch_ != nullptr && scratch_->w >= size.x && scratch_->h >= size.y) {
      return true;
    }
    const int width = std::max(size.x, scratch_ != nullptr ? scratch_->w : 0);
    const int height = std::max(size.y, scratch_ != nullptr ? scratch_->h : 0);
    auto *replacement =
        SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
    if (replacement == nullptr) {
      logging::SdlError(logging::Channel::Graphics,
                        "Failed to create the text rendering surface");
      return false;
    }
    scratch_ = replacement;
    return true;
  }

  [[nodiscard]] PixelPoint Measure(FontId id, std::string_view text) const {
    PixelPoint size = {};
    if (Font(id) == nullptr) {
      return size;
    }
    const bool measured =
        ForEachFontRun(id, text, [&size](TTF_Font *font, std::string_view run) {
          PixelPoint run_size = {};
          if (!TTF_GetStringSize(font, run.data(), run.size(), &run_size.x,
                                 &run_size.y)) {
            return false;
          }
          size.x += run_size.x;
          size.y = std::max(size.y, run_size.y);
          return true;
        });
    return measured ? size : PixelPoint{};
  }

  [[nodiscard]] bool Draw(FontId id, std::string_view text, PixelPoint topleft,
                          Rgb color) const {
    if (engine_ == nullptr || scratch_ == nullptr || Font(id) == nullptr) {
      return false;
    }
    int x = topleft.x;
    return ForEachFontRun(
        id, text,
        [this, &x, topleft, color](TTF_Font *font, std::string_view run) {
          auto *rendered =
              TTF_CreateText(engine_, font, run.data(), run.size());
          if (rendered == nullptr) {
            logging::SdlError(logging::Channel::Graphics,
                              "Failed to shape UTF-8 text");
            return false;
          }
          const auto rendered_guard =
              util::MakeGuard(rendered, TTF_DestroyText);
          if (!TTF_SetTextColor(rendered, color.r, color.g, color.b,
                                SDL_ALPHA_OPAQUE) ||
              !TTF_DrawSurfaceText(rendered, x, topleft.y, scratch_)) {
            logging::SdlError(logging::Channel::Graphics,
                              "Failed to draw text to the text surface");
            return false;
          }
          int width = 0;
          int height = 0;
          if (!TTF_GetStringSize(font, run.data(), run.size(), &width,
                                 &height)) {
            logging::SdlError(logging::Channel::Graphics,
                              "Failed to measure a text run");
            return false;
          }
          x += width;
          return true;
        });
  }
};
