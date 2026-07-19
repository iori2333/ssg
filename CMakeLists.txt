cmake_minimum_required(VERSION 3.21)
project(GIAN07 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


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
            setupapi user32 uuid version shell32
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

add_subdirectory(libs/SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(libs/libogg EXCLUDE_FROM_ALL)
add_subdirectory(libs/libvorbis EXCLUDE_FROM_ALL)
add_subdirectory(libs/libwebp EXCLUDE_FROM_ALL)
add_subdirectory(libs/miniaudio EXCLUDE_FROM_ALL)
add_subdirectory(libs/tomlplusplus EXCLUDE_FROM_ALL)
add_subdirectory(libs/fluidsynth EXCLUDE_FROM_ALL)
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
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS
    "game/*.cpp"
)
list(FILTER GAME_SRC EXCLUDE REGEX "game/platform/")

# --- Windows-specific backends ---
file(GLOB PLATFORM_WINDOWS_SRC CONFIGURE_DEPENDS
    "game/platform/windows/*.cpp"
)

# --- Linux-specific backends ---
file(GLOB PLATFORM_LINUX_SRC CONFIGURE_DEPENDS
    "game/platform/linux/*/*.cpp"
)

# ============================================================
# Create the executable
# ============================================================
add_executable(GIAN07
    ${PBG_SRC}
    ${GAME_SRC}
)

target_compile_definitions(GIAN07 PRIVATE
    $<$<CONFIG:Debug>:PBG_DEBUG>
)

if(WIN32)
    target_sources(GIAN07 PRIVATE ${PLATFORM_WINDOWS_SRC})

    target_sources(GIAN07 PRIVATE GIAN07/app/GIAN07.rc)

    target_compile_definitions(GIAN07 PRIVATE WIN32)
    target_link_libraries(GIAN07 PRIVATE ${WIN32_SYSTEM_LIBS})
elseif(UNIX)
    target_sources(GIAN07 PRIVATE ${PLATFORM_LINUX_SRC})
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
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/GIAN07
)

