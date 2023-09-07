#include "tgui_os.h"
#include "tgui_memory.h"

/* ----------------------
      Arena Interface
   ---------------------- */

void tgui_arena_initialize(TGuiArena *arena, u64 size, TGuiArenaType type) {

    memset(arena, 0, sizeof(TGuiArena));
    arena->type = type;

    switch (type) {
    case TGUI_ARENA_TYPE_STATIC: { 
        tgui_static_arena_initialize(arena, size); 
        arena->alloc = tgui_static_arena_alloc;
        arena->free = tgui_static_arena_free;
    } break;
    case TGUI_ARENA_TYPE_VIRTUAL: {
        tgui_virtual_arena_initialize(arena); 
        arena->alloc = tgui_virtual_arena_alloc;
        arena->free = tgui_virtual_arena_free;
    } break;
    }
}

void tgui_arena_terminate(TGuiArena *arena) {
    switch (arena->type) {
    case TGUI_ARENA_TYPE_STATIC: { 
        tgui_static_arena_terminate(arena); 
    } break;
    case TGUI_ARENA_TYPE_VIRTUAL: {
        tgui_virtual_arena_terminate(arena); 
    } break;
    }
}

/* -------------------
      StaticArena 
   ------------------- */

void tgui_static_arena_initialize(TGuiArena *arena, u64 size) {
    u64 page_size = tgui_os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));
    ASSERT(page_size > 0);
    
    u64 page_number = (size + (page_size - 1)) / page_size;
    u64 align_size = page_size * page_number;

    arena->size = align_size;
    arena->used = 0;
    arena->buffer = tgui_os_virtual_reserve(align_size);
    tgui_os_virtual_commit(arena->buffer, align_size);
}

void tgui_static_arena_terminate(TGuiArena *arena) {
    ASSERT(arena->buffer);
    tgui_os_virtual_decommit(arena->buffer, arena->size);
    tgui_os_virtual_release(arena->buffer, arena->size);
}

void *tgui_static_arena_alloc(TGuiArena *arena, u64 size, u32 align) {
    ASSERT(arena->buffer);
    ASSERT(IS_POWER_OF_TWO(align));
    
    u64 align_used = (arena->used + (align - 1)) & ~(align - 1);
    ASSERT(align_used + size <= arena->size);
    
    void *result = arena->buffer + align_used;
    arena->used = align_used + size;
    
    return (void *)result;
}

void tgui_static_arena_free(TGuiArena *arena) {
    ASSERT(arena->buffer);
    arena->used = 0;
}

/* -------------------
       VirtualArena
   ------------------- */

static void commit_more_pages(TGuiArena *arena, u64 size) {
    
    u64 page_size = tgui_os_get_page_size(); 
    ASSERT(page_size > 0);
    
    u64 page_number = (size + (page_size - 1)) / page_size;
    u64 align_size = page_size * page_number;
    
    u64 commit_size = MAX(align_size, arena->size);
    
    ASSERT(arena->size + align_size <= TGUI_DEFAULT_VIRTUAL_SPACE_RANGE);

    tgui_os_virtual_commit((void *)(arena->buffer + arena->size), commit_size);
    arena->size = arena->size + commit_size;
    
    ASSERT((arena->size & (page_size - 1)) == 0);
}

void tgui_virtual_arena_initialize(TGuiArena *arena) {
    
    u64 page_size = tgui_os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));
    ASSERT(page_size > 0);
    
    u64 size = TGUI_DEFAULT_VIRTUAL_SPACE_RANGE;
    u64 pages_number = (size + (page_size - 1)) / page_size;
    u64 align_size = page_size * pages_number;

    arena->size = 0;
    arena->used = 0;
    arena->buffer = tgui_os_virtual_reserve(align_size);
}

void tgui_virtual_arena_terminate(TGuiArena *arena) {
    ASSERT(arena->buffer) ;
    tgui_os_virtual_decommit(arena->buffer, arena->size);
    tgui_os_virtual_release(arena->buffer, TGUI_DEFAULT_VIRTUAL_SPACE_RANGE);
}

