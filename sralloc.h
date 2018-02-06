
#ifndef INCLUDE_SRALLOC_H
#define INCLUDE_SRALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

struct srallocator_t;

// General API
SRALLOC_API void* sralloc_alloc( srallocator_t* allocator, srint_t size );
SRALLOC_API allocation_result_t sralloc_alloc_with_size( srallocator_t* allocator, srint_t size );
SRALLOC_API void* sralloc_alloc_aligned( srallocator_t* allocator, srint_t size, srint_t align );
SRALLOC_API allocation_result_t sralloc_alloc_aligned_with_size( srallocator_t* allocator,
                                                                 srint_t        size,
                                                                 srint_t        align );
SRALLOC_API void*               sralloc_dealloc( srallocator_t* allocator, void* ptr );
SRALLOC_API void* sralloc_allocate( srallocator_t* allocator, srint_t size, srint_t align );

// Malloc allocator (global allocator)
SRALLOC_API srallocator_t* sralloc_create_malloc_allocator( const char* name );
SRALLOC_API void           sralloc_destroy_malloc_allocator( srallocator_t* allocator );

// Stack allocator (or stack frame allocator)
SRALLOC_API      srallocator_t*
                 sralloc_create_stack_allocator( const char* name, srallocator_t* parent, srint_t capacity );
SRALLOC_API void sralloc_destroy_stack_allocator( srallocator_t* allocator );

// Slot allocator (for things of same size)
SRALLOC_API srallocator_t* sralloc_create_slot_allocator( srallocator_t* parent,
                                                          const char*    name,
                                                          srint_t        slot_size,
                                                          srint_t        capacity );
SRALLOC_API void           sralloc_destroy_slot_allocator( srallocator_t* allocator );

#ifdef SRALLOC_IMPLEMENTATION

#ifndef SRALLOC_TYPES
typedef int                srint_t;
typedef short              srshort_t;
typedef unsigned long long srptr_t; // TODO fix
#endif

#ifndef SRALLOC_malloc
#include <stdlib.h>
#define SRALLOC_malloc malloc
#define SRALLOC_free free
#endif

#ifndef SRALLOC_assert
#include <assert.h>
#define SRALLOC_assert assert
#endif

#ifndef SRALLOC_NULL
#define SRALLOC_NULL 0
#endif

#ifndef SRALLOC_ZERO_SIZE_PTR
#define SRALLOC_ZERO_SIZE_PTR ( (void*)16 ) // kmalloc style
#endif

#ifndef SRALLOC_UNUSED
#define SRALLOC_UNUSED( ... ) (void)( __VA_ARGS__ )
#endif

#ifndef SRALLOC_NOSTATS
#define SRALLOC_STATS
#endif

#if defined( SRALLOC_STATIC )
#define SRALLOC_API static
#else
#define SRALLOC_API extern
#endif

typedef struct {
    int num_allocations;
    int amount_allocated;
} sralloc_stats;

typedef struct {
    void* ptr;
    int   size;
} allocation_result_t;

// struct sralloc_handle_t;

typedef allocation_result_t ( *sralloc_allocate_func )( srallocator_t* allocator,
                                                        srint_t        size,
                                                        srint_t        align );
typedef void ( *sralloc_deallocate_func )( srallocator_t* allocator, void* ptr );

struct srallocator_t;
typedef struct {
    const char*             name;
    srallocator_t*          parent;
    srallocator_t*          children;
    srint_t                 num_children;
    sralloc_allocate_func   allocate_func;
    sralloc_deallocate_func deallocate_func;
#ifdef SRALLOC_STATS
    sralloc_stats stats;
#endif
} srallocator_t;

//  █████╗ ██████╗ ██╗
// ██╔══██╗██╔══██╗██║
// ███████║██████╔╝██║
// ██╔══██║██╔═══╝ ██║
// ██║  ██║██║     ██║
// ╚═╝  ╚═╝╚═╝     ╚═╝

SRALLOC_API void*
sralloc_alloc( srallocator_t* allocator, srint_t size ) {
    if ( size == 0 ) {
        return SRALLOC_ZERO_SIZE_PTR;
    }

    return allocator->allocate_func( allocator, size, 0 ).ptr;
}

SRALLOC_API allocation_result_t
            sralloc_alloc_with_size( srallocator_t* allocator, srint_t size ) {
    if ( size == 0 ) {
        return { .ptr = SRALLOC_ZERO_SIZE_PTR, .size = 0 };
    }

    return allocator->allocate_func( allocator, size, 0 );
}

SRALLOC_API void*
sralloc_alloc_aligned( srallocator_t* allocator, srint_t size, srint_t align ) {
    if ( size == 0 ) {
        return SRALLOC_ZERO_SIZE_PTR;
    }

    return allocator->allocate_func( allocator, size, align ).ptr;
}

