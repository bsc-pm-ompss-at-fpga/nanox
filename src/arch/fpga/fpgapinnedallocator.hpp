#ifndef _NANOS_FPGA_PINNED_ALLOCATOR
#define _NANOS_FPGA_PINNED_ALLOCATOR

#include "simpleallocator_decl.hpp"
#include "fpgaconfig.hpp"

#include "libxtasks_wrapper.hpp"

namespace nanos {
namespace ext {

   class FPGAPinnedAllocator : public SimpleAllocator
   {
      private:
         xtasks_mem_handle   _handle;   //!< Memory chunk handler for xTasks library

      public:
         FPGAPinnedAllocator();
         ~FPGAPinnedAllocator();

         void * allocate( size_t size );
         size_t free( void *address );


         /* \brief Returns the xTasks library handle for the memory region that is being managed
          */
         xtasks_mem_handle getBufferHandle();
   };

   //! \brief Pointer to the fpgaAllocator instance
   extern FPGAPinnedAllocator    *fpgaAllocator;

   /*! \brief Copies data from the user memory to the FPGA device memory
    *         fpgaAllocator[offset .. offset+len] = ptr[0 .. len]
    */
   void fpgaCopyDataToFPGA(xtasks_mem_handle handle, size_t offset, size_t len, void *ptr);

   /*! \brief Copies data from the FPGA device memory to the user memory
    *         ptr[offset .. offset+len] = fpgaAllocator[0 .. len]
    */
   void fpgaCopyDataFromFPGA(xtasks_mem_handle handle, size_t offset, size_t len, void *ptr);



} // namespace ext
} // namespace nanos

#endif //_NANOS_FPGA_PINNED_ALLOCATOR
