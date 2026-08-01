///
/// Platform-specific window backend interface
///

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
struct GraphicsParams;

// Initialization
// --------------
// Just here to support switchable window backends for certain graphics
// backends.

typedef struct SDL_Window SDL_Window;

// Returns the SDL handle of the current window.
SDL_Window *WindowBackendSDL();

// Creates the game window and returns the actual configuration the backend is
// running. Fails if the window already exists.
std::optional<GraphicsParams> WindowBackendCreate(GraphicsParams);

void WindowBackendCleanup();
// --------------

// Returns the current top-left position of the game window.
std::optional<std::pair<int16_t, int16_t>> WindowBackendTopleft();

// Runs the main loop each frame, calling [frame_func] for each iteration, and
// returns the exit code after the game was quit.
int WindowBackendRun(std::function<void()> input_func,
                     std::function<bool()> frame_func);
