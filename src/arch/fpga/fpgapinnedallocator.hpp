#ifndef _NANOS_FPGA_PINNED_ALLOCATOR
#define _NANOS_FPGA_PINNED_ALLOCATOR

#include "simpleallocator_decl.hpp"

#include "libxtasks_wrapper.hpp"

namespace nanos {
namespace ext {


   class FPGAPinnedAllocator : public SimpleAllocator
   {
      private:
         xtasks_mem_handle   _handle;   //!< Memory chunk handler for xTasks library

      public:
         FPGAPinnedAllocator( size_t size );
         ~FPGAPinnedAllocator();

         /* \brief Returns the xTasks library handle for the memory region that is being managed
          */
         xtasks_mem_handle getBufferHandle();
   };

   //! \brief Pointer to the fpgaAllocator instance
   extern FPGAPinnedAllocator    *fpgaAllocator;

} // namespace ext
} // namespace nanos

#endif //_NANOS_FPGA_PINNED_ALLOCATOR
