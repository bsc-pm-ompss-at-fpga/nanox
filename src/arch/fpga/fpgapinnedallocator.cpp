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

#include "fpgapinnedallocator.hpp"
#include "debug.hpp"
#include "simpleallocator.hpp"
#include "system_decl.hpp" //For debug0 macro

using namespace nanos;
using namespace nanos::ext;

FPGAPinnedAllocator *nanos::ext::fpgaAllocator;

FPGAPinnedAllocator::FPGAPinnedAllocator()
{
   const size_t userRequestedSize = FPGAConfig::getAllocatorPoolSize();
   size_t size = userRequestedSize > 0 ? userRequestedSize : FPGAConfig::getDefaultAllocatorPoolSize();
   xtasks_stat status;
   status = xtasksMalloc( size, &_handle );
   if ( status != XTASKS_SUCCESS ) {
      // Before fail, try to allocate less memory
      do {
         size = size/2;
         status = xtasksMalloc( size, &_handle );
      } while ( status != XTASKS_SUCCESS && size > 32768 /* 32KB */ );

      // Emit a warning with the allocation result
      if ( status == XTASKS_SUCCESS && userRequestedSize > 0 ) {
         warning0( "Could not allocate requested amount of FPGA device memory (" << userRequestedSize <<
                   " bytes). Only " << size << " bytes have been allocated." );
      } else if ( status != XTASKS_SUCCESS ) {
         warning0( "Could not allocate FPGA device memory for the FPGAPinnedAllocator" );
         size = 0;
      }
   }
   verbose0( "Allocated " << size << " bytes of FPGA device memory for the FPGAPinnedAllocator" );

   uint64_t addr = 0;
   if ( status == XTASKS_SUCCESS ) {
      status = xtasksGetAccAddress( _handle, &addr );
      ensure( status == XTASKS_SUCCESS, " Error getting the FPGA device address for the FPGAPinnedAllocator" );
   }
   init( addr, size );

   debug0( "New FPGAPinnedAllocator created with size: " << size/1024 << "KB, base_addr: 0x" <<
      std::hex << addr << std::dec );
}

FPGAPinnedAllocator::~FPGAPinnedAllocator()
{
   xtasksFree( _handle );
}

void * FPGAPinnedAllocator::allocate( size_t size ) {
   static const std::size_t align = FPGAConfig::getAllocAlign();

   lock();
   //Force the allocated sizes to be multiples of align
   //This prevents allocation of unaligned chunks
   void * ret = SimpleAllocator::allocate( ( size + align - 1 ) & ( ~( align - 1 ) ) );
   unlock();
   return ret;
}

size_t FPGAPinnedAllocator::free( void *address ) {
   size_t ret;
   lock();
   ret = SimpleAllocator::free( address );
   unlock();
   return ret;
}

xtasks_mem_handle FPGAPinnedAllocator::getBufferHandle()
{
   return _handle;
}

void nanos::ext::fpgaCopyDataToFPGA(xtasks_mem_handle handle, size_t offset, size_t len, void *ptr)
{
#if defined(NANOS_DEBUG_ENABLED)
   //Check that the copy is aligned
   static const std::size_t align = FPGAConfig::getAllocAlign();
   ensure( ( offset & ( align - 1 ) ) == 0, "Unaligned copy into FPGA memory not supported" );
#endif //defined(NANOS_DEBUG_ENABLED)

   xtasks_stat stat = xtasksMemcpy( handle, offset, len, ptr, XTASKS_HOST_TO_ACC );
   if ( stat != XTASKS_SUCCESS ) {
      //NOTE: Cannot put the ensure directly, as compiler will claim about unused stat var in performance
      ensure( false, " Error in xtasksMemcpy of FPGADevice::_copyIn" );
   }
}

void nanos::ext::fpgaCopyDataFromFPGA(xtasks_mem_handle handle, size_t offset, size_t len, void *ptr)
{
#if defined(NANOS_DEBUG_ENABLED)
   //Check that the copy is aligned
   static const std::size_t align = FPGAConfig::getAllocAlign();
   ensure( ( offset & ( align - 1 ) ) == 0, "Unaligned copy from FPGA memory not supported" );
#endif //defined(NANOS_DEBUG_ENABLED)

   xtasks_stat stat = xtasksMemcpy( handle, offset, len, ptr, XTASKS_ACC_TO_HOST );
   if ( stat != XTASKS_SUCCESS ) {
      //NOTE: Cannot put the ensure directly, as compiler will claim about unused stat var in performance
      ensure( false, " Error in xtasksMemcpy of FPGADevice::_copyOut" );
   }
}
