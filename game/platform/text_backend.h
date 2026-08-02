///
/// Platform-specific text rendering backend
///

#pragma once

#include "gfx/graphics.h"

#ifdef WIN32
// GDI text rendering bridge to the graphics backend
class SurfaceGdi;
SurfaceGdi &GraphicsSurfaceGdiTextSurface() noexcept;
bool GraphicsSurfaceGdiTextCreate(int32_t w, int32_t h, Rgb colorkey);
bool GraphicsSurfaceGdiTextUpdate(const PixelLtwh &r) noexcept;

#include "platform/windows/text_gdi.h"
#elifdef LINUX
#include "platform/linux/text_pangocairo.h"
#endif

TextRender &TextRenderer();

// Shuts down the backend, deleting all fonts.
void TextBackendCleanup();
