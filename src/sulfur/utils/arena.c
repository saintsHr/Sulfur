#include "sulfur/utils/arena.h"

#include <stdlib.h>

static size_t align_up(size_t n, size_t align);
static sf_arena_chunk* arena_chunk_new(size_t size);

void sf_arena_init(sf_arena* arena, size_t default_chunk_size) {
    arena->first = NULL;
    arena->current = NULL;
    arena->default_chunk_size = default_chunk_size;
}

void* sf_arena_alloc(sf_arena* arena, size_t size) {
    size_t aligned = align_up(size, 8);

    if (arena->current == NULL) {
        size_t chunk_size = arena->default_chunk_size;
        if (aligned > chunk_size) chunk_size = aligned;

        sf_arena_chunk* chunk = arena_chunk_new(chunk_size);
        if (!chunk) return NULL;

        arena->first = chunk;
        arena->current = chunk;
    }

    if (arena->current->used + aligned > arena->current->size) {
        size_t chunk_size = arena->default_chunk_size;
        if (aligned > chunk_size) chunk_size = aligned;

        sf_arena_chunk* chunk = arena_chunk_new(chunk_size);
        if (!chunk) return NULL;

        arena->current->next = chunk;
        arena->current = chunk;
    }

    void* ptr = arena->current->data + arena->current->used;
    arena->current->used += aligned;
    return ptr;
}

void sf_free_arena(sf_arena* arena) {
    sf_arena_chunk* chunk = arena->first;

    while (chunk) {
        sf_arena_chunk* next = chunk->next;
        free(chunk);
        chunk = next;
    }

    arena->first = NULL;
    arena->current = NULL;
}

static size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

static sf_arena_chunk* arena_chunk_new(size_t size) {
    sf_arena_chunk* chunk = malloc(sizeof(sf_arena_chunk) + size);
    if (!chunk) return NULL;

    chunk->next = NULL;
    chunk->size = size;
    chunk->used = 0;

    return chunk;
}