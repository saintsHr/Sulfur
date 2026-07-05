cmake_minimum_required(VERSION 3.16)

project(sulfur
	VERSION 0.1.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD    17)
set(C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include_directories(
	include
)

set(SOURCES
	src/sulfur/main.c
	src/sulfur/util/log.c
	src/sulfur/pipeline/ast.c
	src/sulfur/pipeline/lexer.c
	src/sulfur/pipeline/parser.c
	src/sulfur/pipeline/preprocessor.c
	src/sulfur/pipeline/semantic.c
	src/sulfur/pipeline/ir.c
	src/sulfur/pipeline/codegen.c
)

add_executable(sulfur ${SOURCES})
