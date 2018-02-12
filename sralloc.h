
#ifndef INCLUDE_SRALLOC_H
#define INCLUDE_SRALLOC_H

#ifndef SRALLOC_ENABLE_WARNINGS
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4820 4548 4711 )
#endif

#ifdef __clang__
#pragma clang diagnostic push
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
#endif // SRALLOC_ENABLE_WARNINGS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct srallocator srallocator_t;

#ifndef SRALLOC_TYPES
#include <stdint.h>
typedef char      srchar_t;
typedef int       srint_t;
typedef short     srshort_t;
typedef uintptr_t srintptr_t;
#endif

typedef struct {
    void*   ptr;
    srint_t size;
} allocation_result_t;

#if defined( SRALLOC_STATIC )
#define SRALLOC_API static
#else
#define SRALLOC_API extern
#endif

// General API
SRALLOC_API void* sralloc_alloc( srallocator_t* allocator, srint_t size );
SRALLOC_API allocation_result_t sralloc_alloc_with_size( srallocator_t* allocator, srint_t size );
SRALLOC_API void* sralloc_alloc_aligned( srallocator_t* allocator, srint_t size, srint_t align );
SRALLOC_API allocation_result_t sralloc_alloc_aligned_with_size( srallocator_t* allocator,
                                                                 srint_t        size,
                                                                 srint_t        align );
SRALLOC_API void                sralloc_dealloc( srallocator_t* allocator, void* ptr );
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

// Util API. BYTES and DEALLOC only here for consistency.
#ifndef SRALLOC_ALIGNOF
#define SRALLOC_ALIGNOF alignof
#endif

#define SRALLOC_BYTES( allocator, size ) sralloc_alloc( allocator, size )
#define SRALLOC_OBJECT( allocator, type ) ( (type*)sralloc_alloc( allocator, sizeof( size ) ) )
#define SRALLOC_ARRAY( allocator, type, length ) \
    ( (type*)sralloc_alloc( allocator, sizeof( size ) * length ) );

#define SRALLOC_ALIGNED_BYTES( allocator, size, align ) \
    sralloc_alloc_aligned( allocator, size, align )
#define SRALLOC_ALIGNED_OBJECT( allocator, type ) \
    ( (type*)sralloc_alloc_aligned( allocator, sizeof( type ), SRALLOC_ALIGNOF( type ) ) )
#define SRALLOC_ALIGNED_ARRAY( allocator, type, length ) \
    ( (type*)sralloc_alloc_aligned( allocator, sizeof( type ) * length ), SRALLOC_ALIGNOF( type ) );

#define SRALLOC_DEALLOC( allocator, ptr ) sralloc_alloc(allocator, sizeof(size) * length));

#ifdef __cplusplus
}
#endif

// Implementation
#ifdef SRALLOC_IMPLEMENTATION

#ifndef SRALLOC_malloc
#include <stdlib.h>
#define SRALLOC_malloc malloc
#define SRALLOC_free free
#endif

#ifndef SRALLOC_assert
#include <assert.h>
#define SRALLOC_assert assert
#endif

#ifndef SRALLOC_memset
#include <string.h>
#define SRALLOC_memset memset
#endif

#ifndef SRALLOC_memcpy
#include <string.h>
#define SRALLOC_memcpy memcpy
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

#ifndef SRALLOC_NO_DEALLOC_PATTERN
// TODO
#endif

#ifndef SRALLOC_DISABLE_STATS
#define SRALLOC_USE_STATS
#endif

typedef struct {
    int num_allocations;
    int amount_allocated;
} sralloc_stats_t;

typedef allocation_result_t ( *sralloc_allocate_func )( srallocator_t* allocator,
                                                        srint_t        size,
                                                        srint_t        align );
typedef void ( *sralloc_deallocate_func )( srallocator_t* allocator, void* ptr );

struct srallocator {
    const char*             name;
    srallocator_t*          parent;
    srallocator_t**         children;
    srint_t                 num_children;
    srint_t                 children_capacity;
    sralloc_allocate_func   allocate_func;
    sralloc_deallocate_func deallocate_func;
#ifdef SRALLOC_USE_STATS
    sralloc_stats_t stats;
#endif
};

// ██╗███╗   ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
// ██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
// ██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
// ██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
// ██║██║ ╚████║   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
// ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝

