#ifndef _NANOS_FPGA_PINNED_ALLOCATOR
#define _NANOS_FPGA_PINNED_ALLOCATOR

#include "simpleallocator_decl.hpp"

#include "libxdma_wrapper.hpp"

namespace nanos {
namespace ext {


   class FPGAPinnedAllocator : public SimpleAllocator
   {
      private:
         xdma_buf_handle   _xdmaHandle;   //!< Memory chunk handler for xdma library

      public:
         FPGAPinnedAllocator( size_t size );
         ~FPGAPinnedAllocator();

         /* \brief Returns the XDMA library buffer handle for the memory region that is being managed
          */
         xdma_buf_handle getBufferHandle();

         /* \brief Returns the physical address of the memory region that is being managed
          */
         uint64_t getBaseAddressPhy() const;
   };

   //! \brief Pointer to the fpgaAllocator instance
   extern FPGAPinnedAllocator    *fpgaAllocator;

} // namespace ext
} // namespace nanos

#endif //_NANOS_PINNED_ALLOCATOR
