# Convert a binary file to a C header with an unsigned char array.
# Usage: cmake -DINPUT=<binary> -DOUTPUT=<header> [-DVARNAME=<name>] -P bin2h.cmake

if(NOT INPUT)
    message(FATAL_ERROR "INPUT not specified")
endif()
if(NOT OUTPUT)
    message(FATAL_ERROR "OUTPUT not specified")
endif()

file(READ "${INPUT}" hex_data HEX)
string(LENGTH "${hex_data}" hex_len)

if(NOT VARNAME)
    get_filename_component(VARNAME "${INPUT}" NAME_WE)
endif()
string(REGEX REPLACE "[^a-zA-Z0-9]" "_" VARNAME "${VARNAME}")

math(EXPR byte_len "${hex_len} / 2")

set(byte_list "")
set(col 0)
set(i 0)
while(i LESS hex_len)
    string(SUBSTRING "${hex_data}" ${i} 2 byte)
    string(APPEND byte_list "0x${byte}")
    math(EXPR i "${i} + 2")
    math(EXPR col "${col} + 1")
    if(i LESS hex_len)
        if(col EQUAL 12)
            string(APPEND byte_list ",\n    ")
            set(col 0)
        else()
            string(APPEND byte_list ", ")
        endif()
    endif()
endwhile()

file(WRITE "${OUTPUT}"
    "// Auto-generated from ${INPUT}\n"
    "#include <cstddef>\n"
    "#include <cstdint>\n"
    "const uint8_t ${VARNAME}[] = {\n"
    "    ${byte_list}\n"
    "};\n"
    "const size_t ${VARNAME}_len = ${byte_len};\n"
)