SRALLOC_API allocation_result_t
            sralloc_alloc_aligned_with_size( srallocator_t* allocator, srint_t size, srint_t align ) {
    if ( size == 0 ) {
        return { .ptr = SRALLOC_ZERO_SIZE_PTR, .size = 0 };
    }

    return allocator->allocate_func( allocator, size, align );
}

SRALLOC_API void*
sralloc_dealloc( srallocator_t* allocator, void* ptr ) {
    if ( ptr == SRALLOC_ZERO_SIZE_PTR ) {
        return;
    }

    return allocator->deallocate_func( allocator, ptr );
}


// ███╗   ███╗ █████╗ ██╗     ██╗      ██████╗  ██████╗
// ████╗ ████║██╔══██╗██║     ██║     ██╔═══██╗██╔════╝
// ██╔████╔██║███████║██║     ██║     ██║   ██║██║
// ██║╚██╔╝██║██╔══██║██║     ██║     ██║   ██║██║
// ██║ ╚═╝ ██║██║  ██║███████╗███████╗╚██████╔╝╚██████╗
// ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝  ╚═════╝

static allocation_result_t
sralloc_malloc_allocate( srallocator_t* allocator, srint_t size, srint_t align ) {
    SRALLOC_UNUSED( align );
#ifdef SRALLOC_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif
    srint_t* ptr            = (srint_t*)SRALLOC_MALLOC( size + sizeof( srint_t ) );
    *ptr                    = size;
    allocation_result_t res = { (void*)( ptr + 1 ), size };
    return res;
}

