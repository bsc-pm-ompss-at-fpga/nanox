#ifndef _NANOS_FPGA_PINNED_ALLOCATOR
#define _NANOS_FPGA_PINNED_ALLOCATOR

#include <map>

#include "atomic.hpp"
#include "simpleallocator_decl.hpp"

#include "libxdma.h"

namespace nanos {
namespace ext {

   class FPGAPinnedAllocator
   {
      private:
         typedef struct {
            xdma_buf_handle      _handle;    //! Handler of the region for the xdmaLibrary
            SimpleAllocator      _allocator; //! Allocator that manages the memory region
         } FPGAMemoryRegion;

         /*!
          * NOTE: The idea here is to have a design that may allow further chunk allocations
          *       if an allocate fails in the current chunck.
          */
         FPGAMemoryRegion        _chunk;

      public:
         FPGAPinnedAllocator( size_t size );
         ~FPGAPinnedAllocator();

         void *allocate( size_t size );
         void free( void * address );
         void * getBasePointer( void *address, size_t size );
         xdma_buf_handle getBufferHandle( void *address );
   };
} // namespace ext
} // namespace nanos

#endif //_NANOS_PINNED_ALLOCATOR
