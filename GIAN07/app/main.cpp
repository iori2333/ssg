///
/// SDL entry point — application assembly layer
///

#ifdef WIN32
// Enable visual styles for nice-looking SDL message boxes. Taken from the
// comment in `SDL_windowsmessagebox.c`, which was in turn taken from
//
// 	https://learn.microsoft.com/en-us/windows/win32/controls/cookbook-overview
#pragma comment(linker, "\"/manifestdependency:type='win32'"                   \
                        " name='Microsoft.Windows.Common-Controls'"            \
                        " version='6.0.0.0'"                                   \
                        " processorArchitecture='*'"                           \
                        " publicKeyToken='6595b64144ccf1df'"                   \
                        " language='*'\"")
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game_application.h"
#include "gfx/constants.h"
#include "sys/log.h"
#include "sys/path.h"
#include "util/crash_handler.h"
#include "util/guard.h"

int main(int argc, char **args) {
#ifndef APP_ID
#define APP_ID "GIAN07"
#endif
  logging::Initialize(PathForData(), APP_ID, VERSION_TAG);
  auto logging_guard = make_guard(logging::Shutdown);
#ifdef PBG_DEBUG
  crash::Install();
  auto crash_guard = make_guard(crash::Uninstall);
#endif

  // SDL 3 automatically pulls the Desktop Entry name from the new app
  // metadata, avoiding the need for the environment variable below.
  SDL_SetAppMetadata(GAME_TITLE.data(), VERSION_TAG.data(), APP_ID);

  if (!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO)) {
    logging::SdlError(logging::Channel::Platform, "Error initializing SDL");
    return 1;
  }
  auto sdl_guard = make_guard(SDL_Quit);

  // The X11 and Wayland backends load their dynamic symbols by trying to
  // look up each function in each of the hardcoded .so files until it's
  // found. This ends up throwing a lot of symbol loading errors at DEBUG
  // level during SDL_VideoInit() that might confuse Linux users, but we'd
  // like this log level for everything we call ourselves. So let's only
  // activate it after SDL_Init().
#ifdef PBG_DEBUG
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#else
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#endif

  // Use the backend API's line drawing algorithm, which at least gives us
  // pixel-perfect accuracy with pbg's original 16-bit code when using
  // Direct3D and framebuffer scaling. It does make sense to set this hint
  // unconditionally because the native OpenGL line drawing algorithm is also
  // slightly more accurate to the original game than the SDL algorithm when
  // drawing longer lines. But in any case, we're talking differences of less
  // than 1% of pixels here; both OpenGL and SDL algorithms are at least 97%
  // accurate. For a graphical comparison, see
  //
  // 	https::/rec98.nmlgc.net/blog/2024-10-22#lines-2024-10-22
  //
  // And if we ever want 100% accuracy for every backend API, we can always
  // reimplement Direct3D exact algorithm:
  //
  // 	https://github.com/nmlgc/ssg/issues/74
  SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "2");

  GameApplication application;
  if (!application.Initialize()) {
    return 1;
  }
  return application.Run();
}
