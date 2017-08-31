#include "fpgapinnedallocator.hpp"
#include "debug.hpp"
#include "simpleallocator.hpp"
#include "lock.hpp"
#include "basethread.hpp" //For getMyThreadSafe in warning

using namespace nanos;
using namespace nanos::ext;

FPGAPinnedAllocator *nanos::ext::fpgaAllocator;

FPGAPinnedAllocator::FPGAPinnedAllocator( size_t size ) : _extraAllocLock(), _extraAllocMap()
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

uint64_t FPGAPinnedAllocator::getBaseAddressPhy() const
{
   unsigned long ret;
   xdma_status status = xdmaGetDMAAddress( _xdmaHandle, &ret );
   if ( status != XDMA_SUCCESS ) {
      ensure0( status == XDMA_SUCCESS, "Error getting the DMA address of the FPGAPinnedAllocator" );
   }
   return ret;
}

void * FPGAPinnedAllocator::allocateExtraMemory( size_t const size )
{
   xdma_status status;
   xdma_buf_handle handle;
   void * addr;

   status = xdmaAllocateKernelBuffer( &addr, &handle, size );
   if ( status != XDMA_SUCCESS ) {
      return NULL;
   }

   _extraAllocLock.acquire();
   _extraAllocMap[addr] = handle;
   _extraAllocLock.release();

   return addr;
}

void FPGAPinnedAllocator::freeExtraMemory( void * address )
{
   XdmaHandleMap::iterator it = _extraAllocMap.find( address );
   if ( it != _extraAllocMap.end() ) {
      xdmaFreeKernelBuffer( address, it->second );
      _extraAllocLock.acquire();
      _extraAllocMap.erase( it );
      _extraAllocLock.release();
   } else {
      warning( "Unregistered FPGA pinned memory region. Address: " << std::hex << address );
   }
}
