/*************************************************************************************/
/*      Copyright 2009-2018 Barcelona Supercomputing Center                          */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            */
/*************************************************************************************/

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
