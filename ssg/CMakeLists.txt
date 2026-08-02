file(GLOB_RECURSE SSG_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_executable(ssg
    ${SSG_SOURCES}
)

if(WIN32)
    target_sources(ssg PRIVATE app/GIAN07.rc)
endif()

target_include_directories(ssg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ssg PRIVATE
    ssg::runtime
    ssg::scripts
    tomlplusplus::tomlplusplus
)
add_dependencies(ssg ssg-scripts-generated)

set_target_properties(ssg PROPERTIES
    OUTPUT_NAME GIAN07
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET ssg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:ssg>"
        "${PROJECT_SOURCE_DIR}/bin/"
    COMMENT "Copying GIAN07 to bin/"
)

if(MSVC)
    target_compile_options(ssg PRIVATE /MP)
endif()
