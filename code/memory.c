#include "common.h"
#include "os.h"
#include "memory.h"

/* ----------------------
      Arena Interface
   ---------------------- */

void arena_initialize(Arena *arena, u64 size, ArenaType type) {
    switch (type) {
    case ARENA_TYPE_STATIC: { 
        static_arena_initialize(arena, size); 
        arena->alloc = static_arena_alloc;
        arena->free = static_arena_free;
    } break;
    case ARENA_TYPE_VIRTUAL: {
        virtual_arena_initialize(arena); 
        arena->alloc = virtual_arena_alloc;
        arena->free = virtual_arena_free;
    } break;
    }
}

void arena_terminate(Arena *arena) {
    switch (arena->type) {
    case ARENA_TYPE_STATIC: { 
        static_arena_terminate(arena); 
    } break;
    case ARENA_TYPE_VIRTUAL: {
        virtual_arena_terminate(arena); 
    } break;
    }
}

void arena_stack_begin(Arena *arena, u32 type_size, u32 align) {
    arena->stack_alloc_size = type_size;
    arena->alloc(arena, 0, align);
    ASSERT((arena->used & (os_get_page_size() - 1)) == 0);
}

void arena_stack_end(Arena *arena) {
    arena->stack_alloc_size = 0;
}

void *arena_stack_push(Arena *arena) {
    ASSERT(arena->stack_alloc_size > 0);
    void *result = arena->alloc(arena, arena->stack_alloc_size, 1);
    return result;
}

/* -------------------
      StaticArena 
   ------------------- */

void static_arena_initialize(Arena *arena, u64 size) {
    u64 page_size = os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));
    ASSERT(page_size > 0);
    
    u64 page_number = (size + (page_size - 1)) / page_size;
    u64 align_size = page_size * page_number;

    arena->size = align_size;
    arena->used = 0;
    arena->buffer = os_virtual_reserve(align_size);
    os_virtual_commit(arena->buffer, align_size);

    arena->stack_alloc_size = 0;
}

void static_arena_terminate(Arena *arena) {
    ASSERT(arena->buffer);
    os_virtual_decommit(arena->buffer, arena->size);
    os_virtual_release(arena->buffer, arena->size);
}

void *static_arena_alloc(Arena *arena, u64 size, u32 align) {
    ASSERT(arena->buffer);
    ASSERT(IS_POWER_OF_TWO(align));
    
    u64 align_used = (arena->used + (align - 1)) & ~(align - 1);
    ASSERT(align_used + size <= arena->size);
    
    void *result = arena->buffer + align_used;
    arena->used = align_used + size;
    
    return (void *)result;
}

void static_arena_free(Arena *arena) {
    ASSERT(arena->buffer);
    arena->used = 0;
}

/* -------------------
       VirtualArena
   ------------------- */

static void commit_more_pages(Arena *arena, u64 size) {
    
    u64 page_size = os_get_page_size(); 
    ASSERT(page_size > 0);
    
    u64 page_number = (size + (page_size - 1)) / page_size;
    u64 align_size = page_size * page_number;
    
    u64 commit_size = MAX(align_size, arena->size);
    
    ASSERT(arena->size + align_size <= DEFAULT_VIRTUAL_SPACE_RANGE);

    os_virtual_commit((void *)(arena->buffer + arena->size), commit_size);
    arena->size = arena->size + commit_size;
    
    ASSERT((arena->size & (page_size - 1)) == 0);
}

void virtual_arena_initialize(Arena *arena) {
    
    u64 page_size = os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));
    ASSERT(page_size > 0);
    
    u64 size = DEFAULT_VIRTUAL_SPACE_RANGE;
    u64 pages_number = (size + (page_size - 1)) / page_size;
    u64 align_size = page_size * pages_number;

    arena->size = 0;
    arena->used = 0;
    arena->buffer = os_virtual_reserve(align_size);

    arena->stack_alloc_size = 0;
}

void virtual_arena_terminate(Arena *arena) {
    ASSERT(arena->buffer) ;
    os_virtual_decommit(arena->buffer, arena->size);
    os_virtual_release(arena->buffer, DEFAULT_VIRTUAL_SPACE_RANGE);
}

void *virtual_arena_alloc(Arena *arena, u64 size, u32 align) {

    u64 align_used = (arena->used + (align - 1)) & ~(align - 1);
    
    if(align_used + size > arena->size) {
        commit_more_pages(arena, size);
    }

    ASSERT(align_used + size <= arena->size);
    
    void *result = arena->buffer + align_used;
    arena->used = align_used + size;
    
    return (void *)result;
}

void virtual_arena_free(Arena *arena) {
    ASSERT(arena->buffer);
    arena->used = 0;
}

/* -------------------
       VirtualMap
   ------------------- */

static inline void set_all_buckets_as_free(VirtualMapBucket *buckets, u64 size) {
    for(u64 bucket_index = 0; bucket_index < size; ++bucket_index) {
        VirtualMapBucket *bucket = buckets + bucket_index;
        bucket->key = VIRTUAL_MAP_BUCKET_FREE;
        bucket->data = NULL;
    }
}

