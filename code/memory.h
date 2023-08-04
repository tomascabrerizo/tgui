#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "common.h"

#define KB(value) ((value)*1024ll)
#define MB(value) (KB(value)*1024ll)
#define GB(value) (MB(value)*1024ll)

/* ----------------------
      Arena Interface
   ---------------------- */

typedef enum ArenaType {
    ARENA_TYPE_STATIC,
    ARENA_TYPE_VIRTUAL
} ArenaType;

typedef struct Arena {
    void *(*alloc)(struct Arena *arena, u64 size, u32 align);
    void (*free)(struct Arena *arena);

    u8 *buffer;
    u64 used;
    u64 size; 

    u32 stack_alloc_size;
    ArenaType type;
} Arena;

void arena_initialize(Arena *arena, u64 size, ArenaType type);

void arena_terminate(Arena *arena);

#define arena_alloc(arena, size, align) \
    (arena)->alloc((arena), (size), (align))

#define arena_free(arena) \
    (arena)->free(arena)

#define arena_push_struct(arena, type, align) \
    (type *)(arena)->alloc((arena), sizeof(type), (align))

#define arena_push_array(arena, type, count, align) \
    (type *)(arena)->alloc((arena), (sizeof(type)*(count)), (align))

void arena_stack_begin(Arena *arena, u32 type_size, u32 align);

void arena_stack_end(Arena *arena);

void *arena_stack_push(Arena *arena);

/* -------------------
      StaticArena 
   ------------------- */

void static_arena_initialize(Arena *arena, u64 size);

void static_arena_terminate(Arena *arena);

void *static_arena_alloc(Arena *arena, u64 size, u32 align);

void static_arena_free(Arena *arena);


/* -------------------
       VirtualArena
   ------------------- */

#define DEFAULT_VIRTUAL_SPACE_RANGE GB(8)

void virtual_arena_initialize(Arena *arena);

void virtual_arena_terminate(Arena *arena);

void *virtual_arena_alloc(Arena *arena, u64 size, u32 align);

void virtual_arena_free(Arena *arena);

/* ------------------------
      Circular Link list 
   ------------------------ */

/* NOTE: This is a Circular double link list and need to have a dummy object setup, pease dont forget to call dll_init */
#define clink_list_init(node) ((node)->prev = (node), (node)->next = (node))

#define clink_list_insert_front(node0, node1) \
    (node1)->prev = (node0); \
    (node1)->next = (node0)->next; \
    (node1)->prev->next = (node1); \
    (node1)->next->prev = (node1); \

#define clink_list_insert_back(node0, node1) \
    (node1)->prev = (node0)->prev; \
    (node1)->next = (node0); \
    (node1)->prev->next = (node1); \
    (node1)->next->prev = (node1); \

#define clink_list_remove(node) \
    (node)->prev->next = (node)->next; \
    (node)->next->prev = (node)->prev;

#define clink_list_is_empty(dummy) \
    ((dummy)->next == (dummy) && (dummy)->prev == (dummy))

#define clink_list_end(node, dummy) ((node) == (dummy))
#define clink_list_first(node, dummy) ((node)->prev == (dummy))
#define clink_list_last(node, dummy) ((node)->next == (dummy))

#define clink_list_get_back(dummy) (dummy)->prev

/* -------------------
       VirtualMap
   ------------------- */

typedef struct VirtualMapBucket {
    u64 key;
    void *data;
} VirtualMapBucket; 

typedef struct VirtualMap {
    u64 size;
    u64 used;
    u64 size_in_bytes;

    b32 memory_buffer_index;
    VirtualMapBucket *memory_buffer[2];

} VirtualMap;

#define VIRTUAL_MAP_BUCKET_FREE ((u64)(0ll - 1ll))
#define VIRTUAL_MAP_BUCKET_TUMBSTONE ((u64)(0ll - 2ll))

#define DEFAULT_VIRTUAL_MAP_SIZE 256 

void virtual_map_initialize(VirtualMap *map);

void virtual_map_terminate(VirtualMap *map);

void virtual_map_insert(VirtualMap *map, u64 key, void *data);

void virtual_map_remove(VirtualMap *map, u64 key);

void *virtual_map_find(VirtualMap *map, u64 key);

b32 virtual_map_contains(VirtualMap *map, u64 key);

#endif /* _MEMORY_H_ */
