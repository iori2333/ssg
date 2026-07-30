# Generates the embedded localized message catalog lookup table.

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

file(WRITE "${OUTPUT_H}"
    "// Auto-generated embedded localized message catalogs\n"
    "#pragma once\n"
    "#include <cstddef>\n"
    "#include <cstdint>\n"
    "struct EmbeddedMessageCatalog {\n"
    "    const char* language;\n"
    "    const uint8_t* message_data;\n"
    "    size_t message_size;\n"
    "    const uint8_t* ui_data;\n"
    "    size_t ui_size;\n"
    "};\n"
    "extern const EmbeddedMessageCatalog embedded_message_catalogs[];\n"
    "extern const size_t embedded_message_catalog_count;\n"
)

set(cpp_body "// Auto-generated embedded localized message catalog data\n")
string(APPEND cpp_body "#include \"messages_data.h\"\n")
foreach(entry IN LISTS EMBEDDED_MESSAGE_CATALOGS)
    string(REGEX MATCH "^([^|]+)\\|([A-Za-z0-9_-]+)$" _dummy "${entry}")
    set(language "${CMAKE_MATCH_2}")
    string(REPLACE "-" "_" variable "${language}_messages")
    string(APPEND cpp_body "#include \"${variable}.h\"\n")
    string(REPLACE "-" "_" ui_variable "${language}_ui")
    string(APPEND cpp_body "#include \"${ui_variable}.h\"\n")
endforeach()

string(APPEND cpp_body "\nconst EmbeddedMessageCatalog embedded_message_catalogs[] = {\n")
foreach(entry IN LISTS EMBEDDED_MESSAGE_CATALOGS)
    string(REGEX MATCH "^([^|]+)\\|([A-Za-z0-9_-]+)$" _dummy "${entry}")
    set(language "${CMAKE_MATCH_2}")
    string(REPLACE "-" "_" variable "${language}_messages")
    string(REPLACE "-" "_" ui_variable "${language}_ui")
    string(APPEND cpp_body
        "    { \"${language}\", ${variable}, ${variable}_len, ${ui_variable}, ${ui_variable}_len },\n")
endforeach()
list(LENGTH EMBEDDED_MESSAGE_CATALOGS catalog_count)
string(APPEND cpp_body
    "};\nconst size_t embedded_message_catalog_count = ${catalog_count};\n")

file(WRITE "${OUTPUT_CPP}" "${cpp_body}")