static inline VirtualMapBucket *find_free_bucket(VirtualMapBucket *buckets, u64 key, u64 size) {
    u64 bucket_index = key % size;
    VirtualMapBucket *bucket = buckets + bucket_index;
    while(bucket->key != VIRTUAL_MAP_BUCKET_FREE && bucket->key != VIRTUAL_MAP_BUCKET_TUMBSTONE) {
        bucket_index = (bucket_index + 1) % size;
        bucket = buckets + bucket_index;

    }
    ASSERT(bucket->key == VIRTUAL_MAP_BUCKET_FREE || bucket->key == VIRTUAL_MAP_BUCKET_TUMBSTONE);
    return bucket;
}

static inline void copy_bucket_into_buckets(VirtualMapBucket *buckets, VirtualMapBucket *src_bucket, u64 size) {
    VirtualMapBucket *dst_bucket = find_free_bucket(buckets, src_bucket->key, size);
    *dst_bucket = *src_bucket;
}

static void resize_and_rehash_virtual_map(VirtualMap *map) {
    
    u64 page_size = os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));
    
    ASSERT(map->memory_buffer_index == 0 || map->memory_buffer_index == 1);
    b32 new_memory_buffer_index = map->memory_buffer_index == 1 ? 0 : 1;

    VirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    VirtualMapBucket *new_buckets = map->memory_buffer[new_memory_buffer_index];
    
    u64 new_size = map->size * 2;
    u64 new_size_in_bytes = sizeof(VirtualMapBucket) * new_size;
    u64 pages_number = (new_size_in_bytes + (page_size - 1)) / page_size;
    u64 new_size_align = pages_number * page_size;
    
    os_virtual_commit(new_buckets, new_size_align);
    set_all_buckets_as_free(new_buckets, new_size);

    for(u64 bucket_index = 0; bucket_index < map->size; ++bucket_index) {
        VirtualMapBucket *bucket = buckets + bucket_index; 
        
        if(bucket->key == VIRTUAL_MAP_BUCKET_TUMBSTONE) {
            --map->used;
        }else if(bucket->key != VIRTUAL_MAP_BUCKET_FREE) {
            copy_bucket_into_buckets(new_buckets, bucket, new_size);
        }
    }

    os_virtual_decommit(buckets, map->size_in_bytes);
    
    map->size = new_size;
    map->size_in_bytes = new_size_align;
    map->memory_buffer_index = new_memory_buffer_index;
}

void virtual_map_initialize(VirtualMap *map) {

    u64 page_size = os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));

    map->memory_buffer[0] = os_virtual_reserve(DEFAULT_VIRTUAL_SPACE_RANGE);
    map->memory_buffer[1] = os_virtual_reserve(DEFAULT_VIRTUAL_SPACE_RANGE);
    map->memory_buffer_index = 0;

    VirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];

    u64 size = sizeof(VirtualMapBucket) * DEFAULT_VIRTUAL_MAP_SIZE;
    u64 pages_number = (size + (page_size - 1)) / page_size;
    u64 size_align = pages_number * page_size;

    os_virtual_commit(buckets, size_align);

    map->size_in_bytes = size_align;
    map->size = DEFAULT_VIRTUAL_MAP_SIZE;
    map->used = 0;
    
    set_all_buckets_as_free(buckets, map->size);
}

void virtual_map_terminate(VirtualMap *map) {
    VirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    os_virtual_decommit(buckets, map->size_in_bytes);
    os_virtual_release(map->memory_buffer[0], DEFAULT_VIRTUAL_SPACE_RANGE);
    os_virtual_release(map->memory_buffer[1], DEFAULT_VIRTUAL_SPACE_RANGE);
}

void virtual_map_insert(VirtualMap *map, u64 key, void *data) {
    
    if((map->used + 1) > (map->size * 0.7)) {
        resize_and_rehash_virtual_map(map);
    }
    ASSERT((map->used + 1) <= (map->size * 0.7));
    
    VirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    VirtualMapBucket *bucket = find_free_bucket(buckets, key, map->size);
    
    if(bucket->key == VIRTUAL_MAP_BUCKET_FREE) {
        map->used = map->used + 1;
    }
    
    bucket->key = key;
    bucket->data = data;
}

void virtual_map_remove(VirtualMap *map, u64 key) {

    VirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    
    u64 bucket_index = key % map->size;
    VirtualMapBucket *bucket = buckets + bucket_index;
    
    while(bucket->key != key) {
        bucket_index = (bucket_index + 1) % map->size;
        bucket = buckets + bucket_index;

    }
    
    ASSERT(bucket->key == key);
    bucket->key = VIRTUAL_MAP_BUCKET_TUMBSTONE;
    bucket->data = NULL;
}

void *virtual_map_find(VirtualMap *map, u64 key) {

    VirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];

    u64 bucket_index = key % map->size;
    VirtualMapBucket *bucket = buckets + bucket_index;
    while(bucket->key != VIRTUAL_MAP_BUCKET_FREE && bucket->key != key) {
        bucket_index = (bucket_index + 1) % map->size;
        bucket = buckets + bucket_index;

    }

    if(bucket->key == VIRTUAL_MAP_BUCKET_FREE) {
        return NULL;
    }

    return bucket->data;
}

b32 virtual_map_contains(VirtualMap *map, u64 key) {
    return virtual_map_find(map, key) != NULL;
}