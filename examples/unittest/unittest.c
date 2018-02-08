//#define SRALLOC_STATIC
#define SRALLOC_IMPLEMENTATION
#pragma warning( suppress : 4464 )
#include "../../sralloc.h"

#pragma warning( push )
#pragma warning( disable : 4777 4710 )

#pragma warning( push )
#pragma warning( disable : 4464 4820 )
#include "../external/minctest/minctest.h"
#pragma warning( pop )

void
malloc_test( void ) {
    srallocator_t* rootalloc = sralloc_create_malloc_allocator( "root" );
	lequal(rootalloc->stats.num_allocations, 0);

    sralloc_destroy_malloc_allocator( rootalloc );
}

int
main( void ) {

    lrun( "malloc_allocator", malloc_test );

    lresults();
    return lfails != 0;
}


#pragma warning( pop )