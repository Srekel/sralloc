
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
typedef char         srchar_t;
typedef int          srint_t;
typedef unsigned int sruint_t;
typedef short        srshort_t;
typedef uintptr_t    sruintptr_t;
#endif

typedef struct {
    void*   ptr;
    srint_t size;
    // srint_t offset;
} sr_result_t;

#if defined( SRALLOC_STATIC )
#define SRALLOC_API static
#else
#define SRALLOC_API extern
#endif

// General API
SRALLOC_API void* sralloc_alloc( srallocator_t* allocator, srint_t size );
SRALLOC_API sr_result_t sralloc_alloc_with_size( srallocator_t* allocator, srint_t size );
SRALLOC_API void* sralloc_alloc_aligned( srallocator_t* allocator, srint_t size, srint_t align );
SRALLOC_API sr_result_t sralloc_alloc_aligned_with_size( srallocator_t* allocator,
                                                         srint_t        size,
                                                         srint_t        align );
SRALLOC_API void        sralloc_dealloc( srallocator_t* allocator, void* ptr );
SRALLOC_API void*       sralloc_allocate( srallocator_t* allocator, srint_t size, srint_t align );

// Malloc allocator (global allocator)
SRALLOC_API srallocator_t* sralloc_create_malloc_allocator( const char* name );
SRALLOC_API void           sralloc_destroy_malloc_allocator( srallocator_t* allocator );

// Stack allocator (or stack frame allocator)
SRALLOC_API      srallocator_t*
                 sralloc_create_stack_allocator( const char* name, srallocator_t* parent, srint_t capacity );
SRALLOC_API void sralloc_destroy_stack_allocator( srallocator_t* allocator );

// Proxy allocator (for categorizing/structuring)
SRALLOC_API srallocator_t* sralloc_create_proxy_allocator( const char*    name,
                                                           srallocator_t* parent );
SRALLOC_API void           sralloc_destroy_proxy_allocator( srallocator_t* allocator );

// End-of-page allocator (for debugging write-past-eob)
SRALLOC_API srallocator_t* sralloc_create_end_of_page_allocator( const char*    name,
                                                                 srallocator_t* parent );
SRALLOC_API void           sralloc_destroy_end_of_page_allocator( srallocator_t* allocator );

#ifdef SRALLOC_ENABLE_IG_DEBUGHEAP
// Insomniac Games's debug heap allocator (for lots of things)
// See its github page. sralloc assumes that it's .h-file has been included
// prior to including sralloc.h for the implementation
// https://github.com/deplinenoise/ig-debugheap
SRALLOC_API      srallocator_t*
                 sralloc_create_ig_debugheap_allocator( const char* name, srallocator_t* parent, sruint_t capacity );
SRALLOC_API void sralloc_destroy_ig_debugheap_allocator( srallocator_t* allocator );
#endif

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
#define SRALLOC_OBJECT( allocator, type ) ( (type*)sralloc_alloc( allocator, sizeof( type ) ) )
#define SRALLOC_ARRAY( allocator, type, length ) \
    ( (type*)sralloc_alloc( allocator, sizeof( size ) * length ) );

#define SRALLOC_ALIGNED_BYTES( allocator, size, align ) \
    sralloc_alloc_aligned( allocator, size, align )
#define SRALLOC_ALIGNED_OBJECT( allocator, type ) \
    ( (type*)sralloc_alloc_aligned( allocator, sizeof( type ), SRALLOC_ALIGNOF( type ) ) )
#define SRALLOC_ALIGNED_ARRAY( allocator, type, length ) \
    ( (type*)sralloc_alloc_aligned( allocator, sizeof( type ) * length ), SRALLOC_ALIGNOF( type ) );

#define SRALLOC_DEALLOC( allocator, ptr ) sralloc_dealloc( allocator, ptr );

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

#ifdef SRALLOC_DO_WRITE_DEALLOCATION_PATTERN
#ifndef SRALLOC_DEALLOCATION_PATTERN
#define SRALLOC_DEALLOCATION_PATTERN 0xcd
#endif
#define SRALLOC_WRITE_DEALLOCATION_PATTERN( ptr, size ) \
    SRALLOC_memset( ptr, SRALLOC_DEALLOCATION_PATTERN, size )
#else
#define SRALLOC_WRITE_DEALLOCATION_PATTERN( ptr, size )
#endif

#ifndef SRALLOC_DISABLE_STATS
#define SRALLOC_USE_STATS
#endif

