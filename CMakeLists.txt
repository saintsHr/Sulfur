cmake_minimum_required(VERSION 3.16)

project(sulfur
	VERSION 0.1.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD    17)
set(C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
find_program(CLANG_FORMAT_EXE NAMES clang-format)

if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/sulfur/*.c
        ${CMAKE_SOURCE_DIR}/include/sulfur/*.h
    )

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "formating files"
    )
else()
    message(WARNING "clang-format not found")
endif()

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include_directories(
	include
)

set(SOURCES
	src/sulfur/main.c
	
	src/sulfur/utils/log.c
	src/sulfur/utils/type_utils.c
	src/sulfur/utils/token_utils.c

	src/sulfur/pipeline/frontend/ast.c
	src/sulfur/pipeline/frontend/lexer.c
	src/sulfur/pipeline/frontend/parser.c
	src/sulfur/pipeline/frontend/preprocessor.c
	
	src/sulfur/pipeline/frontend/semantic/semantic.c
	src/sulfur/pipeline/frontend/semantic/scope.c
	src/sulfur/pipeline/frontend/semantic/symbol.c

	src/sulfur/pipeline/backend/ir.c
	src/sulfur/pipeline/backend/codegen.c
)

add_executable(sulfur ${SOURCES})
