///
/// Internal coordination between the common graphics policy and SDL renderer.
///
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>

#include "gfx/graphics.h"

std::optional<GraphicsInitResult>
SdlGraphicsInit(std::optional<const GraphicsParams> previous,
                GraphicsParams requested);
void SdlGraphicsFlip(bool take_screenshot);

// Text atlas texture, owned by the render backend on behalf of the text
// module. The atlas is destroyed with the renderer and recreated lazily.
PixelPoint SdlTextTextureSize();
bool SdlTextTexturePrepare(PixelPoint size);
bool SdlTextTextureUpdate(const Rect *subrect,
                          std::tuple<const uint8_t *, size_t> pixels);
bool SdlTextTextureBlit(PixelPoint topleft, const Rect &src);