#ifndef SRALLOC_DISABLE_NAMES
#define SRALLOC_USE_NAMES
#endif

// #ifndef SRALLOC_DISABLE_TYPES
// #define SRALLOC_USE_TYPES
// #endif

// End of page allocator config
#ifndef SRALLOC_PAGE_SIZE
#define SRALLOC_PAGE_SIZE 0x1000
#endif

#ifndef SRALLOC_PROTECT_MEMORY
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
typedef DWORD srmemflag_t;
#define SRALLOC_MEMPROTECT_FLAG PAGE_NOACCESS
#define SRALLOC_PROTECT_MEMORY( ptr, size, protection, old_protection ) \
    VirtualProtect( ptr, size, protection, old_protection );
#elif defined( __APPLE__ ) || defined( __linux__ )
#include <sys/mman.h>
typedef int srmemflag_t;
#define SRALLOC_MEMPROTECT_FLAG PROT_NONE
#define SRALLOC_PROTECT_MEMORY( ptr, size, protection, old_protection ) \
    SRALLOC_UNUSED( old_protection );                                   \
    mprotect( ptr, size, protection );
#else
#define SRALLOC_PROTECT_MEMORY( ptr, size, protection, old_protection ) \
    SRALLOC_UNUSED( ptr, size, protection, old_protection );
#endif // _WIN32
#endif // SRALLOC_PROTECT_MEMORY

typedef struct {
    int num_allocations;
    int amount_allocated;
} sralloc_stats_t;

typedef sr_result_t ( *sralloc_allocate_func )( srallocator_t* allocator,
                                                srint_t        size,
                                                srint_t        align );
typedef void ( *sralloc_deallocate_func )( srallocator_t* allocator, void* ptr );

struct srallocator {
#ifdef SRALLOC_USE_NAMES
    const char* name; // TODO arrayify
#endif
    // #ifdef SRALLOC_USE_TYPES
    //     char type[16];
    // #endif
    sralloc_allocate_func   allocate_func;
    sralloc_deallocate_func deallocate_func;
#ifdef SRALLOC_USE_STATS
    srallocator_t*  parent;
    srallocator_t** children;
    srint_t         num_children;
    srint_t         children_capacity;
    sralloc_stats_t stats;
#endif
};

// ██╗███╗   ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
// ██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
// ██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
// ██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
// ██║██║ ╚████║   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
// ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝

static void
sr__add_child_allocator( srallocator_t* parent, srallocator_t* child ) {
    SRALLOC_UNUSED( parent, child );
#ifdef SRALLOC_USE_STATS
    child->parent = parent;
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
#endif // SRALLOC_USE_STATS
}

static void
sr__remove_child_allocator( srallocator_t* parent, srallocator_t* child ) {
    SRALLOC_UNUSED( parent, child );
#ifdef SRALLOC_USE_STATS
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

                parent->children          = SRALLOC_NULL;
                parent->children_capacity = 0;
            }

            return;
        }
    }

    SRALLOC_assert( 0 );
#endif // SRALLOC_USE_STATS
}

static void
sr__set_name( srallocator_t* allocator, const srchar_t* name ) {
    SRALLOC_UNUSED( allocator, name );
#ifdef SRALLOC_USE_NAMES
    allocator->name = name;
#endif
}

// static void
// sr__set_type( srallocator_t* allocator, const srchar_t* name ) {
//     SRALLOC_UNUSED( allocator, name );
// #ifdef SRALLOC_USE_NAMES
//     SRALLOC_memcpy( allocator->name, SRALLOC_strlen( name ), name );
// #endif
// }

static void*
sr__ptr_to_aligned_ptr( void* ptr, srint_t align ) {
    if ( align == 0 ) {
        return ptr;
    }

    sruintptr_t offset      = ( ~(sruintptr_t)ptr + 1 ) & ( align - 1 );
    void*       aligned_ptr = (srchar_t*)ptr + offset;
    return aligned_ptr;
}

static srchar_t*
sr__aligned_ptr_after_preamble( void* ptr, srint_t preamble_size, srint_t align ) {
    srchar_t* after_preamble = (srchar_t*)ptr + preamble_size;
    return sr__ptr_to_aligned_ptr( after_preamble, align );
}

