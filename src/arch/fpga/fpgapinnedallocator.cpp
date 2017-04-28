#include "fpgapinnedallocator.hpp"
#include "debug.hpp"
#include "simpleallocator.hpp"

using namespace nanos;
using namespace nanos::ext;

FPGAPinnedAllocator *nanos::ext::fpgaAllocator;

FPGAPinnedAllocator::FPGAPinnedAllocator( size_t size )
{
   void * addr;
   xdma_status status;
   status = xdmaAllocateKernelBuffer( &addr, &_xdmaHandle, size );
   if ( status != XDMA_SUCCESS ) {
      // Before fail, try to allocate less memory
      do {
         size = size/2;
         status = xdmaAllocateKernelBuffer( &addr, &_xdmaHandle, size );
      } while ( status != XDMA_SUCCESS && size > 32768 /* 32KB */ );

      // Emit a warning with the allocation result
      if ( status == XDMA_SUCCESS ) {
         warning0( "Could not allocate requested amount of XDMA pinned memory. Only " <<
                   size << " bytes have been allocated." );
      } else {
         warning0( "Could not allocate XDMA pinned memory for the FPGAPinnedAllocator" );
         addr = NULL;
         size = 0;
      }
   }
   init( ( uint64_t )( addr ), size );
}

FPGAPinnedAllocator::~FPGAPinnedAllocator()
{
   void * addr = ( void * )( getBaseAddress() );
   xdmaFreeKernelBuffer( addr, _xdmaHandle );
}

xdma_buf_handle FPGAPinnedAllocator::getBufferHandle()
{
   return _xdmaHandle;
}
