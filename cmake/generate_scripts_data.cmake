# cmake/generate_scripts_data.cmake — generate scripts_data.h and scripts_data.cpp
# Usage: cmake -DENTRIES_FILE=<path> -DOUTPUT_CPP=<path> -DOUTPUT_H=<path> -P cmake/generate_scripts_data.cmake

if(NOT ENTRIES_FILE)
    message(FATAL_ERROR "ENTRIES_FILE not specified")
endif()
if(NOT OUTPUT_CPP)
    message(FATAL_ERROR "OUTPUT_CPP not specified")
endif()
if(NOT OUTPUT_H)
    message(FATAL_ERROR "OUTPUT_H not specified")
endif()

include("${ENTRIES_FILE}")

# --- Generate header ---
file(WRITE "${OUTPUT_H}"
    "// Auto-generated — embedded ECL/SCL script lookup\n"
    "#pragma once\n"
    "#include <cstddef>\n"
    "#include <cstdint>\n"
    "struct EmbeddedScript {\n"
    "    int index;\n"
    "    const uint8_t* data;\n"
    "    size_t size;\n"
    "};\n"
    "extern const EmbeddedScript embedded_scripts[];\n"
    "extern const size_t embedded_script_count;\n"
)

# --- Generate cpp ---
set(cpp_body "// Auto-generated — embedded ECL/SCL script data\n")
string(APPEND cpp_body "#include \"scripts_data.h\"\n")

# Include all individual script headers
foreach(entry IN LISTS EMBEDDED_SCRIPTS)
    string(REGEX MATCH "^([^|]+)\\|([0-9]+)\\|([a-z-]+)$" _dummy "${entry}")
    set(script_src "${CMAKE_MATCH_1}")
    set(script_cmd "${CMAKE_MATCH_3}")
    get_filename_component(stem "${script_src}" NAME_WE)
    if(script_cmd STREQUAL "asm-ecl")
        set(header_name "${stem}_ecl.h")
    else()
        set(header_name "${stem}_scl.h")
    endif()
    string(APPEND cpp_body "#include \"${header_name}\"\n")
endforeach()

# Generate lookup table
string(APPEND cpp_body "\nconst EmbeddedScript embedded_scripts[] = {\n")
foreach(entry IN LISTS EMBEDDED_SCRIPTS)
    string(REGEX MATCH "^([^|]+)\\|([0-9]+)\\|([a-z-]+)$" _dummy "${entry}")
    set(script_src "${CMAKE_MATCH_1}")
    set(script_idx "${CMAKE_MATCH_2}")
    set(script_cmd "${CMAKE_MATCH_3}")
    get_filename_component(stem "${script_src}" NAME_WE)
    if(script_cmd STREQUAL "asm-ecl")
        set(var_name "${stem}_ecl")
    else()
        set(var_name "${stem}_scl")
    endif()
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" var_name "${var_name}")
    string(APPEND cpp_body "    { ${script_idx}, ${var_name}, ${var_name}_len },\n")
endforeach()

list(LENGTH EMBEDDED_SCRIPTS script_count)
string(APPEND cpp_body "};\nconst size_t embedded_script_count = ${script_count};\n")

file(WRITE "${OUTPUT_CPP}" "${cpp_body}")
