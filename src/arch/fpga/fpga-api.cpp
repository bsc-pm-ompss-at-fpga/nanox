/*************************************************************************************/
/*      Copyright 2010-2018 Barcelona Supercomputing Center                          */
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
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#include "nanos-fpga.h"
#include "fpgadd.hpp"
#include "fpgapinnedallocator.hpp"
#include "fpgaprocessor.hpp"
#include "simpleallocator.hpp"

using namespace nanos;

NANOS_API_DEF( void *, nanos_fpga_factory, ( void *args ) )
{
   nanos_fpga_args_t *fpga = ( nanos_fpga_args_t * ) args;
   // FIXME: acc_num have to be converted into an string
   return ( void * ) NEW ext::FPGADD( fpga->outline, fpga->acc_num );
}

NANOS_API_DEF( void *, nanos_fpga_alloc_dma_mem, ( size_t len ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "fpga_alloc_dma_mem" ); );
   fatal( "The API nanos_fpga_alloc_dma_mem is no longer supported" );

   ensure( nanos::ext::fpgaAllocator != NULL,
      "FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   nanos::ext::fpgaAllocator->lock();
   void * ret = nanos::ext::fpgaAllocator->allocate( len );
   nanos::ext::fpgaAllocator->unlock();
   return ret;
}

NANOS_API_DEF( void, nanos_fpga_free_dma_mem, ( void * buffer ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "fpga_free_dma_mem" ); );
   fatal( "The API nanos_fpga_free_dma_mem is no longer supported" );

   ensure( nanos::ext::fpgaAllocator != NULL,
      "FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   nanos::ext::fpgaAllocator->lock();
   nanos::ext::fpgaAllocator->free( buffer );
   nanos::ext::fpgaAllocator->unlock();
}

NANOS_API_DEF( nanos_err_t, nanos_find_fpga_pe, ( void *req, nanos_pe_t * pe ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "find_fpga_pe" ); );
   *pe = NULL;

   nanos_find_fpga_args_t * opts = ( nanos_find_fpga_args_t * )req;
   for (size_t i = 0; i < nanos::ext::fpgaPEs->size(); i++) {
      nanos::ext::FPGAProcessor * fpgaPE = nanos::ext::fpgaPEs->at(i);
      nanos::ext::FPGADevice * fpgaDev = ( nanos::ext::FPGADevice * )( fpgaPE->getActiveDevice() );
      if ( ( fpgaDev->getFPGAType() == opts->acc_num ) &&
           ( !opts->check_free || !fpgaPE->isExecLockAcquired() ) &&
           ( !opts->lock_pe || fpgaPE->tryAcquireExecLock() ) )
      {
         *pe = fpgaPE;
         break;
      }
   }
   return NANOS_OK;
}

NANOS_API_DEF( void *, nanos_fpga_get_phy_address, ( void * buffer ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_get_phy_address" ); );
   return buffer;
}

NANOS_API_DEF( nanos_err_t, nanos_fpga_set_task_arg, ( nanos_wd_t wd, size_t argIdx, bool isInput, bool isOutput, uint64_t argValue ))
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_set_task_arg" ); );

   nanos::ext::FPGAProcessor * fpgaPE = ( nanos::ext::FPGAProcessor * )myThread->runningOn();
   fpgaPE->setTaskArg( *( WD * )wd, argIdx, isInput, isOutput, argValue );

   return NANOS_OK;
}

NANOS_API_DEF( nanos_fpga_alloc_t, nanos_fpga_malloc, ( size_t len ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_malloc" ); );

   ensure( nanos::ext::fpgaAllocator != NULL,
      "FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   nanos::ext::fpgaAllocator->lock();
   nanos_fpga_alloc_t handle = ( uintptr_t )( nanos::ext::fpgaAllocator->allocate( len ) );
   nanos::ext::fpgaAllocator->unlock();
   return handle;
}

NANOS_API_DEF( void, nanos_fpga_free, ( nanos_fpga_alloc_t handle ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_free" ); );

   ensure( nanos::ext::fpgaAllocator != NULL,
      "FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   nanos::ext::fpgaAllocator->lock();
   nanos::ext::fpgaAllocator->free( ( void * )( handle ) );
   nanos::ext::fpgaAllocator->unlock();
}

NANOS_API_DEF( void, nanos_fpga_memcpy, ( nanos_fpga_alloc_t handle, size_t offset, void * ptr, size_t len,
   nanos_fpga_memcpy_kind_t kind ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_memcpy" ); );

   ensure( nanos::ext::fpgaAllocator != NULL,
      "FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   size_t realOffset = ( handle - nanos::ext::fpgaAllocator->getBaseAddress() ) + offset;
   if ( kind == NANOS_COPY_HOST_TO_FPGA ) {
      nanos::ext::fpgaCopyDataToFPGA( nanos::ext::fpgaAllocator->getBufferHandle(), realOffset, len, ptr );
   } else if ( kind == NANOS_COPY_FPGA_TO_HOST ) {
      nanos::ext::fpgaCopyDataFromFPGA( nanos::ext::fpgaAllocator->getBufferHandle(), realOffset, len, ptr );
   }
}
