# ------------------------------------------------------------------------------
# Project
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

project(sulfur
    LANGUAGES C
)

include(GNUInstallDirs)

# ------------------------------------------------------------------------------
# Checks
# ------------------------------------------------------------------------------

if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
    message(
        FATAL_ERROR
        "Compiler not supported: ${CMAKE_C_COMPILER_ID} (use GCC or Clang)"
    )
endif()

# ------------------------------------------------------------------------------
# Config
# ------------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

# ------------------------------------------------------------------------------
# Common Flags
# ------------------------------------------------------------------------------

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wfloat-conversion
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wold-style-definition
    -Wmissing-declarations
    -Wredundant-decls
    -Wnested-externs
    -Wformat=2
    -Wformat-security
    -Wundef
    -Wcast-align
    -Wcast-qual
    -Wpointer-arith
    -Wwrite-strings
    -Wbad-function-cast
    -Wswitch-default
    -Wimplicit-fallthrough
    -Wunused
    -Wunused-macros
    -Wuninitialized
    -Winit-self
    -Wnull-dereference
    -Wdouble-promotion
    -Wvla
    -Walloca
    -Wstrict-aliasing=2
    -Warray-bounds=2
    -Wstringop-overflow=2
    -Wstringop-truncation
    -Wformat-overflow=2
    -Wpointer-sign
    -Wint-conversion
    -Wenum-conversion
    -Waddress
    -Wlogical-not-parentheses
    -Wparentheses
    -Wsequence-point
    -Wreturn-type
    -Wimplicit
    -Werror=implicit-function-declaration
    -fno-common
    -fdiagnostics-color=always
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wlogical-op
        -Wduplicated-cond
        -Wduplicated-branches
        -Wtrampolines
        -Wjump-misses-init
        -Wuseless-cast
        -Walloc-zero
        -Walloc-size
        -Warray-parameter=2
        -Wcast-function-type
        -fstrict-flex-arrays=3
        -Wstringop-overread
        -Wmaybe-uninitialized
    )
else()
    add_compile_options(
        -Wcomma
        -Wconditional-uninitialized
        -Wassign-enum
        -Wimplicit-int-conversion
        -Wshorten-64-to-32
        -Wdocumentation
        -Wunreachable-code
        -Wunreachable-code-aggressive
        -Wthread-safety
        -Wtautological-compare
        -Wtautological-constant-out-of-range-compare
        -Wextra-semi
        -Wreserved-identifier
    )
endif()

# ------------------------------------------------------------------------------
# Debug Flags
# ------------------------------------------------------------------------------

add_compile_options(
    "$<$<CONFIG:Debug>:-O0>"
    "$<$<CONFIG:Debug>:-g3>"
    "$<$<CONFIG:Debug>:-DDEBUG>"
    "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
    "$<$<CONFIG:Debug>:-fno-optimize-sibling-calls>"
    "$<$<CONFIG:Debug>:-fstack-protector-strong>"
    "$<$<CONFIG:Debug>:-fno-inline>"
    "$<$<CONFIG:Debug>:-fno-common>"
)

# ------------------------------------------------------------------------------
# Release Flags
# ------------------------------------------------------------------------------

add_compile_options(
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:Release>:-DNDEBUG>"
    "$<$<CONFIG:Release>:-fno-plt>"
    "$<$<CONFIG:Release>:-ffunction-sections>"
    "$<$<CONFIG:Release>:-fdata-sections>"
    "$<$<CONFIG:Release>:-fstack-protector-strong>"
    "$<$<CONFIG:Release>:-fstack-clash-protection>"
    "$<$<CONFIG:Release>:-fcf-protection=full>"
    "$<$<CONFIG:Release>:-fPIE>"
    "$<$<CONFIG:Release>:-D_FORTIFY_SOURCE=3>"
    "$<$<CONFIG:Release>:-fstrict-aliasing>"
    "$<$<CONFIG:Release>:-fno-delete-null-pointer-checks>"
    "$<$<CONFIG:Release>:-fno-strict-overflow>"
)

add_link_options(
    "$<$<CONFIG:Release>:-Wl,--as-needed>"
    "$<$<CONFIG:Release>:-Wl,--gc-sections>"
    "$<$<CONFIG:Release>:-Wl,-O2>"
    "$<$<CONFIG:Release>:-pie>"
    "$<$<CONFIG:Release>:-Wl,-z,relro>"
    "$<$<CONFIG:Release>:-Wl,-z,now>"
    "$<$<CONFIG:Release>:-Wl,-z,noexecstack>"
    "$<$<CONFIG:Release>:-Wl,--sort-common>"
    "$<$<CONFIG:Release>:-Wl,--build-id=sha1>"
)

# ------------------------------------------------------------------------------
# Sanitizers
# ------------------------------------------------------------------------------

if(ENABLE_SANITIZERS)
    add_compile_options(
        "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
        "$<$<CONFIG:Debug>:-fsanitize-address-use-after-scope>"
        "$<$<CONFIG:Debug>:-fno-sanitize-recover=all>"
        "$<$<CONFIG:Debug>:-fsanitize=float-divide-by-zero>"
        "$<$<CONFIG:Debug>:-fsanitize=float-cast-overflow>"
        "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
        "$<$<CONFIG:Debug>:-fno-optimize-sibling-calls>"
    )

    add_link_options(
        "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
        "$<$<CONFIG:Debug>:-fsanitize=float-divide-by-zero>"
        "$<$<CONFIG:Debug>:-fsanitize=float-cast-overflow>"
    )
endif()

# ------------------------------------------------------------------------------
# Subdirectories
# ------------------------------------------------------------------------------

add_subdirectory(compiler)
add_subdirectory(cli)
add_subdirectory(runtime)
add_subdirectory(std)
