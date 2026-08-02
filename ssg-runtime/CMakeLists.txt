file(GLOB_RECURSE SSG_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_library(ssg-runtime STATIC
    ${SSG_RUNTIME_SOURCES}
)
add_library(ssg::runtime ALIAS ssg-runtime)

target_include_directories(ssg-runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(ssg-runtime PUBLIC
    GAME_VERSION="${SSG_VERSION}"
    $<$<CONFIG:Debug>:PBG_DEBUG>
)

if(WIN32)
    target_compile_definitions(ssg-runtime PUBLIC WIN32)
    target_link_libraries(ssg-runtime PRIVATE ${WIN32_SYSTEM_LIBS})
elseif(UNIX)
    target_compile_definitions(ssg-runtime PUBLIC LINUX SDL3=1)
    target_link_libraries(ssg-runtime PRIVATE
        pthread dl m
    )
endif()

target_link_libraries(ssg-runtime
    PUBLIC SDL3::SDL3-static
    PRIVATE
        SDL3_ttf::SDL3_ttf-static
        miniaudio
        webp
        libfluidsynth
)

if(MSVC)
    target_compile_options(ssg-runtime PRIVATE /MP)
endif()
