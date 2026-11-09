cmake_minimum_required(VERSION 3.16)

include(GNUInstallDirs)

set(SULFUR_STD_DIR
    "${CMAKE_INSTALL_DATADIR}/sulfur/std"
)

set(SULFUR_LIB_DIR
    "${CMAKE_INSTALL_LIBDIR}/sulfur"
)

set(STDLIB_LIB
    "${CMAKE_CURRENT_SOURCE_DIR}/build/lib/libsulfurstd.a"
)

file(GLOB_RECURSE STDLIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}*.slfr"
)

add_custom_command(
    OUTPUT  "${STDLIB_LIB}"
    COMMAND $<TARGET_FILE:sulfur> clean
    COMMAND $<TARGET_FILE:sulfur> build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS sulfur ${STDLIB_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/sulfur.toml"
    VERBATIM
)

add_custom_target(sulfurstd ALL DEPENDS "${STDLIB_LIB}")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/std/"
    DESTINATION "${SULFUR_STD_DIR}"
    FILES_MATCHING PATTERN "*.slfr"
)

install(
    FILES "${STDLIB_LIB}"
    DESTINATION "${SULFUR_LIB_DIR}"
)
