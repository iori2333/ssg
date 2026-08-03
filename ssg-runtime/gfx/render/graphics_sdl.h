///
/// Internal coordination between the common graphics policy and SDL renderer.
///
#pragma once

#include <optional>

#include "gfx/graphics.h"

struct SDL_Surface;

PixelPoint SdlGraphicsDisplaySize(bool fullscreen);
std::optional<GraphicsInitResult>
SdlGraphicsInit(std::optional<const GraphicsParams> previous,
                GraphicsParams requested);
void SdlGraphicsFlip(bool take_screenshot);

bool GraphicsScreenshotSave(SDL_Surface *surface);
