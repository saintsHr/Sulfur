cmake_minimum_required(VERSION 3.16)

project(sulfur-cli
    VERSION 0.0.0
    LANGUAGES C
)

find_program(CCACHE_PROGRAM ccache)

find_path(TOMLC17_INCLUDE_DIR
    NAMES tomlc17.h
    REQUIRED
)

find_library(TOMLC17_LIBRARY
    NAMES tomlc17
    REQUIRED
)

set(SOURCES
    src/sulfur/main.c
    src/sulfur/file.c
    src/sulfur/cmd.c
    src/sulfur/toml.c
)

add_executable(sulfur ${SOURCES})

target_include_directories(sulfur PRIVATE
    include
    ${TOMLC17_INCLUDE_DIR}
)

target_link_libraries(sulfur PRIVATE
    ${TOMLC17_LIBRARY}
)

option(ENABLE_SANITIZERS "Enable Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    target_compile_options(sulfur PRIVATE
        -fsanitize=address,undefined,leak
        -fno-omit-frame-pointer
    )

    target_link_options(sulfur PRIVATE
        -fsanitize=address,undefined,leak
    )
endif()

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

install(TARGETS sulfur
    RUNTIME DESTINATION bin
    COMPONENT cli
)
