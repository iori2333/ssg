add_executable(pack_tool
    pack_tool.cpp
    ${PROJECT_SOURCE_DIR}/ssg/data/data_manifest.cpp
    ${PROJECT_SOURCE_DIR}/ssg/data/pbg_archive.cpp
)
target_include_directories(pack_tool PRIVATE
    ${PROJECT_SOURCE_DIR}/ssg
)
target_link_libraries(pack_tool PRIVATE
    ssg::runtime
)
set_target_properties(pack_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(stage_validator
    stage_validator.cpp
    ${PROJECT_SOURCE_DIR}/ssg/enemy/ecl/ecl_program.cpp
    ${PROJECT_SOURCE_DIR}/ssg/i18n/localization.cpp
    ${PROJECT_SOURCE_DIR}/ssg/stage/scene_program.cpp
    ${PROJECT_SOURCE_DIR}/ssg/stage/stage_map.cpp
)
target_include_directories(stage_validator PRIVATE
    ${PROJECT_SOURCE_DIR}/ssg
)
target_link_libraries(stage_validator PRIVATE
    ssg::runtime
    ssg::scripts
)
add_dependencies(stage_validator ssg-scripts-generated)
set_target_properties(stage_validator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(pack_tool PRIVATE /MP)
    target_compile_options(stage_validator PRIVATE /MP)
endif()
