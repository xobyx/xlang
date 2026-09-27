#include "arena.h"

Arena *g_lex_arena = NULL;

static ArenaChunk* arena_new_chunk(size_t capacity)
{
    ArenaChunk *chunk = (ArenaChunk *)malloc(sizeof(ArenaChunk) + capacity);
    if (!chunk) return NULL;
    chunk->next = NULL;
    chunk->capacity = capacity;
    chunk->used = 0;
    return chunk;
}

Arena* arena_create(size_t chunk_size)
{
    Arena *a = (Arena *)malloc(sizeof(Arena));
    if (!a) return NULL;
    a->chunk_size = (chunk_size > 0) ? chunk_size : ARENA_CHUNK_SIZE;
    a->head = arena_new_chunk(a->chunk_size);
    return a;
}

void* arena_alloc(Arena *a, size_t size)
{
    if (!a || size == 0) return NULL;

    /* 8-byte alignment */
    size_t aligned = (size + 7) & ~((size_t)7);

    /* Oversized allocation gets an isolated chunk */
    if (aligned > a->chunk_size)
    {
        ArenaChunk *big_chunk = arena_new_chunk(aligned);
        if (!big_chunk) return NULL;
        big_chunk->next = a->head->next;
        a->head->next = big_chunk;
        big_chunk->used = aligned;
        return big_chunk->data;
    }

    /* Move to new chunk if current does not fit */
    if (a->head->used + aligned > a->head->capacity)
    {
        ArenaChunk *new_chunk = arena_new_chunk(a->chunk_size);
        if (!new_chunk) return NULL;
        new_chunk->next = a->head;
        a->head = new_chunk;
    }

    void *ptr = &a->head->data[a->head->used];
    a->head->used += aligned;
    return ptr;
}

void* arena_calloc(Arena *a, size_t count, size_t size)
{
    size_t total = count * size;
    void *ptr = arena_alloc(a, total);
    if (ptr)
    {
        memset(ptr, 0, total);
    }
    return ptr;
}

char* arena_strdup(Arena *a, const char *str)
{
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = (char *)arena_alloc(a, len);
    if (copy)
    {
        memcpy(copy, str, len);
    }
    return copy;
}

void arena_reset(Arena *a)
{
    if (!a) return;
    ArenaChunk *curr = a->head;
    while (curr)
    {
        curr->used = 0;
        curr = curr->next;
    }
}

void arena_destroy(Arena *a)
{
    if (!a) return;
    ArenaChunk *curr = a->head;
    while (curr)
    {
        ArenaChunk *next = curr->next;
        free(curr);
        curr = next;
    }
    free(a);
}
