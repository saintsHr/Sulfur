cmake_minimum_required(VERSION 3.16)

project(sulfur-stdlib
    VERSION 0.0.0
    LANGUAGES NONE
)

include(GNUInstallDirs)

find_program(NASM_EXECUTABLE nasm REQUIRED)
find_program(AR_EXECUTABLE ar REQUIRED)

set(STD_SEARCH_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STD_ROOT ${STD_SEARCH_DIR}/std)

file(GLOB_RECURSE STD_SOURCES CONFIGURE_DEPENDS
    RELATIVE ${STD_ROOT}
    ${STD_ROOT}/*.slfr
)

set(STD_SOURCES_ABS ${STD_SOURCES})
list(TRANSFORM STD_SOURCES_ABS PREPEND ${STD_ROOT}/)

set(STD_OBJECTS "")

foreach(src IN LISTS STD_SOURCES)
    string(REGEX REPLACE "\\.slfr$" "" mod "${src}")
    string(REPLACE "/" "." flat "std.${mod}")

    set(asm ${CMAKE_CURRENT_BINARY_DIR}/asm/${flat}.asm)
    set(obj ${CMAKE_CURRENT_BINARY_DIR}/obj/${flat}.o)

    add_custom_command(
        OUTPUT  ${obj}
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/asm
                ${CMAKE_CURRENT_BINARY_DIR}/obj
        COMMAND $<TARGET_FILE:sulfur-compiler>
                -m ${STD_SEARCH_DIR}
                --module
                -i ${STD_ROOT}/${src}
                -o ${asm}
        COMMAND ${NASM_EXECUTABLE} -f elf64 ${asm} -o ${obj}
        DEPENDS sulfur-compiler ${STD_SOURCES_ABS}
        COMMENT "Compiling stdlib module ${flat}"
        VERBATIM
    )

    list(APPEND STD_OBJECTS ${obj})
endforeach()

set(STD_ARCHIVE_NAME libsulfurstd.a)
set(STD_ARCHIVE ${CMAKE_CURRENT_BINARY_DIR}/${STD_ARCHIVE_NAME})

add_custom_command(
    OUTPUT  ${STD_ARCHIVE}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${STD_ARCHIVE}
    COMMAND ${AR_EXECUTABLE} rcs ${STD_ARCHIVE} ${STD_OBJECTS}
    DEPENDS ${STD_OBJECTS}
    COMMENT "Archiving libsulfur_std.a"
    VERBATIM
)

add_custom_target(sulfur-std ALL DEPENDS ${STD_ARCHIVE})

install(
    DIRECTORY std/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sulfur/std
    COMPONENT stdlib
    FILES_MATCHING PATTERN "*.slfr"
)

install(
    FILES ${STD_ARCHIVE}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/sulfur
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    COMPONENT stdlib
)
