cmake_minimum_required(VERSION 3.16)

project(sulfur-runtime LANGUAGES NONE)

include(GNUInstallDirs)

find_program(NASM_EXECUTABLE nasm REQUIRED)
find_program(AR_EXECUTABLE ar REQUIRED)

set(RUNTIME_NAME
    sulfurrt
)
set(RUNTIME_LIB
    "${CMAKE_BINARY_DIR}/lib/lib${RUNTIME_NAME}.a"
)
set(RUNTIME_LIB_DIR
    "${CMAKE_INSTALL_LIBDIR}/sulfur"
)

file(GLOB_RECURSE RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm"
)

if(NOT RUNTIME_SOURCES)
    message(FATAL_ERROR "No runtime ASM sources found")
endif()

set(RUNTIME_OBJECTS)

foreach(SOURCE IN LISTS RUNTIME_SOURCES)
    file(RELATIVE_PATH RELATIVE_SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${SOURCE}"
    )

    string(REGEX REPLACE "\\.asm$" ".o" OBJECT_RELATIVE
        "${RELATIVE_SOURCE}"
    )

    set(OBJECT
        "${CMAKE_CURRENT_BINARY_DIR}/obj/${OBJECT_RELATIVE}"
    )

    get_filename_component(OBJECT_DIR "${OBJECT}" DIRECTORY)

    add_custom_command(
        OUTPUT "${OBJECT}"

        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${OBJECT_DIR}"
        COMMAND "${NASM_EXECUTABLE}"
            -f elf64
            "${SOURCE}"
            -o "${OBJECT}"

        DEPENDS "${SOURCE}"
        COMMENT "Assembling ${RELATIVE_SOURCE}"
        VERBATIM
    )

    list(APPEND RUNTIME_OBJECTS "${OBJECT}")
endforeach()

add_custom_command(
    OUTPUT "${RUNTIME_LIB}"

    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_BINARY_DIR}/lib"
    COMMAND "${AR_EXECUTABLE}" rcs
        "${RUNTIME_LIB}"
        ${RUNTIME_OBJECTS}

    DEPENDS ${RUNTIME_OBJECTS}
    COMMENT "Building Sulfur Runtime"
    VERBATIM
)

add_custom_target(sulfur-runtime ALL
    DEPENDS "${RUNTIME_LIB}"
)

install(
    FILES "${RUNTIME_LIB}"
    DESTINATION "${RUNTIME_LIB_DIR}"
)
