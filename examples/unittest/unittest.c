
#define SRALLOC_IMPLEMENTATION
#ifdef _MSC_VER
#pragma warning( disable : 4464 )
#endif
#include "../../sralloc.h"

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4777 4710 )

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

void
generic_allocator_tests( srallocator_t* allocator ) {
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );
    void* p1 = sralloc_alloc( allocator, 100 );
    void* p2 = sralloc_alloc( allocator, 1000 );
    lequal( allocator->stats.num_allocations, 2 );
    lequal( allocator->stats.amount_allocated, 1100 );
    sralloc_dealloc( allocator, p2 );
    lequal( allocator->stats.num_allocations, 1 );
    lequal( allocator->stats.amount_allocated, 100 );
    sralloc_dealloc( allocator, p1 );
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
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    srallocator_t* stackalloc  = sralloc_create_stack_allocator( "stack", mallocalloc, 2000 );
    generic_allocator_tests( stackalloc );
    sralloc_destroy_stack_allocator( stackalloc );
    lequal( mallocalloc->stats.num_allocations, 0 );
    lequal( mallocalloc->stats.amount_allocated, 0 );
    sralloc_destroy_malloc_allocator( mallocalloc );
}

int
main( void ) {

    lrun( "malloc_allocator", malloc_test );
    lrun( "stack_allocator", stack_test );

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