void
sralloc_add_child_allocator( srallocator_t* parent, srallocator_t* child ) {
    if ( parent->children_capacity > parent->num_children ) {
        parent->children[parent->num_children++] = child;
        return;
    }

    parent->children_capacity = parent->children_capacity ? parent->children_capacity * 2 : 2;

    void* new_children = SRALLOC_NULL;
    if ( parent->parent != SRALLOC_NULL ) {
        new_children = (srallocator_t*)sralloc_alloc(
          parent->parent, sizeof( parent->children ) * parent->children_capacity );
        SRALLOC_memcpy(
          new_children, parent->children, parent->num_children * sizeof( parent->children ) );
        sralloc_dealloc( parent->parent, parent->children );
        parent->children                         = (srallocator_t**)new_children;
        parent->children[parent->num_children++] = child;
    }
    else {
        new_children = (srallocator_t*)sralloc_alloc(
          parent, sizeof( parent->children ) * parent->children_capacity );
        SRALLOC_memcpy(
          new_children, parent->children, parent->num_children * sizeof( parent->children ) );
        sralloc_dealloc( parent, parent->children );
        parent->children                         = (srallocator_t**)new_children;
        parent->children[parent->num_children++] = child;
    }
}

void
sralloc_remove_child_allocator( srallocator_t* parent, srallocator_t* child ) {
    for ( srint_t i_child = 0; i_child < parent->num_children; ++i_child ) {
        if ( parent->children[i_child] == child ) {
            parent->children[i_child] = parent->children[--parent->num_children];

            if ( parent->num_children == 0 ) {
                if ( parent->parent != SRALLOC_NULL ) {
                    sralloc_dealloc( parent->parent, parent->children );
                }
                else {
                    sralloc_dealloc( parent, parent->children );
                }
            }

            parent->children          = SRALLOC_NULL;
            parent->children_capacity = 0;

            return;
        }
    }

    SRALLOC_assert( 0 );
}

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
        allocation_result_t res = { SRALLOC_ZERO_SIZE_PTR, 0 };
        return res;
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
        allocation_result_t res = { SRALLOC_ZERO_SIZE_PTR, 0 };
        return res;
    }

    return allocator->allocate_func( allocator, size, align );
}

SRALLOC_API void
sralloc_dealloc( srallocator_t* allocator, void* ptr ) {
    if ( ptr == SRALLOC_ZERO_SIZE_PTR ) {
        return;
    }

    if ( ptr == SRALLOC_NULL ) {
        return;
    }

    allocator->deallocate_func( allocator, ptr );
}

// ███╗   ███╗ █████╗ ██╗     ██╗      ██████╗  ██████╗
// ████╗ ████║██╔══██╗██║     ██║     ██╔═══██╗██╔════╝
// ██╔████╔██║███████║██║     ██║     ██║   ██║██║
// ██║╚██╔╝██║██╔══██║██║     ██║     ██║   ██║██║
// ██║ ╚═╝ ██║██║  ██║███████╗███████╗╚██████╔╝╚██████╗
// ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝  ╚═════╝

static allocation_result_t
sralloc_malloc_allocate( srallocator_t* allocator, srint_t size, srint_t align ) {
    SRALLOC_UNUSED( allocator );
    size += align;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif
    srint_t   metadata_size = sizeof( srint_t ) + sizeof( srint_t );
    srchar_t* unaligned_ptr = SRALLOC_malloc( size + metadata_size + align );
    if ( unaligned_ptr == SRALLOC_NULL ) {
        allocation_result_t res = { SRALLOC_NULL, 0 };
        return res;
    }

    // | unaligned_ptr | size | offset | ptr |
    // srint_t to_ptr =
    //   align == 0 ? sizeof( srint_t ) * 2 : align - ( ( (srintptr_t)unaligned_ptr ) % align );
    srchar_t* ptr_after_alignment = unaligned_ptr + metadata_size + align;
    srint_t   alignment_padding   = align == 0 ? 0 : ( (srintptr_t)ptr_after_alignment % align );
    srchar_t* ptr                 = ptr_after_alignment - alignment_padding;
    srint_t*  offset_ptr          = ( (srint_t*)ptr ) - 1;
    srint_t*  size_ptr            = offset_ptr - 1;
    *offset_ptr                   = ( srint_t )( ptr - unaligned_ptr );
    *size_ptr                     = size;
    allocation_result_t res;
    res.ptr  = (void*)ptr;
    res.size = size;
    return res;
}