void *tgui_virtual_arena_alloc(TGuiArena *arena, u64 size, u32 align) {

    u64 align_used = (arena->used + (align - 1)) & ~(align - 1);
    
    if(align_used + size > arena->size) {
        commit_more_pages(arena, size);
        //printf("new pages commited\n");
    }

    ASSERT(align_used + size <= arena->size);
    
    void *result = arena->buffer + align_used;
    arena->used = align_used + size;

    //printf("arena arena used: %lld, size: %lld\n", arena->used, arena->size);
    
    return (void *)result;
}

void tgui_virtual_arena_free(TGuiArena *arena) {
    ASSERT(arena->buffer);
    arena->used = 0;
}

/* -------------------
       VirtualMap
   ------------------- */

static inline void set_all_buckets_as_free(TGuiVirtualMapBucket *buckets, u64 size) {
    for(u64 bucket_index = 0; bucket_index < size; ++bucket_index) {
        TGuiVirtualMapBucket *bucket = buckets + bucket_index;
        bucket->key = TGUI_VIRTUAL_MAP_BUCKET_FREE;
        bucket->data = NULL;
    }
}

static inline TGuiVirtualMapBucket *find_free_bucket(TGuiVirtualMapBucket *buckets, u64 key, u64 size) {
    u64 bucket_index = key % size;
    TGuiVirtualMapBucket *bucket = buckets + bucket_index;
    while(bucket->key != TGUI_VIRTUAL_MAP_BUCKET_FREE && bucket->key != TGUI_VIRTUAL_MAP_BUCKET_TUMBSTONE) {
        bucket_index = (bucket_index + 1) % size;
        bucket = buckets + bucket_index;

    }
    ASSERT(bucket->key == TGUI_VIRTUAL_MAP_BUCKET_FREE || bucket->key == TGUI_VIRTUAL_MAP_BUCKET_TUMBSTONE);
    return bucket;
}

static inline void copy_bucket_into_buckets(TGuiVirtualMapBucket *buckets, TGuiVirtualMapBucket *src_bucket, u64 size) {
    TGuiVirtualMapBucket *dst_bucket = find_free_bucket(buckets, src_bucket->key, size);
    *dst_bucket = *src_bucket;
}

static void resize_and_rehash_virtual_map(TGuiVirtualMap *map) {
    
    u64 page_size = tgui_os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));
    
    ASSERT(map->memory_buffer_index == 0 || map->memory_buffer_index == 1);
    b32 new_memory_buffer_index = map->memory_buffer_index == 1 ? 0 : 1;

    TGuiVirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    TGuiVirtualMapBucket *new_buckets = map->memory_buffer[new_memory_buffer_index];
    
    u64 new_size = map->size * 2;
    u64 new_size_in_bytes = sizeof(TGuiVirtualMapBucket) * new_size;
    u64 pages_number = (new_size_in_bytes + (page_size - 1)) / page_size;
    u64 new_size_align = pages_number * page_size;
    
    tgui_os_virtual_commit(new_buckets, new_size_align);
    set_all_buckets_as_free(new_buckets, new_size);

    for(u64 bucket_index = 0; bucket_index < map->size; ++bucket_index) {
        TGuiVirtualMapBucket *bucket = buckets + bucket_index; 
        
        if(bucket->key == TGUI_VIRTUAL_MAP_BUCKET_TUMBSTONE) {
            --map->used;
        }else if(bucket->key != TGUI_VIRTUAL_MAP_BUCKET_FREE) {
            copy_bucket_into_buckets(new_buckets, bucket, new_size);
        }
    }

    tgui_os_virtual_decommit(buckets, map->size_in_bytes);
    
    map->size = new_size;
    map->size_in_bytes = new_size_align;
    map->memory_buffer_index = new_memory_buffer_index;
}

void tgui_virtual_map_initialize(TGuiVirtualMap *map) {

    u64 page_size = tgui_os_get_page_size(); 
    ASSERT(IS_POWER_OF_TWO(page_size));

    map->memory_buffer[0] = tgui_os_virtual_reserve(TGUI_DEFAULT_VIRTUAL_SPACE_RANGE);
    map->memory_buffer[1] = tgui_os_virtual_reserve(TGUI_DEFAULT_VIRTUAL_SPACE_RANGE);
    map->memory_buffer_index = 0;

    TGuiVirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];

    u64 size = sizeof(TGuiVirtualMapBucket) * TGUI_DEFAULT_VIRTUAL_MAP_SIZE;
    u64 pages_number = (size + (page_size - 1)) / page_size;
    u64 size_align = pages_number * page_size;

    tgui_os_virtual_commit(buckets, size_align);

    map->size_in_bytes = size_align;
    map->size = TGUI_DEFAULT_VIRTUAL_MAP_SIZE;
    map->used = 0;
    
    set_all_buckets_as_free(buckets, map->size);
}

