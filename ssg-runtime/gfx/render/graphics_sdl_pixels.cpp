///
/// Software rendering and CPU pixel access via SDL_Renderer
///

#include <tuple>
#include <utility>

#include <SDL3/SDL_surface.h>

#include "graphics_sdl_internal.h"

#include "sys/log.h"

SDL_Texture *EnsureSoftwareTexture() {
  if (RenderState().software_texture != nullptr) {
    return RenderState().software_texture;
  }
  RenderState().software_texture = SDL_CreateTexture(
      RenderState().primary_renderer, RenderState().software_surface->format,
      SDL_TEXTUREACCESS_STREAMING, RenderState().software_surface->w,
      RenderState().software_surface->h);
  if (RenderState().software_texture == nullptr) {
    logging::SdlError(kLogCat, "Error creating software rendering texture");
    DestroySoftwareRenderer();
    return nullptr;
  }
  TexturePostInit(*RenderState().software_texture,
                  RenderState().primary_renderer);
  return RenderState().software_texture;
}

bool GraphicsPixelAccessStart() {
  if (RenderState().software_renderer != nullptr) {
    return true;
  }
  RenderState().software_renderer =
      SDL_CreateSoftwareRenderer(RenderState().software_surface);
  if (RenderState().software_renderer == nullptr) {
    logging::SdlError(kLogCat, "Error creating software renderer");
    return DestroySoftwareRenderer();
  }
  SwitchActiveRenderer(RenderState().software_renderer.get());
  return (EnsureSoftwareTexture() != nullptr);
}

bool GraphicsPixelAccessEnd() {
  if (RenderState().software_renderer == nullptr) {
    return true;
  }
  SwitchActiveRenderer(RenderState().primary_renderer.get());
  DestroySoftwareRenderer();
  return true;
}

std::tuple<uint8_t *, size_t> GraphicsPixelAccessLock() {
  if (RenderState().software_renderer == nullptr ||
      RenderState().software_surface == nullptr) {
    logging::Critical(kLogCat,
                      "Pixel access requires an active software renderer");
    return {nullptr, 0};
  }
  // Necessary in SDL 3!
  SDL_FlushRenderer(RenderState().software_renderer);

  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    if (!SDL_LockSurface(RenderState().software_surface)) {
      logging::SdlError(kLogCat, "Error locking CPU backbuffer");
      return {nullptr, 0};
    }
  }
  auto *pixels = static_cast<uint8_t *>(RenderState().software_surface->pixels);
  return {pixels, RenderState().software_surface->pitch};
}

void GraphicsPixelAccessUnlock() {
  if (RenderState().software_surface == nullptr) {
    return;
  }
  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    SDL_UnlockSurface(RenderState().software_surface);
  }
}