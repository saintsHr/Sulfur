#include "sulfur/utils/arena.h"
#include "sulfur/utils/log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SF_ARENA_ALIGNMENT ((size_t)8)

static bool align_up_overflows(size_t n, size_t align);
static size_t align_up(size_t n, size_t align);

static sf_arena_chunk *arena_chunk_new(size_t size);
static bool arena_ensure_space(sf_arena *arena, size_t aligned);

void sf_arena_init(sf_arena *arena, size_t default_chunk_size) {
    arena->first = NULL;
    arena->current = NULL;
    arena->default_chunk_size = default_chunk_size;
}

void *sf_arena_alloc(sf_arena *arena, size_t size) {
    if (align_up_overflows(size, SF_ARENA_ALIGNMENT)) {
        return NULL;
    }

    size_t aligned = align_up(size, SF_ARENA_ALIGNMENT);

    if (!arena_ensure_space(arena, aligned)) {
        return NULL;
    }

    sf_arena_chunk *chunk = arena->current;
    void *ptr = chunk->data + chunk->used;
    chunk->used += aligned;

    return ptr;
}

void *sf_arena_grow_array(sf_arena *arena, void *old_array, size_t old_count, size_t new_count, size_t elem_size) {
    if (elem_size != 0 && new_count > SIZE_MAX / elem_size) {
        return NULL;
    }

    void *new_array = sf_arena_alloc(arena, new_count * elem_size);
    if (new_array == NULL) {
        return NULL;
    }

    if (old_array != NULL && old_count > 0) {
        size_t copy_count = old_count < new_count ? old_count : new_count;
        memcpy(new_array, old_array, copy_count * elem_size);
    }

    return new_array;
}

void sf_arena_free(sf_arena *arena) {
    sf_arena_chunk *chunk = arena->first;

    while (chunk) {
        sf_arena_chunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }

    arena->first = NULL;
    arena->current = NULL;
}

void *sf_arena_grow_one(
    sf_arena *arena, void *items, size_t count,
    size_t *capacity, size_t elem_size
) {
    if (count < *capacity) {
        return items;
    }

    size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;

    void *grown = sf_arena_grow_array(arena, items, count, new_capacity, elem_size);

    if (grown == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", "N/A",
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return NULL;
    }

    *capacity = new_capacity;

    return grown;
}

static bool align_up_overflows(size_t n, size_t align) {
    return n > SIZE_MAX - (align - 1);
}

static size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

static sf_arena_chunk *arena_chunk_new(size_t size) {
    if (size > SIZE_MAX - sizeof(sf_arena_chunk)) {
        return NULL;
    }

    sf_arena_chunk *chunk = malloc(sizeof(sf_arena_chunk) + size);
    if (!chunk) {
        return NULL;
    }

    chunk->next = NULL;
    chunk->size = size;
    chunk->used = 0;

    return chunk;
}

static bool arena_ensure_space(sf_arena *arena, size_t aligned) {
    sf_arena_chunk *current = arena->current;
    if (current != NULL && current->size - current->used >= aligned) {
        return true;
    }

    size_t chunk_size = arena->default_chunk_size;
    if (aligned > chunk_size) {
        chunk_size = aligned;
    }

    sf_arena_chunk *chunk = arena_chunk_new(chunk_size);
    if (!chunk) {
        return false;
    }

    if (current != NULL) {
        current->next = chunk;
    } else {
        arena->first = chunk;
    }

    arena->current = chunk;

    return true;
}
