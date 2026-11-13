
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct sf_arena_chunk {
    struct sf_arena_chunk *next;
    size_t size;
    size_t used;

    max_align_t _align;
    uint8_t data[];
} sf_arena_chunk;

typedef struct sf_arena {
    sf_arena_chunk *first;
    sf_arena_chunk *current;
    size_t current_chunk_size;
} sf_arena;

void sf_arena_init(sf_arena *arena);

void *sf_arena_alloc(sf_arena *arena, size_t size);

void *sf_arena_grow_array(
    sf_arena *arena, void *old_array, size_t old_count, size_t new_count,
    size_t elem_size
);

void *sf_arena_grow_one(
    sf_arena *arena, void *items, size_t count, size_t *capacity,
    size_t elem_size
);

void sf_arena_free(sf_arena *arena);
