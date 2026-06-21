///
/// File I/O
///

#pragma once

#include <filesystem>
#include <optional>

#include <SDL3/SDL_iostream.h>

#include "buffer.h"

struct SDL_IOStream;

using FILE_TIMESTAMPS = std::filesystem::file_time_type;

std::optional<FILE_TIMESTAMPS> File_TimestampsGet(const char *fn);

bool File_CloseWithTimestamps(SDL_IOStream *&&context, const char *path,
                              std::optional<FILE_TIMESTAMPS> maybe_time);

// SDL wrappers
// ------------

BYTE_BUFFER_OWNED SDL_LoadFile(const char *file);
BYTE_BUFFER_OWNED SDL_LoadFile_IO(SDL_IOStream *src, bool closeio);
bool SDL_MustReadIO(SDL_IOStream *context, void *ptr, size_t size);
bool SDL_MustWriteIO(SDL_IOStream *context, const void *ptr, size_t size);
// ------------
