#ifndef XLANG_ARENA_H
#define XLANG_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_CHUNK_SIZE (64 * 1024)

typedef struct ArenaChunk {
    struct ArenaChunk *next;
    size_t capacity;
    size_t used;
    uint8_t data[];
} ArenaChunk;

typedef struct Arena {
    ArenaChunk *head;
    size_t chunk_size;
} Arena;

Arena* arena_create(size_t chunk_size);
void*  arena_alloc(Arena *a, size_t size);
void*  arena_calloc(Arena *a, size_t count, size_t size);
char*  arena_strdup(Arena *a, const char *str);
void   arena_reset(Arena *a);
void   arena_destroy(Arena *a);

/* Global compilation arena instance */
extern Arena *g_lex_arena;

#endif /* XLANG_ARENA_H */
