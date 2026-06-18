///
/// Internal logging helpers for the SDL platform
///

#pragma once

#include <SDL3/SDL_log.h>

void Log_Init(const char *title);

// Logs [msg] together with the last SDL_GetError() as a critical error.
void Log_Fail(SDL_LogCategory category, const char *msg);
