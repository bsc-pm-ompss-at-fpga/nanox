#include "fpgapinnedallocator.hpp"
#include "debug.hpp"
#include "simpleallocator.hpp"
#include "system_decl.hpp" //For debug0 macro

using namespace nanos;
using namespace nanos::ext;

FPGAPinnedAllocator *nanos::ext::fpgaAllocator;

FPGAPinnedAllocator::FPGAPinnedAllocator( size_t size )
{
   xtasks_stat status;
   status = xtasksMalloc( size, &_handle );
   if ( status != XTASKS_SUCCESS ) {
      // Before fail, try to allocate less memory
      do {
         size = size/2;
         status = xtasksMalloc( size, &_handle );
      } while ( status != XTASKS_SUCCESS && size > 32768 /* 32KB */ );

      // Emit a warning with the allocation result
      if ( status == XTASKS_SUCCESS ) {
         warning0( "Could not allocate requested amount of FPGA device memory. Only " <<
                   size << " bytes have been allocated." );
      } else {
         warning0( "Could not allocate FPGA device memory for the FPGAPinnedAllocator" );
         size = 0;
      }
   }

   uint64_t addr = 0;
   if ( status == XTASKS_SUCCESS ) {
      status = xtasksGetAccAddress( _handle, &addr );
      ensure( status == XTASKS_SUCCESS, "Error getting the FPGA device address for the FPGAPinnedAllocator" );
   }
   init( addr, size );

   debug0( "New FPGAPinnedAllocator created with size: " << size/1024 << "KB, base_addr: 0x" <<
      std::hex << addr << std::dec );
}

FPGAPinnedAllocator::~FPGAPinnedAllocator()
{
   xtasksFree( _handle );
}

xtasks_mem_handle FPGAPinnedAllocator::getBufferHandle()
{
   return _handle;
}