void tgui_virtual_map_terminate(TGuiVirtualMap *map) {
    TGuiVirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    tgui_os_virtual_decommit(buckets, map->size_in_bytes);
    tgui_os_virtual_release(map->memory_buffer[0], TGUI_DEFAULT_VIRTUAL_SPACE_RANGE);
    tgui_os_virtual_release(map->memory_buffer[1], TGUI_DEFAULT_VIRTUAL_SPACE_RANGE);
}

void tgui_virtual_map_insert(TGuiVirtualMap *map, u64 key, void *data) {
    
    if((map->used + 1) > (map->size * 0.7)) {
        resize_and_rehash_virtual_map(map);
    }
    ASSERT((map->used + 1) <= (map->size * 0.7));
    
    TGuiVirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    TGuiVirtualMapBucket *bucket = find_free_bucket(buckets, key, map->size);
    
    if(bucket->key == TGUI_VIRTUAL_MAP_BUCKET_FREE) {
        map->used = map->used + 1;
    }
    
    bucket->key = key;
    bucket->data = data;
}

void tgui_virtual_map_remove(TGuiVirtualMap *map, u64 key) {

    TGuiVirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];
    
    u64 bucket_index = key % map->size;
    TGuiVirtualMapBucket *bucket = buckets + bucket_index;
    
    while(bucket->key != key) {
        bucket_index = (bucket_index + 1) % map->size;
        bucket = buckets + bucket_index;

    }
    
    ASSERT(bucket->key == key);
    bucket->key = TGUI_VIRTUAL_MAP_BUCKET_TUMBSTONE;
    bucket->data = NULL;
}

void *tgui_virtual_map_find(TGuiVirtualMap *map, u64 key) {

    TGuiVirtualMapBucket *buckets = map->memory_buffer[map->memory_buffer_index];

    u64 bucket_index = key % map->size;
    TGuiVirtualMapBucket *bucket = buckets + bucket_index;
    while(bucket->key != TGUI_VIRTUAL_MAP_BUCKET_FREE && bucket->key != key) {
        bucket_index = (bucket_index + 1) % map->size;
        bucket = buckets + bucket_index;

    }

    if(bucket->key == TGUI_VIRTUAL_MAP_BUCKET_FREE) {
        return NULL;
    }

    return bucket->data;
}

b32 tgui_virtual_map_contains(TGuiVirtualMap *map, u64 key) {
    return tgui_virtual_map_find(map, key) != NULL;
}

/* ------------------------
        Virtual Array 
   ------------------------ */

void _tgui_array_initialize(TGuiVoidArray *array, u64 element_size) {
    ASSERT(array);
    tgui_arena_initialize(&array->arena, 0, TGUI_ARENA_TYPE_VIRTUAL);

    array->size = 0;
    array->capacity = TGUI_ARRAY_DEFAULT_CAPACITY;
    array->buffer = tgui_arena_alloc(&array->arena, TGUI_ARRAY_DEFAULT_CAPACITY*element_size, 8);
}

void _tgui_array_terminate(TGuiVoidArray *array) {
    array->buffer = NULL;
    array->size = 0;
    array->capacity = 0;
    tgui_arena_terminate(&array->arena);
}

void _tgui_array_push(TGuiVoidArray *array, u64 element_size) {
    if((array->size + 1) > array->capacity) {
        tgui_arena_alloc(&array->arena, array->size*element_size, 1);
        array->capacity += array->size;
        //printf("array increase capacity, size: %lld, capacity: %lld\n", array->size, array->capacity);
        //printf("arena arena used: %lld, size: %lld\n", array->arena.used, array->arena.size);
    }
    
    ASSERT(array->size < array->capacity);
    ++array->size;
}

void _tgui_array_reserve(TGuiVoidArray *array, u32 count, u64 element_size) {
    tgui_arena_alloc(&array->arena, count*element_size, 1);
    array->capacity += count;
    ASSERT(array->size < array->capacity);
    array->size += count;
}

void _tgui_array_clear(TGuiVoidArray *array) {
    array->size = 0;
}
