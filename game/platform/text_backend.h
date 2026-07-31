///
/// Platform-specific text rendering backend
///

#pragma once

#include "gfx/graphics.h"

#ifdef WIN32
// GDI text rendering bridge to the graphics backend
class SURFACE_GDI;
SURFACE_GDI &GrpSurface_GDIText_Surface(void) noexcept;
bool GrpSurface_GDIText_Create(int32_t w, int32_t h, RGB colorkey);
bool GrpSurface_GDIText_Update(const PIXEL_LTWH &r) noexcept;

#include "platform/windows/text_gdi.h"
#elif defined(LINUX)
#include "platform/linux/pangocairo/text_pangocairo.h"
#endif

extern TEXTRENDER TextObj;

// Shuts down the backend, deleting all fonts.
void TextBackend_Cleanup(void);
