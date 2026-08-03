///
/// Screenshot capture and encoding
///

#pragma once

#include <string_view>

struct SDL_Surface;

namespace image {

// Enables/disables the whole screenshot feature; an effort of 0 limits
// captures to BMP.
void ScreenshotSetEffort(int effort);

void ScreenshotSetPrefix(std::string_view prefix);
void ScreenshotRequest(bool requested);

// True while a screenshot is pending and a prefix is configured.
[[nodiscard]] bool ScreenshotActive();

// Encodes [surface] into a numbered .BMP or .webp file under the configured
// prefix.
bool ScreenshotSave(SDL_Surface *surface);

} // namespace image