static void
sralloc_malloc_deallocate( srallocator_t* allocator, void* ptr ) {
    SRALLOC_UNUSED( allocator );
    srint_t*  malloc_ptr    = (srint_t*)ptr;
    srint_t*  offset_ptr    = malloc_ptr - 1;
    srint_t*  size_ptr      = offset_ptr - 1;
    srint_t   offset        = *offset_ptr;
    srint_t   size          = *size_ptr;
    srchar_t* unaligned_ptr = (srchar_t*)ptr - offset;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated -= size;
    allocator->stats.num_allocations--;
#else
    SRALLOC_UNUSED( size );
#endif
    SRALLOC_free( unaligned_ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_malloc_allocator( const char* name ) {
    srallocator_t* allocator = (srallocator_t*)SRALLOC_malloc( sizeof( srallocator_t ) );
    SRALLOC_memset( allocator, 0, sizeof( srallocator_t ) );
    allocator->name            = name;
    allocator->allocate_func   = sralloc_malloc_allocate;
    allocator->deallocate_func = sralloc_malloc_deallocate;
    return allocator;
}

SRALLOC_API void
sralloc_destroy_malloc_allocator( srallocator_t* allocator ) {
    SRALLOC_assert( allocator->num_children == 0 );
#ifdef SRALLOC_USE_STATS
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    SRALLOC_free( allocator );
}

// ███████╗████████╗ █████╗  ██████╗██╗  ██╗
// ██╔════╝╚══██╔══╝██╔══██╗██╔════╝██║ ██╔╝
// ███████╗   ██║   ███████║██║     █████╔╝
// ╚════██║   ██║   ██╔══██║██║     ██╔═██╗
// ███████║   ██║   ██║  ██║╚██████╗██║  ██╗
// ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝

typedef struct {
    void* top;
#ifdef SRALLOC_USE_STATS
    sralloc_stats_t stats;
#endif
} srallocator_stack_state_t;

typedef struct {
    void*                     top;
    void*                     end;
    srallocator_stack_state_t states[4];
    srint_t                   num_states;
} srallocator_stack_t;

static allocation_result_t
sralloc_stack_allocate( srallocator_t* allocator, srint_t size, srint_t align ) {
    SRALLOC_UNUSED( align );
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif

    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    char*                ptr             = (char*)stack_allocator->top;
    stack_allocator->top                 = ptr + size;
    SRALLOC_assert( stack_allocator->top <= stack_allocator->end );
    allocation_result_t res;
    res.ptr  = (void*)( ptr );
    res.size = size;
    return res;
}

static void
sralloc_stack_deallocate( srallocator_t* allocator, void* ptr ) {
    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    SRALLOC_assert( ptr < stack_allocator->top );

    srint_t size         = ( srint_t )( (char*)stack_allocator->top - (char*)ptr );
    stack_allocator->top = ptr;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated -= size;
    allocator->stats.num_allocations--;
#else
    SRALLOC_UNUSED( size );
#endif
}

SRALLOC_API void
sralloc_stack_allocator_clear( srallocator_t* allocator ) {
    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    stack_allocator->top                 = stack_allocator + 1;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated = 0;
    allocator->stats.num_allocations  = 0;
#endif
}

SRALLOC_API void
sralloc_stack_allocator_push_state( srallocator_t* allocator ) {
    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    SRALLOC_assert(
      stack_allocator->num_states <
      ( srint_t )( sizeof( stack_allocator->states ) / sizeof( srallocator_stack_state_t ) ) );
    srallocator_stack_state_t* state = &stack_allocator->states[stack_allocator->num_states++];
    state->top                       = stack_allocator->top;
#ifdef SRALLOC_USE_STATS
    state->stats = allocator->stats;
#endif
}

SRALLOC_API void
sralloc_stack_allocator_pop_state( srallocator_t* allocator ) {
    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    SRALLOC_assert( stack_allocator->num_states > 0 );
    srallocator_stack_state_t* state = &stack_allocator->states[--stack_allocator->num_states];
    stack_allocator->top             = state->top;
#ifdef SRALLOC_USE_STATS
    allocator->stats = state->stats;
#endif
}

SRALLOC_API srallocator_t*
            sralloc_create_stack_allocator( const char* name, srallocator_t* parent, srint_t capacity ) {
    srint_t allocator_size = sizeof( srallocator_t ) + sizeof( srallocator_stack_t ) + capacity;
    void*   memory         = sralloc_alloc( parent, allocator_size );
    srallocator_t*       allocator       = (srallocator_t*)memory;
    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );

    sralloc_add_child_allocator( parent, allocator );

    SRALLOC_memset( allocator, 0, allocator_size );
    allocator->name             = name;
    allocator->parent           = parent;
    allocator->allocate_func    = sralloc_stack_allocate;
    allocator->deallocate_func  = sralloc_stack_deallocate;
    stack_allocator->top        = stack_allocator + 1;
    stack_allocator->end        = ( (char*)stack_allocator->top ) + capacity;
    stack_allocator->num_states = 0;
    return allocator;
}

SRALLOC_API void
sralloc_destroy_stack_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_USE_STATS
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    sralloc_remove_child_allocator( allocator->parent, allocator );

    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    SRALLOC_UNUSED( stack_allocator );
    SRALLOC_assert( stack_allocator->num_states == 0 );
    SRALLOC_assert( allocator->num_children == 0 );
    sralloc_dealloc( allocator->parent, allocator );
}

    // ███████╗██╗      ██████╗ ████████╗
    // ██╔════╝██║     ██╔═══██╗╚══██╔══╝
    // ███████╗██║     ██║   ██║   ██║
    // ╚════██║██║     ██║   ██║   ██║
    // ███████║███████╗╚██████╔╝   ██║
    // ╚══════╝╚══════╝ ╚═════╝    ╚═╝
    /*
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

        SRALLOC_memset( allocator, 0, sizeof( srallocator_t ) );
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

    END OF SLOT
    */

    // ██████╗ ██████╗ ██╗ ██████╗ ██████╗ ██╗████████╗██╗   ██╗
    // ██╔══██╗██╔══██╗██║██╔═══██╗██╔══██╗██║╚══██╔══╝╚██╗ ██╔╝
    // ██████╔╝██████╔╝██║██║   ██║██████╔╝██║   ██║    ╚████╔╝
    // ██╔═══╝ ██╔══██╗██║██║   ██║██╔══██╗██║   ██║     ╚██╔╝
    // ██║     ██║  ██║██║╚██████╔╝██║  ██║██║   ██║      ██║
    // ╚═╝     ╚═╝  ╚═╝╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝   ╚═╝      ╚═╝

    // ███████╗███╗   ██╗██████╗          ██████╗ ███████╗   ██████╗  █████╗  ██████╗ ███████╗
    // ██╔════╝████╗  ██║██╔══██╗        ██╔═══██╗██╔════╝   ██╔══██╗██╔══██╗██╔════╝ ██╔════╝
    // █████╗  ██╔██╗ ██║██║  ██║        ██║   ██║█████╗     ██████╔╝███████║██║  ███╗█████╗
    // ██╔══╝  ██║╚██╗██║██║  ██║        ██║   ██║██╔══╝     ██╔═══╝ ██╔══██║██║   ██║██╔══╝
    // ███████╗██║ ╚████║██████╔╝███████╗╚██████╔╝██║███████╗██║     ██║  ██║╚██████╔╝███████╗
    // ╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝ ╚═════╝ ╚═╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚══════╝
    //

    // ███████╗███████╗██████╗  ██████╗
    // ╚══███╔╝██╔════╝██╔══██╗██╔═══██╗
    //   ███╔╝ █████╗  ██████╔╝██║   ██║
    //  ███╔╝  ██╔══╝  ██╔══██╗██║   ██║
    // ███████╗███████╗██║  ██║╚██████╔╝
    // ╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝
    //

#endif // SRALLOC_IMPLEMENTATION

#if defined( __cplusplus ) && !defined( SRALLOC_NO_CLASSES )
#define SRALLOC_DISALLOW_COPY_AND_ASSIGN( type ) \
    type( const type& ) = delete;                \
    void operator=( const type& ) = delete;

namespace sralloc {
class malloc_allocator {
  public:
    malloc_allocator() { _allocator = sralloc_create_malloc_allocator(); };
    ~malloc_allocator() { sralloc_destroy_malloc_allocator(); };
    void* allocate( srint_t size ) {}

  private:
    SRALLOC_DISALLOW_COPY_AND_ASSIGN( malloc_allocator );
    srallocator_t* _allocator;
}
} // namespace sralloc
#endif //__cplusplus && SRALLOC_NO_CLASSES

#ifndef SRALLOC_ENABLE_WARNINGS
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#endif // DEBUGINATOR_ENABLE_WARNINGS

#endif // INCLUDE_SRALLOC_H
