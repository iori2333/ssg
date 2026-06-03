/*
 *   Internal logging helpers for the SDL platform
 *
 */

#pragma once

// (only uses built-in types char8_t and SDL types from the included SDL_log.h)

#include <SDL3/SDL_log.h>

void Log_Init(const char8_t *title);

// Logs [msg] together with the last SDL_GetError() as a critical error.
void Log_Fail(SDL_LogCategory category, const char *msg);
