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

#include "nanos-fpga.h"
#include "fpgadd.hpp"
#include "fpgapinnedallocator.hpp"
#include "fpgaworker.hpp"
#include "fpgaprocessor.hpp"
#include "fpgaconfig.hpp"
#include "simpleallocator.hpp"

using namespace nanos;

NANOS_API_DEF( void *, nanos_fpga_factory, ( void *args ) )
{
   nanos_fpga_args_t *fpga = ( nanos_fpga_args_t * ) args;
   return ( void * ) NEW ext::FPGADD( fpga->outline, fpga->type );
}

NANOS_API_DEF( void *, nanos_fpga_alloc_dma_mem, ( size_t len ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "fpga_alloc_dma_mem" ); );
   fatal( "The API nanos_fpga_alloc_dma_mem is no longer supported" );

   ensure( nanos::ext::fpgaAllocator != NULL,
      " FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
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
      " FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
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
      if ( ( fpgaDev->getFPGAType() == opts->type ) &&
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

NANOS_API_DEF( void *, nanos_fpga_malloc, ( size_t len ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_malloc" ); );

   ensure( nanos::ext::fpgaAllocator != NULL,
      " FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   void * ptr = nanos::ext::fpgaAllocator->allocate( len );
   return ptr;
}

NANOS_API_DEF( void, nanos_fpga_free, ( void * fpgaPtr ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_free" ); );

   ensure( nanos::ext::fpgaAllocator != NULL,
      " FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   nanos::ext::fpgaAllocator->free( fpgaPtr );
}

NANOS_API_DEF( void, nanos_fpga_memcpy, ( void *fpgaPtr, void * hostPtr, size_t len,
   nanos_fpga_memcpy_kind_t kind ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_memcpy" ); );

   ensure( nanos::ext::fpgaAllocator != NULL,
      " FPGA allocator is not available. Try to force the FPGA support initialization with '--fpga-enable'" );
   size_t offset = ((uintptr_t)fpgaPtr) - nanos::ext::fpgaAllocator->getBaseAddress();
   if ( kind == NANOS_COPY_HOST_TO_FPGA ) {
      nanos::ext::fpgaCopyDataToFPGA( nanos::ext::fpgaAllocator->getBufferHandle(), offset, len, hostPtr );
   } else if ( kind == NANOS_COPY_FPGA_TO_HOST ) {
      nanos::ext::fpgaCopyDataFromFPGA( nanos::ext::fpgaAllocator->getBufferHandle(), offset, len, hostPtr );
   }
}

NANOS_API_DEF( void, nanos_fpga_create_wd_async, ( uint32_t archMask, uint64_t type,
   uint8_t numArgs, uint64_t * args, uint8_t numDeps, uint64_t * deps, uint8_t * depsFlags,
   uint8_t numCopies, nanos_fpga_copyinfo_t * copies ) )
{
   fatal( "The API nanos_fpga_create_wd_async can only be called from a FPGA device" );
}

NANOS_API_DEF( nanos_err_t, nanos_fpga_register_wd_info, ( uint64_t type, size_t num_devices,
  nanos_device_t * devices, nanos_translate_args_t translate ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_register_wd_info" ) );

   if ( nanos::ext::FPGAWorker::_registeredTasks->count(type) == 0 ) {
      verbose( "Registering WD info: " << type << " with " << num_devices << " devices." );

      std::string description = "fpga_created_task_" + toString( type );
      ( *nanos::ext::FPGAWorker::_registeredTasks )[type] =
         new nanos::ext::FPGAWorker::FPGARegisteredTask( num_devices, devices, translate, description );

      //Enable the creation callback
      if ( !nanos::ext::FPGAConfig::isIdleCreateCallbackRegistered() && !nanos::ext::FPGAConfig::forceDisableIdleCreateCallback() ) {
         sys.getEventDispatcher().addListenerAtIdle( *nanos::ext::FPGAWorker::_createWdListener );
         nanos::ext::FPGAConfig::setIdleCreateCallbackRegistered();
      }
      return NANOS_OK;
   }

   return NANOS_INVALID_REQUEST;
}