# Include generated version header
target_include_directories(GIAN07 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(GIAN07 PRIVATE
    vorbis vorbisfile miniaudio dr_libs
    SDL3::SDL3-static webp
    tomlplusplus::tomlplusplus libfluidsynth
)

# ============================================================
# Output directories
# ============================================================
set_target_properties(GIAN07 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET GIAN07 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:GIAN07>"
        "${CMAKE_SOURCE_DIR}/bin/GIAN07"
    COMMENT "Copying GIAN07 to bin/"
)

if(MSVC)
    target_compile_options(GIAN07 PRIVATE /MP)
endif()

# ============================================================
# Script tool (ECL/SCL disassembler/assembler)
# ============================================================
add_executable(script_tool
    tools/script_tool.cpp
)
target_include_directories(script_tool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/game
)
set_target_properties(script_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(MSVC)
    target_compile_options(script_tool PRIVATE /MP)
endif()

# ============================================================
# Script code generation (ECL/SCL source -> binary -> C arrays)
# ============================================================

set(SCRIPT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(SCRIPT_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/scripts")
file(MAKE_DIRECTORY "${SCRIPT_GEN_DIR}")

# {source_path}|{entry_index}|{asm_subcommand}
set(EMBEDDED_SCRIPTS
    "${SCRIPT_SRC_DIR}/stage1.ecl|0|asm-ecl"
    "${SCRIPT_SRC_DIR}/stage2.ecl|1|asm-ecl"
    "${SCRIPT_SRC_DIR}/stage3.ecl|2|asm-ecl"
    "${SCRIPT_SRC_DIR}/stage4.ecl|3|asm-ecl"
    "${SCRIPT_SRC_DIR}/stage5.ecl|4|asm-ecl"
    "${SCRIPT_SRC_DIR}/stage6.ecl|5|asm-ecl"
    "${SCRIPT_SRC_DIR}/stage1.scl|6|asm-scl"
    "${SCRIPT_SRC_DIR}/stage2.scl|7|asm-scl"
    "${SCRIPT_SRC_DIR}/stage3.scl|8|asm-scl"
    "${SCRIPT_SRC_DIR}/stage4.scl|9|asm-scl"
    "${SCRIPT_SRC_DIR}/stage5.scl|10|asm-scl"
    "${SCRIPT_SRC_DIR}/stage6.scl|11|asm-scl"
    "${SCRIPT_SRC_DIR}/extra.ecl|24|asm-ecl"
    "${SCRIPT_SRC_DIR}/extra.scl|25|asm-scl"
    "${SCRIPT_SRC_DIR}/ending.scl|47|asm-scl"
)

set(GENERATED_HEADERS "")

foreach(entry IN LISTS EMBEDDED_SCRIPTS)
    string(REGEX MATCH "^([^|]+)\\|([0-9]+)\\|([a-z-]+)$" _dummy "${entry}")
    set(script_src "${CMAKE_MATCH_1}")
    set(script_idx "${CMAKE_MATCH_2}")
    set(script_cmd "${CMAKE_MATCH_3}")
    get_filename_component(script_stem "${script_src}" NAME_WE)

    if(script_cmd STREQUAL "asm-ecl")
        set(type_suffix "ecl")
    else()
        set(type_suffix "scl")
    endif()
    set(script_bin "${SCRIPT_GEN_DIR}/${script_stem}_${type_suffix}.bin")
    set(script_header "${SCRIPT_GEN_DIR}/${script_stem}_${type_suffix}.h")
    set(script_var "${script_stem}_${type_suffix}")

    # Assemble text script -> binary
    add_custom_command(
        OUTPUT "${script_bin}"
        COMMAND script_tool ${script_cmd} "${script_src}" "${script_bin}"
        DEPENDS script_tool "${script_src}"
        COMMENT "Assembling ${script_stem} (${script_cmd})"
    )

    # Binary -> C header
    add_custom_command(
        OUTPUT "${script_header}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT="${script_bin}"
            -DOUTPUT="${script_header}"
            -DVARNAME="${script_var}"
            -P "${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake"
        DEPENDS "${script_bin}" "${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake"
        COMMENT "Generating header for ${script_stem}"
    )

    list(APPEND GENERATED_HEADERS "${script_header}")
endforeach()

# Write entries list for the aggregation script
set(ENTRIES_FILE "${SCRIPT_GEN_DIR}/entries.cmake")
file(WRITE "${ENTRIES_FILE}" "set(EMBEDDED_SCRIPTS\n")
foreach(entry IN LISTS EMBEDDED_SCRIPTS)
    file(APPEND "${ENTRIES_FILE}" "    \"${entry}\"\n")
endforeach()
file(APPEND "${ENTRIES_FILE}" ")\n")

# Generate scripts_data.h + scripts_data.cpp (aggregates all script headers)
set(SCRIPTS_DATA_CPP "${SCRIPT_GEN_DIR}/scripts_data.cpp")
set(SCRIPTS_DATA_H   "${SCRIPT_GEN_DIR}/scripts_data.h")

add_custom_command(
    OUTPUT "${SCRIPTS_DATA_CPP}" "${SCRIPTS_DATA_H}"
    COMMAND ${CMAKE_COMMAND}
        -DENTRIES_FILE="${ENTRIES_FILE}"
        -DOUTPUT_CPP="${SCRIPTS_DATA_CPP}"
        -DOUTPUT_H="${SCRIPTS_DATA_H}"
        -P "${CMAKE_SOURCE_DIR}/cmake/generate_scripts_data.cmake"
    DEPENDS ${GENERATED_HEADERS}
            "${CMAKE_SOURCE_DIR}/cmake/generate_scripts_data.cmake"
            "${ENTRIES_FILE}"
    COMMENT "Generating scripts_data.cpp/h"
)

# Add generated source to GIAN07
target_sources(GIAN07 PRIVATE "${SCRIPTS_DATA_CPP}")
target_include_directories(GIAN07 PRIVATE "${SCRIPT_GEN_DIR}")
