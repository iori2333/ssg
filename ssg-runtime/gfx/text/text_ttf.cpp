///
/// Cross-platform text rendering via SDL_ttf
///

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_surface.h>

#include "text_renderer.h"
#include "text_state.h"

#include "gfx/graphics_system.h"
#include "gfx/render/graphics_sdl.h"
#include "sys/log.h"
#include "util/guard.h"

namespace {

TextState &State() { return gfx::ActiveGraphics().text; }

} // namespace

TextRender &TextRenderer() { return gfx::ActiveGraphics().cache; }

uint32_t &
TextRenderSession::PixelSession::PixelAt(const PixelPoint &xy_rel) const {
  if (xy_rel.x < 0 || xy_rel.y < 0 || xy_rel.x >= size_.x ||
      xy_rel.y >= size_.y) {
    logging::Critical(logging::Channel::Graphics,
                      "Text pixel coordinate ({}, {}) is outside {}x{}",
                      xy_rel.x, xy_rel.y, size_.x, size_.y);
    std::abort();
  }
  return reinterpret_cast<uint32_t *>(pixels_ + (pitch_ * xy_rel.y))[xy_rel.x];
}

TextRenderSession::PixelSession::PixelSession(PixelPoint size) : size_(size) {
  auto *surface = State().Scratch();
  if (surface == nullptr) {
    logging::Critical(logging::Channel::Graphics,
                      "Text pixel editing requires a scratch surface");
    std::abort();
  }
  if (SDL_MUSTLOCK(surface)) {
    locked_ = SDL_LockSurface(surface);
    if (!locked_) {
      logging::Critical(logging::Channel::Graphics,
                        "Failed to lock the text scratch surface: {}",
                        SDL_GetError());
      std::abort();
    }
  }
  pixels_ = static_cast<uint8_t *>(surface->pixels);
  pitch_ = surface->pitch;
}

TextRenderSession::PixelSession::~PixelSession() {
  if (locked_) {
    SDL_UnlockSurface(State().Scratch());
  }
}

uint32_t
TextRenderSession::PixelSession::GetRaw(const PixelPoint &xy_rel) const {
  return PixelAt(xy_rel);
}

void TextRenderSession::PixelSession::SetRaw(const PixelPoint &xy_rel,
                                             uint32_t color) const {
  PixelAt(xy_rel) = color;
}

Rgb TextRenderSession::PixelSession::Get(const PixelPoint &xy_rel) const {
  const auto color = GetRaw(xy_rel);
  return {
      .r = static_cast<uint8_t>(color >> 16U),
      .g = static_cast<uint8_t>(color >> 8U),
      .b = static_cast<uint8_t>(color),
  };
}

void TextRenderSession::PixelSession::Set(const PixelPoint &xy_rel,
                                          const Rgb color) const {
  SetRaw(xy_rel, 0xFF000000U | (static_cast<uint32_t>(color.r) << 16U) |
                     (static_cast<uint32_t>(color.g) << 8U) | color.b);
}

TextRenderSession::TextRenderSession(const Rect rect)
    : texture_origin_{.x = rect.left, .y = rect.top}, size_(rect.Size()) {
  auto *surface = State().Scratch();
  if (surface == nullptr) {
    logging::Critical(logging::Channel::Graphics,
                      "Text rendering requires a scratch surface");
    std::abort();
  }
  const SDL_Rect clip = {.x = 0, .y = 0, .w = rect.Width(), .h = rect.Height()};
  (void)SDL_SetSurfaceClipRect(surface, &clip);
  (void)SDL_FillSurfaceRect(surface, &clip, 0);
}

TextRenderSession::~TextRenderSession() {
  auto *surface = State().Scratch();
  if (surface == nullptr) {
    return;
  }
  const Rect destination = Rect::FromPositionAndSize(texture_origin_, size_);
  if (!SdlTextTextureUpdate(&destination,
                            {static_cast<const uint8_t *>(surface->pixels),
                             static_cast<size_t>(surface->pitch)})) {
    logging::SdlError(logging::Channel::Graphics,
                      "Failed to upload rendered text");
  }
}

PixelPoint TextRenderSession::RectSize() const { return size_; }

void TextRenderSession::SetFont(FontId font) { font_ = font; }

void TextRenderSession::SetColor(Rgb color) { color_ = color; }

PixelPoint TextRenderSession::Extent(std::string_view text) const {
  return State().Measure(font_, text);
}

void TextRenderSession::Put(const PixelPoint &topleft_rel,
                            std::string_view text, std::optional<Rgb> color) {
  if (color) {
    SetColor(*color);
  }
  (void)State().Draw(font_, text, topleft_rel, color_);
}

std::optional<TextRenderSession> TextRender::Session(TextRenderRectId rect_id) {
  auto *entry = Find(rect_id);
  if (entry == nullptr) {
    return std::nullopt;
  }
  if (!State().Initialized()) {
    return std::nullopt;
  }
  const auto &rect = entry->rect;
  if (!State().PrepareScratch(rect.Size())) {
    return std::nullopt;
  }
  if (SdlTextTextureSize() != bounds_) {
    Wipe();
    if (!SdlTextTexturePrepare(bounds_)) {
      return std::nullopt;
    }
  }
  return std::optional<TextRenderSession>{std::in_place, rect};
}

void TextRender::WipeBeforeNextRender() { Wipe(); }

PixelPoint TextRender::TextExtent(FontId font, std::string_view text) {
  return State().Measure(font, text);
}

bool TextInitialize(std::string_view language) {
  return State().Initialize(language);
}

bool TextSetLanguage(std::string_view language) {
  if (!State().SetLanguage(language)) {
    return false;
  }
  TextRenderer().Wipe();
  return true;
}

void TextCleanup() {
  TextRenderer().Clear();
  State().Cleanup();
}
