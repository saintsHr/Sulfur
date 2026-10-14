cmake_minimum_required(VERSION 3.16)

project(sulfur-compiler
    VERSION 0.0.0
    LANGUAGES C
)

find_program(CCACHE_PROGRAM ccache)

set(SOURCES
    src/sulfur/driver/main.c
	src/sulfur/driver/flags.c

	src/sulfur/utils/log.c
	src/sulfur/utils/type_utils.c
	src/sulfur/utils/token_utils.c
	src/sulfur/utils/string.c
	src/sulfur/utils/arena.c
	src/sulfur/utils/file.c

	src/sulfur/pipeline/frontend/ast.c
	src/sulfur/pipeline/frontend/lexer.c
	src/sulfur/pipeline/frontend/parser/parser.c
	src/sulfur/pipeline/frontend/parser/parser_stmt.c
	src/sulfur/pipeline/frontend/parser/parser_expr.c
	src/sulfur/pipeline/frontend/preprocessor.c
	src/sulfur/pipeline/frontend/semantic/semantic.c
	src/sulfur/pipeline/frontend/semantic/semantic_expr.c
	src/sulfur/pipeline/frontend/semantic/semantic_const.c
	src/sulfur/pipeline/frontend/semantic/scope.c
	src/sulfur/pipeline/frontend/semantic/symbol.c

	src/sulfur/pipeline/backend/ir/ir.c
	src/sulfur/pipeline/backend/ir/optimization.c
	src/sulfur/pipeline/backend/codegen/codegen.c
	src/sulfur/pipeline/backend/codegen/stack.c
)

add_executable(sulfur-compiler ${SOURCES})

target_include_directories(sulfur-compiler PRIVATE
    include
)

option(ENABLE_SANITIZERS "Enable Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    target_compile_options(sulfur-compiler PRIVATE
        -fsanitize=address,undefined,leak
        -fno-omit-frame-pointer
    )

    target_link_options(sulfur-compiler PRIVATE
        -fsanitize=address,undefined,leak
    )
endif()

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

install(TARGETS sulfur-compiler
    RUNTIME DESTINATION bin
    COMPONENT compiler
)