static void
sralloc_malloc_deallocate( srallocator_t* allocator, void* ptr ) {
    int* malloc_ptr = (srint_t*)ptr;
    int  size_ptr   = malloc_ptr - 1;
    int  size       = *size_ptr;
#ifdef SRALLOC_STATS
    allocator->stats.amount_allocated -= size;
    allocator->stats.num_allocations--;
#endif
    return SRALLOC_FREE( size_ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_malloc_allocator( const char* name ) {
    srallocator_t* allocator = SRALLOC_MALLOC( sizeof( srallocator_t ) );
    SRALLOC_MEMSET( allocator, 0, sizeof( srallocator_t ) );
    allocator->name            = name;
    allocator->allocate_func   = sralloc_malloc_allocate;
    allocator->deallocate_func = sralloc_malloc_deallocate;
    return allocator;
};

SRALLOC_API void
sralloc_destroy_malloc_allocator( srallocator_t* allocator ) {
    SRALLOC_assert( allocator->num_children == 0 );
#ifdef SRALLOC_STATS
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    SRALLOC_FREE( allocator );
};

// ███████╗████████╗ █████╗  ██████╗██╗  ██╗
// ██╔════╝╚══██╔══╝██╔══██╗██╔════╝██║ ██╔╝
// ███████╗   ██║   ███████║██║     █████╔╝
// ╚════██║   ██║   ██╔══██║██║     ██╔═██╗
// ███████║   ██║   ██║  ██║╚██████╗██║  ██╗
// ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝

typedef struct {
    void* top;
    void* end;
} srallocator_stack_t;

typedef struct {
    void* top;
#ifdef SRALLOC_STATS
    sralloc_stats stats;
#endif
} srallocator_stack_state_t;

static allocation_result_t
sralloc_stack_allocate( srallocator_t* allocator, srint_t size, srint_t align ) {
    SRALLOC_UNUSED( align );
#ifdef SRALLOC_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif

    srallocator_stack_t* stack_allocator = (srallocator_slot_t*)( allocator + 1 );
    void*                ptr             = stack_allocator->top;
    stack_allocator->top                 = ptr + size;
    allocation_result_t res              = { (void*)( ptr ), size };
    return res;
}

static void
sralloc_stack_deallocate( srallocator_t* allocator, void* ptr ) {
    srallocator_stack_t* stack_allocator = (srallocator_slot_t*)( allocator + 1 );
    SRALLOC_assert( ptr < stack_allocator->top );

    srint_t size         = ptr - stack_allocator->top;
    stack_allocator->top = ptr;
#ifdef SRALLOC_STATS
    allocator->stats.amount_allocated -= size;
    allocator->stats.num_allocations--;
#else
    SRALLOC_UNUSED( size );
#endif
}

SRALLOC_API void
sralloc_stack_allocator_clear( srallocator_t* allocator ) {
    srallocator_stack_t* stack_allocator = (srallocator_slot_t*)( allocator + 1 );
    stack_allocator->top                 = stack_allocator + 1;
#ifdef SRALLOC_STATS
    allocator->stats.amount_allocated = 0;
    allocator->stats.num_allocations  = 0;
#endif
}

SRALLOC_API void
sralloc_stack_allocator_push_state( srallocator_t* allocator ) {
    srallocator_stack_t*       stack_allocator = (srallocator_slot_t*)( allocator + 1 );
    srallocator_stack_state_t* state           = &stack_allocator[stack_allocator->num_states++];
    state->top                                 = stack_allocator->top;
#ifdef SRALLOC_STATS
    state->stats = allocator->stats;
#endif
}

SRALLOC_API void
sralloc_stack_allocator_pop_state( srallocator_t* allocator ) {
    srallocator_stack_t*       stack_allocator = (srallocator_slot_t*)( allocator + 1 );
    srallocator_stack_state_t* state           = &stack_allocator[--stack_allocator->num_states];
    stack_allocator->top                       = state->top;
#ifdef SRALLOC_STATS
    allocator->stats = state->stats;
#endif
}

SRALLOC_API srallocator_t*
            sralloc_create_stack_allocator( const char* name, srallocator_t* parent, srint_t capacity ) {
    srint_t allocator_size = sizeof( srallocator_t ) + sizeof( srallocator_stack_t ) + capacity;
    void*   memory         = parent->allocate( parent, allocator_size );
    srallocator_t*       allocator       = (srallocator_t*)memory;
    srallocator_stack_t* stack_allocator = (srallocator_slot_t*)( allocator + 1 );

    SRALLOC_MEMSET( allocator, 0, allocator_size );
    allocator->name       = name;
    allocator->parent     = parent;
    allocator->allocate   = sralloc_stack_allocate;
    allocator->deallocate = sralloc_stack_deallocate;
    stack_allocator->top  = stack_allocator + 1;
    stack_allocator->end  = ( (char*)stack_allocator->top ) + capacity;
    return allocator;
};

SRALLOC_API void
sralloc_destroy_stack_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_STATS
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    SRALLOC_assert( allocator->num_children == 0 );
    SRALLOC_FREE( allocator );
};

// ███████╗██╗      ██████╗ ████████╗
// ██╔════╝██║     ██╔═══██╗╚══██╔══╝
// ███████╗██║     ██║   ██║   ██║
// ╚════██║██║     ██║   ██║   ██║
// ███████║███████╗╚██████╔╝   ██║
// ╚══════╝╚══════╝ ╚═════╝    ╚═╝

typedef struct {

} srallocator_slot_t;

void*
sralloc_slot_allocate( srallocator_t* allocator, srint_t size ) {
    srallocator_slot_t* slot_allocator = (srallocator_slot_t*)( allocator + 1 );
    srshort_t           slot           = slot_allocator->free_slot;
    if ( slot_allocator->free_slot != 0 ) {
        srshort_t next_slot_offset = slot * slot_allocator->slot_size;
        char*     next_free_slot   = (char*)slot_allocator->slots + next_slot_offset;
        slot_allocator->free_slot  = *(srshort_t*)next_free_slot;
    }
    else {
        SRALLOC_assert( slot_allocator->num_slots < slot_allocator->capacity );
        slot = slot_allocator->num_slots++;
    }

    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;

    srshort_t slot_offset = slot * slot_allocator->slot_size;
    char*     slot        = (char*)slot_allocator->slots + next_slot_offset;
    void*     ptr         = (void*)slot;
    return ptr;
}

void
sralloc_slot_deallocate( srallocator_t* allocator, void* ptr ) {
    srallocator_slot_t* slot_allocator = (srallocator_slot_t*)( allocator + 1 );
    srint_t*            slot_ptr       = (srint_t*)ptr;
    if ( slot_allocator->free_slot == 0 ) {
    slot_allocator->free_slot = (slot_ptr - slot_allocator->slots;
    }

    slot_ptr--;
    allocator->stats.amount_allocated -= *slot_ptr;
    allocator->stats.num_allocations--;
    return free( slot_ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_slot_allocator( srallocator_t* parent,
                                           const char*    name,
                                           srint_t        slot_size,
                                           srint_t        capacity ) {

    srint_t allocator_size =
      sizeof( srallocator_t ) + sizeof( srallocator_slot_t ) + slot_size * capacity;
    srallocator_t*      allocator      = parent->allocate( parent, allocator_size );
    srallocator_slot_t* slot_allocator = (srallocator_slot_t*)( allocator + 1 );

    SRALLOC_MEMSET( allocator, 0, sizeof( srallocator_t ) );
    allocator->name           = name;
    allocator->parent         = parent;
    allocator->allocate       = sralloc_slot_allocate;
    allocator->deallocate     = sralloc_slot_deallocate;
    slot_allocator->num_slots = 0;
    slot_allocator->capacity  = capacity;
    slot_allocator->slot_size = slot_size;
    slot_allocator->slots     = (void*)slot_allocator + 1;
    return allocator;
};

SRALLOC_API void
sralloc_destroy_slot_allocator( srallocator_t* allocator ) {
    SRALLOC_assert( allocator->num_children == 0 );
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
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