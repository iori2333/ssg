add_executable(script_tool
    tools/script_tool.cpp
)
target_include_directories(script_tool PRIVATE
    ${PROJECT_SOURCE_DIR}/ssg
)
target_link_libraries(script_tool PRIVATE
    ssg::runtime
)
set_target_properties(script_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(MSVC)
    target_compile_options(script_tool PRIVATE /MP)
endif()

set(SCRIPT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SCRIPT_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SCRIPT_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
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

set(GENERATED_SCRIPT_HEADERS "")
foreach(entry IN LISTS EMBEDDED_SCRIPTS)
    string(REGEX MATCH "^([^|]+)\\|([0-9]+)\\|([a-z-]+)$" _dummy "${entry}")
    set(script_src "${CMAKE_MATCH_1}")
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

    add_custom_command(
        OUTPUT "${script_bin}"
        COMMAND "$<TARGET_FILE:script_tool>" ${script_cmd}
                "${script_src}" "${script_bin}"
        DEPENDS script_tool "${script_src}"
        COMMENT "Assembling ${script_stem} (${script_cmd})"
    )
    add_custom_command(
        OUTPUT "${script_header}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT="${script_bin}"
            -DOUTPUT="${script_header}"
            -DVARNAME="${script_var}"
            -P "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        DEPENDS "${script_bin}" "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        COMMENT "Generating header for ${script_stem}"
    )
    list(APPEND GENERATED_SCRIPT_HEADERS "${script_header}")
endforeach()

set(ENTRIES_FILE "${SCRIPT_GEN_DIR}/entries.cmake")
file(WRITE "${ENTRIES_FILE}" "set(EMBEDDED_SCRIPTS\n")
foreach(entry IN LISTS EMBEDDED_SCRIPTS)
    file(APPEND "${ENTRIES_FILE}" "    \"${entry}\"\n")
endforeach()
file(APPEND "${ENTRIES_FILE}" ")\n")

set(SCRIPTS_DATA_CPP "${SCRIPT_GEN_DIR}/scripts_data.cpp")
set(SCRIPTS_DATA_H "${SCRIPT_GEN_DIR}/scripts_data.h")
add_custom_command(
    OUTPUT "${SCRIPTS_DATA_CPP}" "${SCRIPTS_DATA_H}"
    COMMAND ${CMAKE_COMMAND}
        -DENTRIES_FILE="${ENTRIES_FILE}"
        -DOUTPUT_CPP="${SCRIPTS_DATA_CPP}"
        -DOUTPUT_H="${SCRIPTS_DATA_H}"
        -P "${SCRIPT_CMAKE_DIR}/generate_scripts_data.cmake"
    DEPENDS
        ${GENERATED_SCRIPT_HEADERS}
        "${SCRIPT_CMAKE_DIR}/generate_scripts_data.cmake"
        "${ENTRIES_FILE}"
    COMMENT "Generating scripts_data.cpp/h"
)

set(SUPPORTED_MESSAGE_LANGUAGES ja en zh)
set(MESSAGE_SRC_DIR "${SCRIPT_SRC_DIR}/i18n")
set(MESSAGE_GENERATED_HEADERS "")
set(UI_GENERATED_HEADERS "")
set(MUSIC_GENERATED_HEADERS "")
set(EMBEDDED_MESSAGE_CATALOGS "")
foreach(language IN LISTS SUPPORTED_MESSAGE_LANGUAGES)
    set(message_src "${MESSAGE_SRC_DIR}/${language}/messages.txt")
    set(ui_src "${MESSAGE_SRC_DIR}/${language}/ui.txt")
    set(music_src "${MESSAGE_SRC_DIR}/${language}/music.txt")
    set(message_bin "${SCRIPT_GEN_DIR}/${language}_messages.bin")
    set(ui_bin "${SCRIPT_GEN_DIR}/${language}_ui.bin")
    set(music_bin "${SCRIPT_GEN_DIR}/${language}_music.bin")
    set(message_header "${SCRIPT_GEN_DIR}/${language}_messages.h")
    set(ui_header "${SCRIPT_GEN_DIR}/${language}_ui.h")
    set(music_header "${SCRIPT_GEN_DIR}/${language}_music.h")

    add_custom_command(
        OUTPUT "${message_bin}"
        COMMAND "$<TARGET_FILE:script_tool>" asm-text
                "${message_src}" "${message_bin}"
        DEPENDS script_tool "${message_src}"
        COMMENT "Assembling ${language} message catalog"
    )
    add_custom_command(
        OUTPUT "${message_header}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT="${message_bin}"
            -DOUTPUT="${message_header}"
            -DVARNAME="${language}_messages"
            -P "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        DEPENDS "${message_bin}" "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        COMMENT "Generating ${language} message catalog header"
    )
    add_custom_command(
        OUTPUT "${ui_bin}"
        COMMAND "$<TARGET_FILE:script_tool>" asm-text "${ui_src}" "${ui_bin}"
        DEPENDS script_tool "${ui_src}"
        COMMENT "Assembling ${language} UI text catalog"
    )
    add_custom_command(
        OUTPUT "${ui_header}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT="${ui_bin}"
            -DOUTPUT="${ui_header}"
            -DVARNAME="${language}_ui"
            -P "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        DEPENDS "${ui_bin}" "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        COMMENT "Generating ${language} UI text catalog header"
    )
    add_custom_command(
        OUTPUT "${music_bin}"
        COMMAND "$<TARGET_FILE:script_tool>" asm-text
                "${music_src}" "${music_bin}"
        DEPENDS script_tool "${music_src}"
        COMMENT "Assembling ${language} Music Room text catalog"
    )
    add_custom_command(
        OUTPUT "${music_header}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT="${music_bin}"
            -DOUTPUT="${music_header}"
            -DVARNAME="${language}_music"
            -P "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        DEPENDS "${music_bin}" "${SCRIPT_CMAKE_DIR}/bin2h.cmake"
        COMMENT "Generating ${language} Music Room text catalog header"
    )

    list(APPEND MESSAGE_GENERATED_HEADERS "${message_header}")
    list(APPEND UI_GENERATED_HEADERS "${ui_header}")
    list(APPEND MUSIC_GENERATED_HEADERS "${music_header}")
    list(APPEND EMBEDDED_MESSAGE_CATALOGS "${message_src}|${language}")
endforeach()

set(MESSAGE_ENTRIES_FILE "${SCRIPT_GEN_DIR}/message_entries.cmake")
file(WRITE "${MESSAGE_ENTRIES_FILE}" "set(EMBEDDED_MESSAGE_CATALOGS\n")
foreach(entry IN LISTS EMBEDDED_MESSAGE_CATALOGS)
    file(APPEND "${MESSAGE_ENTRIES_FILE}" "    \"${entry}\"\n")
endforeach()
file(APPEND "${MESSAGE_ENTRIES_FILE}" ")\n")

set(MESSAGES_DATA_CPP "${SCRIPT_GEN_DIR}/messages_data.cpp")
set(MESSAGES_DATA_H "${SCRIPT_GEN_DIR}/messages_data.h")
add_custom_command(
    OUTPUT "${MESSAGES_DATA_CPP}" "${MESSAGES_DATA_H}"
    COMMAND ${CMAKE_COMMAND}
        -DENTRIES_FILE="${MESSAGE_ENTRIES_FILE}"
        -DOUTPUT_CPP="${MESSAGES_DATA_CPP}"
        -DOUTPUT_H="${MESSAGES_DATA_H}"
        -P "${SCRIPT_CMAKE_DIR}/generate_messages_data.cmake"
    DEPENDS
        ${MESSAGE_GENERATED_HEADERS}
        ${UI_GENERATED_HEADERS}
        ${MUSIC_GENERATED_HEADERS}
        "${SCRIPT_CMAKE_DIR}/generate_messages_data.cmake"
        "${MESSAGE_ENTRIES_FILE}"
    COMMENT "Generating embedded message catalog data"
)

add_custom_target(ssg-scripts-generated
    DEPENDS
        "${SCRIPTS_DATA_CPP}"
        "${SCRIPTS_DATA_H}"
        "${MESSAGES_DATA_CPP}"
        "${MESSAGES_DATA_H}"
)

add_library(ssg-scripts STATIC
    "${SCRIPTS_DATA_CPP}"
    "${MESSAGES_DATA_CPP}"
)
add_library(ssg::scripts ALIAS ssg-scripts)
add_dependencies(ssg-scripts ssg-scripts-generated)
target_include_directories(ssg-scripts PUBLIC
    ${SCRIPT_GEN_DIR}
)

if(MSVC)
    target_compile_options(ssg-scripts PRIVATE /MP)
endif()
