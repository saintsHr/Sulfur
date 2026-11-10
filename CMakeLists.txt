# ------------------------------------------------------------------------------
# Project
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

project(sulfur
    LANGUAGES C
)

include(GNUInstallDirs)
include(CheckCCompilerFlag)
include(CheckIPOSupported)

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
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(ENABLE_SANITIZERS "Sanitizers" OFF)

# ------------------------------------------------------------------------------
# Helpers
# ------------------------------------------------------------------------------

function(cc_flag config flag)
    string(MAKE_C_IDENTIFIER "HAS_CC_${flag}" var)
    check_c_compiler_flag("${flag}" ${var})
    if(${var})
        if(config STREQUAL "ALL")
            add_compile_options("${flag}")
        else()
            add_compile_options("$<$<CONFIG:${config}>:${flag}>")
        endif()
    endif()
endfunction()

function(ld_flag config flag)
    string(MAKE_C_IDENTIFIER "HAS_LD_${flag}" var)
    set(CMAKE_REQUIRED_LINK_OPTIONS "${flag}")
    check_c_compiler_flag("" ${var})
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(${var})
        if(config STREQUAL "ALL")
            add_link_options("${flag}")
        else()
            add_link_options("$<$<CONFIG:${config}>:${flag}>")
        endif()
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Common Flags
# ------------------------------------------------------------------------------

foreach(f
    -Wall -Wextra -Wpedantic -Wshadow
    -Wconversion -Wsign-conversion -Wfloat-conversion
    -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
    -Wmissing-declarations -Wredundant-decls -Wnested-externs
    -Wformat=2 -Wformat-security -Wundef -Wcast-align
    -Wcast-qual -Wpointer-arith -Wwrite-strings
    -Wbad-function-cast -Wswitch-enum -Wswitch-default
    -Wimplicit-fallthrough -Wunused -Wunused-macros
    -Wuninitialized -Winit-self -Wnull-dereference
    -Wdouble-promotion -Wvla -Walloca -Wstrict-aliasing=2
    -fno-common -fdiagnostics-color=always
)
    cc_flag(ALL ${f})
endforeach()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    foreach(f
        -Wlogical-op -Wduplicated-cond -Wduplicated-branches
        -Wjump-misses-init -Wtrampolines
    )
        cc_flag(ALL ${f})
    endforeach()
else()
    foreach(f
        -Wcomma -Wconditional-uninitialized -Wassign-enum
        -Wimplicit-int-conversion -Wshorten-64-to-32
    )
        cc_flag(ALL ${f})
    endforeach()
endif()

# ------------------------------------------------------------------------------
# Debug Flags
# ------------------------------------------------------------------------------

foreach(f
    -O0 -g3 -DDEBUG
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -fstack-protector-all
)
    cc_flag(Debug ${f})
endforeach()

# ------------------------------------------------------------------------------
# Release Flags
# ------------------------------------------------------------------------------

foreach(f
    -O3 -DNDEBUG -fno-plt -ffunction-sections
    -fdata-sections -fstack-protector-strong
    -fstack-clash-protection -fcf-protection -fPIE
)
    cc_flag(Release ${f})
endforeach()

foreach(f
    -Wl,--as-needed -Wl,--gc-sections -Wl,-dead_strip
    -Wl,-O1 -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
)
    ld_flag(Release ${f})
endforeach()

add_compile_options(
    "$<$<CONFIG:Release>:-U_FORTIFY_SOURCE>"
    "$<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>"
)

# ------------------------------------------------------------------------------
# Sanitizers
# ------------------------------------------------------------------------------

if(ENABLE_SANITIZERS)
    add_compile_options(
        "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
        "$<$<CONFIG:Debug>:-fsanitize-address-use-after-scope>"
        "$<$<CONFIG:Debug>:-fno-sanitize-recover=undefined>"
    )
    add_link_options(
        "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
    )
endif()

# ------------------------------------------------------------------------------
# LTO/IPO
# ------------------------------------------------------------------------------

check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_msg LANGUAGES C)
if(_ipo_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
    message(STATUS "LTO Unavailable: ${_ipo_msg}")
endif()

# ------------------------------------------------------------------------------
# Subdirectories
# ------------------------------------------------------------------------------

add_subdirectory(compiler)
add_subdirectory(cli)
add_subdirectory(runtime)
add_subdirectory(std)
