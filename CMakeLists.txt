cmake_minimum_required(VERSION 3.16)

project(sulfur
	VERSION 0.1.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD    17)
set(C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(
	include
)

set(SOURCES
	src/sulfur/util/log.c
	src/sulfur/ast.c
	src/sulfur/lexer.c
	src/sulfur/main.c
	src/sulfur/parser.c
	src/sulfur/preprocessor.c
	src/sulfur/semantic.c
	src/sulfur/ir.c
	src/sulfur/codegen.c
)

add_executable(sulfur ${SOURCES})
