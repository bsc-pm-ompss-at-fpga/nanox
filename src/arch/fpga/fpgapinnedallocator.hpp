#ifndef _NANOS_FPGA_PINNED_ALLOCATOR
#define _NANOS_FPGA_PINNED_ALLOCATOR

#include "simpleallocator_decl.hpp"
#include "lock_decl.hpp"

#include "libxdma.h"
#include <map>

namespace nanos {
namespace ext {


   class FPGAPinnedAllocator : public SimpleAllocator
   {
      private:
         typedef std::map< void *, xdma_buf_handle > XdmaHandleMap;
         xdma_buf_handle   _xdmaHandle;      //!< XDMA buffer handle for the allocator memory chunk
         Lock              _extraAllocLock;  //!< Lock to protect _extraAllocMap edition
         XdmaHandleMap     _extraAllocMap;   //!< Map with all xdma buffer handles additionally allocated

      public:
         FPGAPinnedAllocator( size_t size );
         ~FPGAPinnedAllocator();

         /*! \brief Returns the XDMA buffer handle for the allocator memory chunk
          */
         xdma_buf_handle getBufferHandle();

         /*! \brief Returns the physical address of the allocator memory chunk
          */
         uint64_t getBaseAddressPhy() const;

         /*! \brief Allocate size bytes of pinned memory directly from the XDMA library
          *         instead of using the pre-allocated memory chunk
          */
         void * allocateExtraMemory( size_t const size );

         /*! \brief Free the memory region pointed by address which was allocated using
          *         allocateExtraMemory method
          */
         void freeExtraMemory( void * address );
   };

   //! \brief Pointer to the fpgaAllocator instance
   extern FPGAPinnedAllocator    *fpgaAllocator;

} // namespace ext
} // namespace nanos

#endif //_NANOS_PINNED_ALLOCATOR
