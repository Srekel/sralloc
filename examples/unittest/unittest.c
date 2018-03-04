
#ifdef _WIN32
#pragma warning( push, 0 )
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma warning( pop )
#endif

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

#ifdef SRALLOC_DISABLE_STATS
#define lequal( ... )
#endif

sr_result_t
unittest_alloc( srallocator_t* allocator, int size ) {
    sr_result_t res = sralloc_alloc_with_size( allocator, size );
    lequal( res.size, size ); // Not necessarily true but for now
    memset( res.ptr, ( ( (sruintptr_t)res.ptr ) & 0xFF0 ) >> 8, res.size );
    return res;
}

void
unittest_dealloc( srallocator_t* allocator, sr_result_t res ) {
    for ( int i = 0; i < res.size; ++i ) {
        lequal( *( (char*)res.ptr + i ), (char)( ( ( (sruintptr_t)res.ptr ) & 0xFF0 ) >> 8 ) );
    }

    sralloc_dealloc( allocator, res.ptr );
}

void
generic_allocator_tests( srallocator_t* allocator ) {
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );

    // Single
    sr_result_t pA1 = unittest_alloc( allocator, 73 );
    lequal( allocator->stats.num_allocations, 1 );
    unittest_dealloc( allocator, pA1 );
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );

    // Multiple
    sr_result_t pB1 = unittest_alloc( allocator, 27 );
    sr_result_t pB2 = unittest_alloc( allocator, 57 );
    lequal( allocator->stats.num_allocations, 2 );
    unittest_dealloc( allocator, pB2 );
    lequal( allocator->stats.num_allocations, 1 );
    unittest_dealloc( allocator, pB1 );
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

    // Test returned size
    sr_result_t psD[10];
    for ( int i = 0; i < 10; i++ ) {
        psD[i] = sralloc_alloc_aligned_with_size( allocator, i * 7 + 100, 16 );
    }
    for ( int i = 0; i < 10; i++ ) {
        memset( psD[i].ptr, i + 150, psD[i].size );
    }
    lequal( allocator->stats.num_allocations, 10 );
    for ( int i = 0; i < 10; i++ ) {
        sralloc_dealloc( allocator, psD[i].ptr );
    }
    lequal( allocator->stats.num_allocations, 0 );
    lequal( allocator->stats.amount_allocated, 0 );

    // Test returned size 2
    sr_result_t psE[10];
    for ( int i = 0; i < 10; i++ ) {
        psE[i] = unittest_alloc( allocator, i * 7 + 100 );
    }
    lequal( allocator->stats.num_allocations, 10 );
    for ( int i = 0; i < 10; i++ ) {
        unittest_dealloc( allocator, psE[i] );
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
        srallocator_t* stackalloc  = sralloc_create_stack_allocator( "stack", mallocalloc, 20000 );
        generic_allocator_tests( stackalloc );
        sralloc_destroy_stack_allocator( stackalloc );
        lequal( mallocalloc->stats.num_allocations, 0 );
        lequal( mallocalloc->stats.amount_allocated, 0 );
        sralloc_destroy_malloc_allocator( mallocalloc );
    }
    {
        srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
        srallocator_t* stackalloc  = sralloc_create_stack_allocator( "stack", mallocalloc, 20000 );
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

void
end_of_page_test( void ) {
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    srallocator_t* eopalloc    = sralloc_create_end_of_page_allocator( "eop1", mallocalloc );
    generic_allocator_tests( eopalloc );
    sr_result_t pA1 = unittest_alloc( eopalloc, 100 );
    // for ( int i = 0; i < 1000; ++i ) {
    //     char c = pA1[i];
    //     pA1[i] = c + 1;
    // }
    unittest_dealloc( eopalloc, pA1 );
    sralloc_destroy_end_of_page_allocator( eopalloc );
    lequal( mallocalloc->stats.num_allocations, 0 );
    lequal( mallocalloc->stats.amount_allocated, 0 );
    sralloc_destroy_malloc_allocator( mallocalloc );
}

int
main( void ) {

    lrun( "malloc_allocator", malloc_test );
    lrun( "stack_allocator", stack_test );
    lrun( "proxy_allocator", proxy_test );
    lrun( "end_of_page_allocator", end_of_page_test );

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
