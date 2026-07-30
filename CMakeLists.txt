cmake_minimum_required(VERSION 3.16)

project(sulfur
	VERSION 0.1.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS FALSE)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

find_program(CCACHE_PROGRAM ccache)
find_program(CLANG_FORMAT_EXE NAMES clang-format)

set(SOURCES
	src/sulfur/main.c
	
	src/sulfur/utils/log.c
	src/sulfur/utils/type_utils.c
	src/sulfur/utils/token_utils.c
	src/sulfur/utils/string.c
	src/sulfur/utils/arena.c

	src/sulfur/pipeline/frontend/ast.c
	src/sulfur/pipeline/frontend/lexer.c
	src/sulfur/pipeline/frontend/parser.c
	src/sulfur/pipeline/frontend/preprocessor.c
	src/sulfur/pipeline/frontend/semantic/semantic.c
	src/sulfur/pipeline/frontend/semantic/scope.c
	src/sulfur/pipeline/frontend/semantic/symbol.c

	src/sulfur/pipeline/backend/ir.c
	src/sulfur/pipeline/backend/codegen/codegen.c
	src/sulfur/pipeline/backend/codegen/stack.c
)

add_executable(sulfur ${SOURCES})

target_include_directories(sulfur PRIVATE
    include
)

option(ENABLE_SANITIZERS "Enable Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    target_compile_options(sulfur PRIVATE
        -fsanitize=address,undefined,leak
        -fno-omit-frame-pointer
    )

    target_link_options(sulfur PRIVATE
        -fsanitize=address,undefined,leak
    )
endif()

if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/sulfur/*.c
        ${CMAKE_SOURCE_DIR}/include/sulfur/*.h
    )

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting Files."
    )
else()
    message(WARNING "clang-format Not Found.")
endif()

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()
