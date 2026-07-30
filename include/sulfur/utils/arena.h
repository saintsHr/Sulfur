#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct sf_arena_chunk {
    struct sf_arena_chunk* next;
    uint64_t size;
    uint64_t used;
    uint8_t data[];
} sf_arena_chunk;

typedef struct sf_arena {
    sf_arena_chunk* first;
    sf_arena_chunk* current;
    uint64_t default_chunk_size;
} sf_arena;

void sf_arena_init(sf_arena* arena, size_t default_chunk_size);
void* sf_arena_alloc(sf_arena* arena, size_t size);
void sf_free_arena(sf_arena* arena);