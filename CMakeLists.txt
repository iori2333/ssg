cmake_minimum_required(VERSION 3.21)
project(GIAN07 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate platform constants header from environment variables
set(PLATFORM_CONSTANTS_DEFINES "")
foreach(VARNAME APP_ID PATH_SKELETON)
    if(DEFINED ENV{${VARNAME}})
        string(APPEND PLATFORM_CONSTANTS_DEFINES
            "#define ${VARNAME} \"$ENV{${VARNAME}}\"\n")
    else()
        string(APPEND PLATFORM_CONSTANTS_DEFINES "#undef ${VARNAME}\n")
    endif()
endforeach()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/obj/platform_constants.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/obj/platform_constants.h @ONLY)

# ============================================================
# Platform-specific settings
# ============================================================
if(WIN32)
    set(SSG_PLATFORM_WINDOWS TRUE)

    if(MSVC)
        # Make CL.exe happy
        add_compile_options(/utf-8)

        # Static CRT to match static SDL3
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

        # Windows system libraries
        set(WIN32_SYSTEM_LIBS
            advapi32 imm32 gdi32 kernel32 ole32 oleaut32
            setupapi user32 uuid version winmm shell32
        )
    endif()
elseif(UNIX)
    set(SSG_PLATFORM_LINUX TRUE)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-pthread)
    endif()

    # System libraries for Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pangocairo)
    pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(libs/SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(libs/BLAKE3/c EXCLUDE_FROM_ALL)
add_subdirectory(libs/libogg EXCLUDE_FROM_ALL)
add_subdirectory(libs/libvorbis EXCLUDE_FROM_ALL)
add_subdirectory(libs/libwebp EXCLUDE_FROM_ALL)
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE libs/dr_libs)

# ============================================================
# Main executable sources
# ============================================================

# --- pbg original game code ---
file(GLOB_RECURSE PBG_SRC CONFIGURE_DEPENDS
    "GIAN07/*.cpp"
)

# --- Modern game layer ---
file(GLOB GAME_SRC CONFIGURE_DEPENDS
    "game/*.cpp"
    "game/codecs/*.cpp"
)

# --- SDL3 platform backend (all platforms) ---
file(GLOB PLATFORM_SDL_SRC CONFIGURE_DEPENDS
    "platform/sdl/*.cpp"
)

# --- miniaudio sound backend ---
file(GLOB PLATFORM_MINIAUDIO_SRC CONFIGURE_DEPENDS
    "platform/miniaudio/*.cpp"
)

# ============================================================
# Create the executable
# ============================================================
add_executable(GIAN07
    ${PBG_SRC}
    ${GAME_SRC}
    ${PLATFORM_SDL_SRC}
    ${PLATFORM_MINIAUDIO_SRC}
)

if(WIN32)
    target_sources(GIAN07 PRIVATE
        platform/windows/file.cpp
        platform/windows/midi_backend_winmm.cpp
        platform/windows/surface_gdi.cpp
        platform/windows/text_gdi.cpp
        platform/windows/time.cpp
        platform/miniaudio/miniaudio.c
    )

    # Windows resource file
    target_sources(GIAN07 PRIVATE GIAN07/GIAN07.rc)

    target_compile_definitions(GIAN07 PRIVATE WIN32)
    target_link_libraries(GIAN07 PRIVATE ${WIN32_SYSTEM_LIBS})
elseif(UNIX)
    target_sources(GIAN07 PRIVATE
        platform/c/file.cpp
        platform/c/time.cpp
        platform/pangocairo/text_pangocairo.cpp
        platform/miniaudio/miniaudio.c
    )
    target_compile_definitions(GIAN07 PRIVATE LINUX SDL3=1)
    target_link_libraries(GIAN07 PRIVATE
        PkgConfig::PANGO
        PkgConfig::FONTCONFIG
        pthread dl m
    )
endif()

# ============================================================
# Include directories
# ============================================================
target_include_directories(GIAN07 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/core
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/gameflow
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/player
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/enemy
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/bullet
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/effect
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/stage
)

# Include generated version header
target_include_directories(GIAN07 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(GIAN07 PRIVATE
    vorbis vorbisfile webp dr_libs BLAKE3::blake3 SDL3::SDL3-static
)

# ============================================================
# Output directories
# ============================================================
set_target_properties(GIAN07 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(GIAN07 PRIVATE /MP)
endif()

# ============================================================
# Pack tool (standalone utility)
# ============================================================
add_executable(pack_tool
    tools/pack_tool.cpp
    GIAN07/core/lz_uty.cpp
    platform/sdl/buffer_sdl.cpp
    platform/sdl/file_sdl.cpp
)
if(WIN32)
    target_sources(pack_tool PRIVATE platform/windows/file.cpp)
    target_compile_definitions(pack_tool PRIVATE WIN32)
    target_link_libraries(pack_tool PRIVATE ${WIN32_SYSTEM_LIBS})
elseif(UNIX)
    target_sources(pack_tool PRIVATE platform/c/file.cpp)
endif()
target_include_directories(pack_tool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07/core
)
target_link_libraries(pack_tool PRIVATE SDL3::SDL3-static)
set_target_properties(pack_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(MSVC)
    target_compile_options(pack_tool PRIVATE /MP)
endif()
