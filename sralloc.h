#ifndef SR_ALLOC_TYPES
typedef int srint_t;
#endif

typedef struct {
} srstats_malloc;

typedef struct {
    int num_allocations;
    int amount_allocated;
} srstats;

typedef struct {
    void* ptr;
    int   size;
} allocation_result_t;

struct srallocator_t;
typedef struct {
    const char*         name;
    srallocator_t*      parent;
    srallocator_t*      children;
    int                 num_children;
    srstats             stats;
    void*               allocate( srallocator_t* allocator, srint_t size, srint_t align );
    allocation_result_t allocate_get_size( srallocator_t* allocator, srint_t size, srint_t align );
    void                deallocate( srallocator_t* allocator, void* ptr );
} srallocator_t;

//  UTIL
void*
sralloc_allocate( srallocator_t* allocator, srint_t size, srint_t align ) {
    return allocator->allocate_get_size( allocator, size, align ).ptr;
}

// MALLOC
allocation_result_t
sralloc_malloc_allocate( srallocator_t* allocator, srint_t size ) {
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
    srint_t* ptr            = (srint_t*)SRALLOC_MALLOC( size + sizeof( srint_t ) );
    *ptr                    = size;
    allocation_result_t res = { (void*)( ptr + 1 ), size };
    return res;
}

void
sralloc_malloc_deallocate( srallocator_t* allocator, void* ptr ) {
    int* malloc_ptr = (srint_t*)ptr;
    malloc_ptr--;
    allocator->stats.amount_allocated -= *malloc_ptr;
    allocator->stats.num_allocations--;
    return SRALLOC_FREE( malloc_ptr );
}

srallocator_t*
sralloc_create_malloc( const char* name, void* ) {
    srallocator_t* allocator = SRALLOC_MALLOC( sizeof( srallocator_t ) );
    SRALLOC_MEMSET( allocator, 0, sizeof( srallocator_t ) );
    allocator->name              = name;
    allocator->allocate          = sralloc_allocate;
    allocator->allocate_get_size = sralloc_malloc_allocate;
    allocator->deallocate        = sralloc_malloc_deallocate;
    return allocator;
};

void
sralloc_destroy_malloc( srallocator_t* allocator ) {
    SRALLOC_ASSERT( allocator->num_children == 0 );
    SRALLOC_ASSERT( allocator->stats.num_allocations == 0 );
    SRALLOC_ASSERT( allocator->stats.amount_allocated == 0 );
    SRALLOC_FREE( allocator );
};

///////////////////////////////////////
// SLOT
typedef struct {
    void*   slots;
    srint_t num_slots;
    srint_t slot_size;
} srallocator_slot_t;

void*
sralloc_slot_allocate( srallocator_t* allocator, srint_t size ) {
    srallocator_slot_t* slot_a = (srallocator_slot_t*)( allocator + 1 );
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
    srint_t* ptr = (srint_t*)slot( size + sizeof( srint_t ) );
    *ptr         = size;
    return (void*)( ptr + 1 );
}

void
sralloc_slot_deallocate( srallocator_t* allocator, void* ptr ) {
    srint_t* slot_ptr = (srint_t*)ptr;
    slot_ptr--;
    allocator->stats.amount_allocated -= *slot_ptr;
    allocator->stats.num_allocations--;
    return free( slot_ptr );
}

srallocator_t*
sralloc_create_slot( srallocator_t* parent,
                     const char*    name,
                     srint_t        slot_size,
                     srint_t        num_slots ) {
    srint_t allocator_size =
      sizeof( srallocator_t ) + sizeof( srallocator_slot_t ) + slot_size * num_slots;
    srallocator_t*      allocator      = parent->allocate( parent, allocator_size );
    srallocator_slot_t* slot_allocator = (srallocator_slot_t*)( allocator + 1 );

    SRALLOC_MEMSET( allocator, 0, sizeof( srallocator_t ) );
    allocator->name           = name;
    allocator->parent         = parent;
    allocator->allocate       = sralloc_slot_allocate;
    allocator->deallocate     = sralloc_slot_deallocate;
    slot_allocator->num_slots = num_slots;
    slot_allocator->slot_size = slot_size;
    slot_allocator->slots     = (void*)slot_allocator + 1;
    return allocator;
};

void
sralloc_destroy_slot( srallocator_t* allocator ) {
    SRALLOC_ASSERT( allocator->num_children == 0 );
    SRALLOC_ASSERT( allocator->stats.num_allocations == 0 );
    SRALLOC_ASSERT( allocator->stats.amount_allocated == 0 );
    allocator->parent->deallocate( allocator->parent, allocator );
};

#define DO_TEST
#ifdef DO_TEST

srallocator_t g_rootalloc;

int
main() {
    g_rootalloc = sralloc_create_malloc();

    return 0;
}

#endif