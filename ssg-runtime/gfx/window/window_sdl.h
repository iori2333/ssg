///
/// SDL window creation
///

#pragma once

#include <optional>
#include <string_view>
#include <utility>

#include <SDL3/SDL_video.h>

#include "gfx/graphics.h"

// Saves the windowed position before entering fullscreen mode.

SDL_Window *SdlWindow();
std::optional<GraphicsParams> SdlWindowCreate(GraphicsParams params);
void SdlWindowCleanup();
void SdlWindowRememberPosition(std::pair<int, int> position);

// Falls back to the primary display if the window doesn't exist yet.
SDL_DisplayID SdlDisplayForWindow();

// Usable display size in pixels, fullscreen or windowed.
PixelPoint SdlGraphicsDisplaySize(bool fullscreen);

std::pair<int, int> SdlWindowPosition(SDL_Window *window);

// Returns the SDL render driver index that matches [hint]. If the hint doesn't
// match any driver, the function resets SDL's render driver hints and returns
// -1.
int SdlValidateRenderDriver(std::string_view hint);

// Resolves the render driver to use for window creation (which needs to set
// driver-specific window flags before SDL_CreateRenderer() runs).
std::string_view SdlRenderDriverName(int id);

// Returns the new active fullscreen flags if the mode change was successful.
struct WindowFullscreenState {
  bool enabled;
  bool exclusive;
};

[[nodiscard]] std::optional<WindowFullscreenState>
SdlSetFullscreen(SDL_Window *window, WindowFullscreenState state);
