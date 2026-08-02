cmake_minimum_required(VERSION 3.21)
project(SSG VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Platform-specific settings
# ============================================================
if(WIN32)
    if(MSVC)
        add_compile_options(/utf-8)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set(WIN32_SYSTEM_LIBS
            advapi32 imm32 gdi32 kernel32 ole32 oleaut32
            setupapi user32 uuid version shell32
        )
    endif()
elseif(UNIX)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-pthread)
    endif()
endif()

# ============================================================
# Third-party dependencies
# ============================================================
set(SDL_SHARED OFF CACHE BOOL "Disable the shared SDL3 library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build the static SDL3 library" FORCE)
add_subdirectory(libs/SDL3 EXCLUDE_FROM_ALL)

function(ssg_add_sdl_ttf)
    set(BUILD_SHARED_LIBS OFF)
    set(SDLTTF_HARFBUZZ ON)
    set(SDLTTF_INSTALL OFF)
    set(SDLTTF_PLUTOSVG OFF)
    set(SDLTTF_SAMPLES OFF)
    set(SDLTTF_VENDORED ON)
    add_subdirectory(libs/SDL_ttf EXCLUDE_FROM_ALL)
endfunction()
ssg_add_sdl_ttf()

add_subdirectory(libs/libwebp EXCLUDE_FROM_ALL)

set(MINIAUDIO_NO_LIBVORBIS ON CACHE BOOL
    "Disable miniaudio's libvorbis backend" FORCE
)
add_subdirectory(libs/miniaudio EXCLUDE_FROM_ALL)
target_compile_definitions(miniaudio PRIVATE STB_VORBIS_NO_INTEGER_CONVERSION)
if(MSVC)
    target_compile_options(miniaudio PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/libs/miniaudio/extras/stb_vorbis.c"
    )
else()
    target_compile_options(miniaudio PRIVATE
        "-include;${CMAKE_CURRENT_SOURCE_DIR}/libs/miniaudio/extras/stb_vorbis.c"
    )
endif()

add_subdirectory(libs/tomlplusplus EXCLUDE_FROM_ALL)

# FluidSynth defaults to osal=glib, which requires GLib2. Use its C++11 OS
# abstraction on Windows so the static build has no GLib dependency.
if(WIN32)
    set(osal cpp11 CACHE STRING "FluidSynth OS abstraction" FORCE)
endif()
add_subdirectory(libs/fluidsynth EXCLUDE_FROM_ALL)

# ============================================================
# Version metadata
# ============================================================
set(SSG_VERSION "v${PROJECT_VERSION}")
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}"
                rev-parse --short HEAD
        RESULT_VARIABLE GIT_REVISION_RESULT
        OUTPUT_VARIABLE GIT_REVISION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_REVISION_RESULT EQUAL 0 AND GIT_REVISION MATCHES "^[0-9a-fA-F]+$")
        set(SSG_VERSION "${GIT_REVISION}")
    endif()
endif()

# ============================================================
# Project targets
# ============================================================
add_subdirectory(ssg-runtime)
add_subdirectory(ssg-scripts)
add_subdirectory(ssg)
add_subdirectory(tools)
