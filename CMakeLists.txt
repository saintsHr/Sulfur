cmake_minimum_required(VERSION 3.16)

project(sulfur
    VERSION 0.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(GNUInstallDirs)

add_subdirectory(compiler)
add_subdirectory(cli)

enable_testing()