static srint_t
sr__ptr_diff( void* ptr1, void* ptr2 ) {
    return ( srint_t )( (srchar_t*)ptr1 - (srchar_t*)ptr2 );
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

SRALLOC_API sr_result_t
            sralloc_alloc_with_size( srallocator_t* allocator, srint_t size ) {
    if ( size == 0 ) {
        sr_result_t res = { SRALLOC_ZERO_SIZE_PTR, 0 };
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

SRALLOC_API sr_result_t
            sralloc_alloc_aligned_with_size( srallocator_t* allocator, srint_t size, srint_t align ) {
    if ( size == 0 ) {
        sr_result_t res = { SRALLOC_ZERO_SIZE_PTR, 0 };
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

typedef struct {
    srint_t offset;
    srint_t size;
} sralloc_malloc_preamble_t;

static sr_result_t
sralloc_malloc_allocate( srallocator_t* allocator, srint_t wanted_size, srint_t align ) {
    SRALLOC_UNUSED( allocator );
    srint_t preamble_size = sizeof( sralloc_malloc_preamble_t );
    srint_t size          = wanted_size;
    size += align;
    size += preamble_size;

#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif

    srchar_t* unaligned_ptr = SRALLOC_malloc( size );
    if ( unaligned_ptr == SRALLOC_NULL ) {
        sr_result_t res = { SRALLOC_NULL, 0 };
        return res;
    }

    srchar_t* ptr = sr__aligned_ptr_after_preamble( unaligned_ptr, preamble_size, align );
    sralloc_malloc_preamble_t* preamble = (sralloc_malloc_preamble_t*)ptr - 1;
    preamble->size                      = size;
    preamble->offset                    = sr__ptr_diff( preamble, unaligned_ptr );

    sr_result_t res;
    res.ptr  = (void*)ptr;
    res.size = wanted_size;
    return res;
}

static void
sralloc_malloc_deallocate( srallocator_t* allocator, void* ptr ) {
    SRALLOC_UNUSED( allocator );
    sralloc_malloc_preamble_t* preamble      = (sralloc_malloc_preamble_t*)ptr - 1;
    srchar_t*                  unaligned_ptr = (srchar_t*)preamble - preamble->offset;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated -= preamble->size;
    allocator->stats.num_allocations--;
#endif
    SRALLOC_free( unaligned_ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_malloc_allocator( const char* name ) {
    srallocator_t* allocator = (srallocator_t*)SRALLOC_malloc( sizeof( srallocator_t ) );
    SRALLOC_memset( allocator, 0, sizeof( srallocator_t ) );
    sr__set_name( allocator, name );
    // sr__set_type( allocator, "malloc" );
    allocator->allocate_func   = sralloc_malloc_allocate;
    allocator->deallocate_func = sralloc_malloc_deallocate;
    return allocator;
}

SRALLOC_API void
sralloc_destroy_malloc_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_USE_STATS
    SRALLOC_assert( allocator->num_children == 0 );
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
    srint_t offset;
    srint_t size;
} sralloc_stack_preamble_t;

typedef struct {
    void* top;
#ifdef SRALLOC_USE_STATS
    sralloc_stats_t stats;
#endif
} srallocator_stack_state_t;

typedef struct {
    void*                     top;
    void*                     end;
    srallocator_t*            backing_allocator;
    srallocator_stack_state_t states[4];
    srint_t                   num_states;
} srallocator_stack_t;

static sr_result_t
sralloc_stack_allocate( srallocator_t* allocator, srint_t wanted_size, srint_t align ) {
    srint_t preamble_size = sizeof( sralloc_stack_preamble_t );
    srint_t size          = wanted_size;
    size += align;
    size += preamble_size;

    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    if ( size > sr__ptr_diff( stack_allocator->end, stack_allocator->top ) ) {
        sr_result_t res = { SRALLOC_NULL, 0 };
        return res;
    }

#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif

    void*     unaligned_ptr = stack_allocator->top;
    srchar_t* ptr           = sr__aligned_ptr_after_preamble( unaligned_ptr, preamble_size, align );
    sralloc_malloc_preamble_t* preamble = (sralloc_malloc_preamble_t*)ptr - 1;
    preamble->size                      = size;
    preamble->offset                    = sr__ptr_diff( preamble, unaligned_ptr );
    stack_allocator->top                = ptr + size;
    sr_result_t res;
    res.ptr  = (void*)( ptr );
    res.size = wanted_size;
    return res;
}

static void
sralloc_stack_deallocate( srallocator_t* allocator, void* ptr ) {
    srallocator_stack_t*       stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    sralloc_malloc_preamble_t* preamble        = (sralloc_malloc_preamble_t*)ptr - 1;
    srchar_t*                  unaligned_ptr   = (srchar_t*)preamble - preamble->offset;
    stack_allocator->top                       = unaligned_ptr;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated -= preamble->size;
    allocator->stats.num_allocations--;
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

    SRALLOC_memset( allocator, 0, allocator_size );
    sr__add_child_allocator( parent, allocator );
    sr__set_name( allocator, name );
    allocator->allocate_func           = sralloc_stack_allocate;
    allocator->deallocate_func         = sralloc_stack_deallocate;
    stack_allocator->top               = stack_allocator + 1;
    stack_allocator->end               = ( (char*)stack_allocator->top ) + capacity;
    stack_allocator->num_states        = 0;
    stack_allocator->backing_allocator = parent;
    return allocator;
}

SRALLOC_API void
sralloc_destroy_stack_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_USE_STATS
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
    sr__remove_child_allocator( allocator->parent, allocator );
    SRALLOC_assert( allocator->num_children == 0 );
#endif

    srallocator_stack_t* stack_allocator = (srallocator_stack_t*)( allocator + 1 );
    SRALLOC_UNUSED( stack_allocator );
    SRALLOC_assert( stack_allocator->num_states == 0 );
    sralloc_dealloc( stack_allocator->backing_allocator, allocator );
}

// ██████╗ ██████╗  ██████╗ ██╗  ██╗██╗   ██╗
// ██╔══██╗██╔══██╗██╔═══██╗╚██╗██╔╝╚██╗ ██╔╝
// ██████╔╝██████╔╝██║   ██║ ╚███╔╝  ╚████╔╝
// ██╔═══╝ ██╔══██╗██║   ██║ ██╔██╗   ╚██╔╝
// ██║     ██║  ██║╚██████╔╝██╔╝ ██╗   ██║
// ╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝

typedef struct {
    srallocator_t* backing_allocator;
} srallocator_proxy_t;

typedef struct {
    srint_t size;
    srint_t offset;
} sralloc_proxy_preamble_t;

static sr_result_t
sralloc_proxy_allocate( srallocator_t* allocator, srint_t wanted_size, srint_t align ) {
    SRALLOC_UNUSED( allocator );
    srint_t preamble_size = sizeof( sralloc_proxy_preamble_t );
    srint_t size          = wanted_size;
    size += align;
    size += preamble_size;

#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += size;
    allocator->stats.num_allocations++;
#endif

    srallocator_proxy_t* proxy_allocator = (srallocator_proxy_t*)( allocator + 1 );
    srchar_t*            unaligned_ptr = SRALLOC_BYTES( proxy_allocator->backing_allocator, size );
    if ( unaligned_ptr == SRALLOC_NULL ) {
        sr_result_t res = { SRALLOC_NULL, 0 };
        return res;
    }

    srchar_t* ptr = sr__aligned_ptr_after_preamble( unaligned_ptr, preamble_size, align );
    sralloc_proxy_preamble_t* preamble = (sralloc_proxy_preamble_t*)ptr - 1;
    preamble->size                     = size;
    preamble->offset                   = sr__ptr_diff( preamble, unaligned_ptr );

    sr_result_t res;
    res.ptr  = (void*)ptr;
    res.size = wanted_size;
    return res;
}

static void
sralloc_proxy_deallocate( srallocator_t* allocator, void* ptr ) {
    SRALLOC_UNUSED( allocator );
    sralloc_proxy_preamble_t* preamble      = (sralloc_proxy_preamble_t*)ptr - 1;
    srchar_t*                 unaligned_ptr = (srchar_t*)preamble - preamble->offset;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated -= preamble->size;
    allocator->stats.num_allocations--;
#endif
    srallocator_proxy_t* proxy_allocator = (srallocator_proxy_t*)( allocator + 1 );
    SRALLOC_DEALLOC( proxy_allocator->backing_allocator, unaligned_ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_proxy_allocator( const char* name, srallocator_t* parent ) {
#ifdef SRALLOC_DISABLE_PROXY
    return parent;
#endif

    srint_t              allocator_size  = sizeof( srallocator_t ) + sizeof( srallocator_proxy_t );
    void*                memory          = sralloc_alloc( parent, allocator_size );
    srallocator_t*       allocator       = (srallocator_t*)memory;
    srallocator_proxy_t* proxy_allocator = (srallocator_proxy_t*)( allocator + 1 );

    SRALLOC_memset( allocator, 0, allocator_size );
    sr__add_child_allocator( parent, allocator );
    sr__set_name( allocator, name );
    allocator->allocate_func           = sralloc_proxy_allocate;
    allocator->deallocate_func         = sralloc_proxy_deallocate;
    proxy_allocator->backing_allocator = parent;

    return allocator;
}

SRALLOC_API void
sralloc_destroy_proxy_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_DISABLE_PROXY
    return;
#endif
#ifdef SRALLOC_USE_STATS
    sr__remove_child_allocator( allocator->parent, allocator );
    SRALLOC_assert( allocator->num_children == 0 );
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    srallocator_proxy_t* proxy_allocator = (srallocator_proxy_t*)( allocator + 1 );
    SRALLOC_DEALLOC( proxy_allocator->backing_allocator, allocator );
}

// ███████╗███╗   ██╗██████╗          ██████╗ ███████╗   ██████╗  █████╗  ██████╗ ███████╗
// ██╔════╝████╗  ██║██╔══██╗        ██╔═══██╗██╔════╝   ██╔══██╗██╔══██╗██╔════╝ ██╔════╝
// █████╗  ██╔██╗ ██║██║  ██║        ██║   ██║█████╗     ██████╔╝███████║██║  ███╗█████╗
// ██╔══╝  ██║╚██╗██║██║  ██║        ██║   ██║██╔══╝     ██╔═══╝ ██╔══██║██║   ██║██╔══╝
// ███████╗██║ ╚████║██████╔╝███████╗╚██████╔╝██║███████╗██║     ██║  ██║╚██████╔╝███████╗
// ╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝ ╚═════╝ ╚═╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚══════╝

typedef struct {
    srallocator_t* backing_allocator;
} srallocator_end_of_page_t;

typedef struct {
    srint_t     size;
    srint_t     offset;
    srmemflag_t initial_protection;
    srchar_t*   page_ptr;
} sralloc_end_of_page_preamble_t;

static sr_result_t
sralloc_end_of_page_allocate( srallocator_t* allocator, srint_t wanted_size, srint_t align ) {
    SRALLOC_UNUSED( allocator );
    srint_t preamble_size    = sizeof( sralloc_end_of_page_preamble_t );
    srint_t size_to_allocate = wanted_size;
    size_to_allocate += align;
    size_to_allocate += preamble_size;
    size_to_allocate += SRALLOC_PAGE_SIZE * 2;

#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += size_to_allocate;
    allocator->stats.num_allocations++;
#endif

    srallocator_end_of_page_t* end_of_page_allocator =
      (srallocator_end_of_page_t*)( allocator + 1 );
    srchar_t* unaligned_ptr =
      SRALLOC_BYTES( end_of_page_allocator->backing_allocator, size_to_allocate );
    if ( unaligned_ptr == SRALLOC_NULL ) {
        sr_result_t res = { SRALLOC_NULL, 0 };
        return res;
    }

    srchar_t* page_ptr = sr__ptr_to_aligned_ptr(
      unaligned_ptr + wanted_size + align + preamble_size, SRALLOC_PAGE_SIZE );

    srchar_t*                       ptr      = page_ptr - wanted_size - align;
    sralloc_end_of_page_preamble_t* preamble = (sralloc_end_of_page_preamble_t*)ptr - 1;
    preamble->size                           = size_to_allocate;
    preamble->offset                         = sr__ptr_diff( preamble, unaligned_ptr );
    preamble->page_ptr                       = page_ptr;

    // SRALLOC_assert( sr__ptr_to_aligned_ptr( ptr, align ) == ptr );
    SRALLOC_PROTECT_MEMORY(
      page_ptr, SRALLOC_PAGE_SIZE, SRALLOC_MEMPROTECT_FLAG, &preamble->initial_protection );

    sr_result_t res;
    res.ptr  = (void*)ptr;
    res.size = wanted_size;
    return res;
}

static void
sralloc_end_of_page_deallocate( srallocator_t* allocator, void* ptr ) {
    SRALLOC_UNUSED( allocator );
    sralloc_end_of_page_preamble_t* preamble      = (sralloc_end_of_page_preamble_t*)ptr - 1;
    srchar_t*                       unaligned_ptr = (srchar_t*)preamble - preamble->offset;
#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated -= preamble->size;
    allocator->stats.num_allocations--;
#endif
    srchar_t*   page_ptr = preamble->page_ptr;
    srmemflag_t current_protection;
    SRALLOC_PROTECT_MEMORY(
      page_ptr, SRALLOC_PAGE_SIZE, preamble->initial_protection, &current_protection );

    srallocator_end_of_page_t* end_of_page_allocator =
      (srallocator_end_of_page_t*)( allocator + 1 );
    SRALLOC_DEALLOC( end_of_page_allocator->backing_allocator, unaligned_ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_end_of_page_allocator( const char* name, srallocator_t* parent ) {
#ifdef SRALLOC_DISABLE_end_of_page
    return parent;
#endif

    srint_t        allocator_size = sizeof( srallocator_t ) + sizeof( srallocator_end_of_page_t );
    void*          memory         = sralloc_alloc( parent, allocator_size );
    srallocator_t* allocator      = (srallocator_t*)memory;
    srallocator_end_of_page_t* end_of_page_allocator =
      (srallocator_end_of_page_t*)( allocator + 1 );

    SRALLOC_memset( allocator, 0, allocator_size );
    sr__add_child_allocator( parent, allocator );
    sr__set_name( allocator, name );
    allocator->allocate_func                 = sralloc_end_of_page_allocate;
    allocator->deallocate_func               = sralloc_end_of_page_deallocate;
    end_of_page_allocator->backing_allocator = parent;

    return allocator;
}

SRALLOC_API void
sralloc_destroy_end_of_page_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_USE_STATS
    sr__remove_child_allocator( allocator->parent, allocator );
    SRALLOC_assert( allocator->num_children == 0 );
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    srallocator_end_of_page_t* end_of_page_allocator =
      (srallocator_end_of_page_t*)( allocator + 1 );
    SRALLOC_DEALLOC( end_of_page_allocator->backing_allocator, allocator );
}

    // ██╗ ██████╗         ██████╗ ███████╗██████╗ ██╗   ██╗ ██████╗ ██╗  ██╗███████╗ █████╗ ██████╗
    // ██║██╔════╝         ██╔══██╗██╔════╝██╔══██╗██║   ██║██╔════╝ ██║ ██║██╔════╝██╔══██╗██╔══██╗
    // ██║██║  ███╗        ██║  ██║█████╗  ██████╔╝██║   ██║██║  ███╗███████║█████╗ ███████║██████╔╝
    // ██║██║   ██║        ██║  ██║██╔══╝  ██╔══██╗██║   ██║██║   ██║██╔══██║██╔══╝  ██╔══██║██╔═══╝
    // ██║╚██████╔╝███████╗██████╔╝███████╗██████╔╝╚██████╔╝╚██████╔╝██║  ██║███████╗██║  ██║██║
    // ╚═╝ ╚═════╝ ╚══════╝╚═════╝ ╚══════╝╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝

#ifdef SRALLOC_ENABLE_IG_DEBUGHEAP

typedef struct {
    DebugHeap* debugheap;
} srallocator_ig_debugheap_t;

typedef struct {
    srint_t     size;
    srint_t     offset;
    srmemflag_t initial_protection;
    srchar_t*   page_ptr;
} sralloc_ig_debugheap_preamble_t;

static sr_result_t
sralloc_ig_debugheap_allocate( srallocator_t* allocator, srint_t wanted_size, srint_t align ) {
    srallocator_ig_debugheap_t* ig_debugheap_allocator =
      (srallocator_ig_debugheap_t*)( allocator + 1 );

    void* ptr =
      DebugHeapAllocate( ig_debugheap_allocator->debugheap, (size_t)wanted_size, (size_t)align );

    srint_t actual_size = (srint_t)DebugHeapGetAllocSize( ig_debugheap_allocator->debugheap, ptr );

#ifdef SRALLOC_USE_STATS
    allocator->stats.amount_allocated += actual_size;
    allocator->stats.num_allocations += 1;
#endif

    sr_result_t res;
    res.ptr  = (void*)ptr;
    res.size = actual_size;
    return res;
}

static void
sralloc_ig_debugheap_deallocate( srallocator_t* allocator, void* ptr ) {
    srallocator_ig_debugheap_t* ig_debugheap_allocator =
      (srallocator_ig_debugheap_t*)( allocator + 1 );

#ifdef SRALLOC_USE_STATS
    srint_t actual_size = (srint_t)DebugHeapGetAllocSize( ig_debugheap_allocator->debugheap, ptr );
    allocator->stats.amount_allocated -= actual_size;
    allocator->stats.num_allocations -= 1;
#endif

    DebugHeapFree( ig_debugheap_allocator->debugheap, ptr );
}

SRALLOC_API srallocator_t*
            sralloc_create_ig_debugheap_allocator( const char*    name,
                                                   srallocator_t* parent,
                                                   sruint_t       capacity ) {
    DebugHeap* debugheap = DebugHeapInit( (size_t)capacity );

    srint_t        allocator_size = sizeof( srallocator_t ) + sizeof( srallocator_ig_debugheap_t );
    void*          memory    = DebugHeapAllocate( debugheap, allocator_size, 64u );
    srallocator_t* allocator = (srallocator_t*)memory;
    srallocator_ig_debugheap_t* ig_debugheap_allocator =
      (srallocator_ig_debugheap_t*)( allocator + 1 );

    SRALLOC_memset( allocator, 0, allocator_size );
    sr__add_child_allocator( parent, allocator );
    sr__set_name( allocator, name );
    allocator->allocate_func          = sralloc_ig_debugheap_allocate;
    allocator->deallocate_func        = sralloc_ig_debugheap_deallocate;
    ig_debugheap_allocator->debugheap = debugheap;

    return allocator;
}

SRALLOC_API void
sralloc_destroy_ig_debugheap_allocator( srallocator_t* allocator ) {
#ifdef SRALLOC_USE_STATS
    sr__remove_child_allocator( allocator->parent, allocator );
    SRALLOC_assert( allocator->num_children == 0 );
    SRALLOC_assert( allocator->stats.num_allocations == 0 );
    SRALLOC_assert( allocator->stats.amount_allocated == 0 );
#endif
    srallocator_ig_debugheap_t* ig_debugheap_allocator =
      (srallocator_ig_debugheap_t*)( allocator + 1 );

    DebugHeap* debugheap = ig_debugheap_allocator->debugheap;
    DebugHeapFree( ig_debugheap_allocator->debugheap, allocator );
    DebugHeapDestroy( debugheap );
}

#endif //  SRALLOC_ENABLE_IG_DEBUGHEAP

/*

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
}
END OF SLOT
*/
#endif // SRALLOC_IMPLEMENTATION

#if defined( __cplusplus ) && !defined( SRALLOC_NO_CLASSES )

namespace sralloc {
class Allocator {
  public:
    // clang-format off
    static Allocator create_malloc_allocator( const char* name )
                    { return Allocator( sralloc_create_malloc_allocator( name ) ); }
    static Allocator create_stack_allocator( const char* name, srallocator_t* parent, srint_t capacity )
                    { return Allocator( sralloc_create_stack_allocator( name, parent, capacity ) ); }

    void*       allocate( srint_t size )
                    { return sralloc_alloc( _allocator, size ); }

    sr_result_t allocate_with_size( srint_t size )
                    { sralloc_alloc_with_size( _allocator, size ); }

    void*       allocate_aligned( srint_t size, srint_t align )
                    { sralloc_alloc_aligned( _allocator, size ); }

    sr_result_t allocate_aligned_with_size( srint_t size, srint_t align )
                    { sralloc_alloc_aligned_with_size( _allocator, size ); }

    template <typename T>
    T*          allocate()
                    { return static_cast<T*>( allocate( sizeof( T ) ) ); }

    template <typename T>
    T*          allocate_aligned( srint_t align )
                    { return static_cast<T*>( allocate_aligned( sizeof( T ), align ) ); }

    void        deallocate( void* ptr )
                    { sralloc_dealloc( _allocator, ptr ); }
    // clang-format on

    ~Allocator() {
        switch ( _type ) {
        case AllocatorMalloc:
            sralloc_destroy_malloc_allocator( _allocator );
            break;
        case AllocatorStack:
            sralloc_destroy_stack_allocator( _allocator );
            break;
        }
    }

  private:
    Allocator( srallocator_t* allocator )
    : _allocator( allocator ) {}

    Allocator( const Allocator& ) = delete;
    void operator=( const Allocator& ) = delete;

    enum AllocatorType {
        AllocatorInvalid = 0,
        AllocatorMalloc,
        AllocatorStack,
    };

    srallocator_t* _allocator;
    AllocatorType  _type;
};

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
