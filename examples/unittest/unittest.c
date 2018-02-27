
#define SRALLOC_IMPLEMENTATION
// #define SRALLOC_DISABLE_NAMES
// #define SRALLOC_DISABLE_STATS
#ifdef _MSC_VER
#pragma warning( disable : 4464 4710 )
#endif
#include "../../sralloc.h"

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4777 4710 4005 )

#pragma warning( push )
#pragma warning( disable : 4464 4820 )
#endif

#include "../external/minctest/minctest.h"
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#ifdef _MSC_VER
#pragma warning( push, 0 )
#include <Windows.h>
#pragma warning( pop )
#endif

#ifdef SRALLOC_DISABLE_STATS
#define lequal( ... )
#endif

void
generic_allocator_tests( srallocator_t* allocator ) {
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );

    // Single
    void* pA1 = sralloc_alloc( allocator, 100 );
    lequal( allocator->stats.num_allocations, 1 );
    sralloc_dealloc( allocator, pA1 );
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );

    // Multiple
    void* pB1 = sralloc_alloc( allocator, 100 );
    void* pB2 = sralloc_alloc( allocator, 1000 );
    lequal( allocator->stats.num_allocations, 2 );
    sralloc_dealloc( allocator, pB2 );
    lequal( allocator->stats.num_allocations, 1 );
    sralloc_dealloc( allocator, pB1 );
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );

    // Aligned
    void* psC[10];
    for ( int i = 0; i < 10; i++ ) {
        psC[i] = sralloc_alloc_aligned( allocator, i * 7 + 100, 16 );
    }
    lequal( allocator->stats.num_allocations, 10 );
    for ( int i = 0; i < 10; i++ ) {
        sralloc_dealloc( allocator, psC[i] );
    }
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );
}

void
malloc_test( void ) {
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    generic_allocator_tests( mallocalloc );
    sralloc_destroy_malloc_allocator( mallocalloc );
}

void
stack_test( void ) {
    {
        srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
        srallocator_t* stackalloc  = sralloc_create_stack_allocator( "stack", mallocalloc, 2000 );
        generic_allocator_tests( stackalloc );
        sralloc_destroy_stack_allocator( stackalloc );
        lequal( mallocalloc->stats.num_allocations, 0 );
        lequal( mallocalloc->stats.amount_allocated, 0 );
        sralloc_destroy_malloc_allocator( mallocalloc );
    }
    {
        srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
        srallocator_t* stackalloc  = sralloc_create_stack_allocator( "stack", mallocalloc, 2000 );
        int*           pA1         = SRALLOC_OBJECT( stackalloc, int );
        *pA1                       = 111;
        sralloc_stack_allocator_push_state( stackalloc );
        void* pA2 = SRALLOC_BYTES( stackalloc, 100 );
        (void)pA2;
        sralloc_stack_allocator_pop_state( stackalloc );
        lequal( stackalloc->stats.num_allocations, 1 );
        int* pA3 = SRALLOC_OBJECT( stackalloc, int );
        *pA3     = 333;
        lequal( *pA1, 111 );
        *pA1 = 111;
        lequal( *pA3, 333 );
        SRALLOC_DEALLOC( stackalloc, pA3 );
        SRALLOC_DEALLOC( stackalloc, pA1 );
        lequal( stackalloc->stats.num_allocations, 0 );
        sralloc_destroy_stack_allocator( stackalloc );
        lequal( mallocalloc->stats.num_allocations, 0 );
        lequal( mallocalloc->stats.amount_allocated, 0 );
        sralloc_destroy_malloc_allocator( mallocalloc );
    }
}

void
proxy_test( void ) {
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    srallocator_t* proxyalloc1 = sralloc_create_proxy_allocator( "proxy1", mallocalloc );
    srallocator_t* proxyalloc2 = sralloc_create_proxy_allocator( "proxy2", mallocalloc );
    generic_allocator_tests( proxyalloc1 );
    generic_allocator_tests( proxyalloc2 );
    srallocator_t* proxyalloc3 = sralloc_create_proxy_allocator( "proxy3", proxyalloc2 );
    lequal( proxyalloc2->stats.num_allocations, 1 );
    generic_allocator_tests( proxyalloc3 );
    sralloc_destroy_proxy_allocator( proxyalloc3 );
    sralloc_destroy_proxy_allocator( proxyalloc1 );
    sralloc_destroy_proxy_allocator( proxyalloc2 );
    lequal( mallocalloc->stats.num_allocations, 0 );
    lequal( mallocalloc->stats.amount_allocated, 0 );
    sralloc_destroy_malloc_allocator( mallocalloc );
}

int
main( void ) {

    lrun( "malloc_allocator", malloc_test );
    lrun( "stack_allocator", stack_test );
    lrun( "proxy_allocator", proxy_test );

    lresults();

#ifdef _MSC_VER
    if ( IsDebuggerPresent() ) {
        system( "pause" );
    }
#endif

    return lfails != 0;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
