#include "fpgapinnedallocator.hpp"
#include "debug.hpp"
#include "simpleallocator.hpp"

using namespace nanos;
using namespace nanos::ext;

FPGAPinnedAllocator::FPGAPinnedAllocator( const size_t size )
{
   void * addr;
   xdma_status status;
   status = xdmaAllocateKernelBuffer( &addr, &_chunk._handle, size );
   if ( status != XDMA_SUCCESS ) {
      warning0( "Could not allocate pinned memory" );
   }
   _chunk._allocator.init( (uint64_t)addr, size );
}

FPGAPinnedAllocator::~FPGAPinnedAllocator()
{
   xdmaFreeKernelBuffer( (void *)_chunk._allocator.getBaseAddress(), _chunk._handle );
}

void * FPGAPinnedAllocator::allocate( size_t size )
{
   return _chunk._allocator.allocate( size );
}

void FPGAPinnedAllocator::free( void * address )
{
   _chunk._allocator.free( address );
}

//size should not be needed to determine the base address of a pointer
void * FPGAPinnedAllocator::getBasePointer( void * address, size_t size )
{
   //return _chunk._allocator.getBasePointer( address, size );
   return (void *)_chunk._allocator.getBaseAddress();
}

xdma_buf_handle FPGAPinnedAllocator::getBufferHandle( void * address )
{
   return _chunk._handle;
}
