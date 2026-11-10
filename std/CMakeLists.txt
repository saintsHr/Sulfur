cmake_minimum_required(VERSION 3.16)

project(sulfur-std LANGUAGES NONE)

include(GNUInstallDirs)

set(STD_NAME
    sulfurstd
)
set(STD_LIB
    "${CMAKE_BINARY_DIR}/lib/lib${STD_NAME}.a"
)
set(STD_DIR
    "${CMAKE_INSTALL_DATADIR}/sulfur/std"
)
set(STD_LIB_DIR
    "${CMAKE_INSTALL_LIBDIR}/sulfur"
)
set(STD_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/src/std"
)
set(STD_CONFIG
    "${CMAKE_CURRENT_SOURCE_DIR}/sulfur.toml"
)

file(GLOB_RECURSE STD_SOURCES CONFIGURE_DEPENDS
    "${STD_SOURCE_DIR}/*.slfr"
)

if(NOT STD_SOURCES)
    message(FATAL_ERROR "No STD sources found")
endif()

if(NOT TARGET sulfur)
    message(FATAL_ERROR "The Sulfur compiler target is not available")
endif()

add_custom_command(
    OUTPUT "${STD_LIB}"

    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_BINARY_DIR}/lib"
    COMMAND $<TARGET_FILE:sulfur> build
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/build/lib/libsulfurstd.a"
        "${STD_LIB}"

    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS
        sulfur
        sulfur-compiler
        sulfur-runtime
        ${STD_SOURCES}
        "${STD_CONFIG}"
    COMMENT "Building Sulfur STD"
    VERBATIM
)

add_custom_target(sulfur-std ALL
    DEPENDS "${STD_LIB}"
)

add_dependencies(sulfur-std sulfur sulfur-compiler sulfur-runtime)

install(
    DIRECTORY "${STD_SOURCE_DIR}/"
    DESTINATION "${STD_DIR}"
    FILES_MATCHING
        PATTERN "*.slfr"
)

install(
    FILES "${STD_LIB}"
    DESTINATION "${STD_LIB_DIR}"
)
