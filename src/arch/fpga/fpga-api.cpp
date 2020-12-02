/*************************************************************************************/
/*      Copyright 2009-2019 Barcelona Supercomputing Center                          */
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
#include "fpgawd_decl.hpp"
#include "fpgaprocessorinfo.hpp"
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

NANOS_API_DEF( nanos_err_t, nanos_fpga_create_task, ( nanos_fpga_task_t *utask, nanos_wd_t uwd ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_create_task" ); );

   WD * wd = ( WD * )( uwd );
   const nanos::ext::FPGAWD * fpgaWd = dynamic_cast<const ext::FPGAWD *>( wd );
   const nanos::ext::FPGAProcessorInfo & fpgaInfo =
      ( ( nanos::ext::FPGAProcessor * )( myThread->runningOn() ) )->getFPGAProcessorInfo();
   xtasks_task_id parentTask = 0;
   xtasks_task_handle task;
   xtasks_stat status;

   if ( fpgaWd != NULL ) {
      //NOTE: This wd is an FPGA spawned task
      parentTask = fpgaWd->getHwRuntimeParentId();
   }

   status = xtasksCreateTask( ( uintptr_t )( wd ), fpgaInfo.getHandle(), parentTask,
      XTASKS_COMPUTE_ENABLE, &task );
   if ( status != XTASKS_SUCCESS ) {
      //TODO: If status == XTASKS_ENOMEM, block and wait untill mem is available
      fatal( "Cannot initialize FPGA task info (accId: " <<
             fpgaInfo.getId() << "): " <<
             ( status == XTASKS_ENOMEM ? "XTASKS_ENOMEM" : "XTASKS_ERROR" ) );
   }

   nanos::ext::FPGADD &dd = ( nanos::ext::FPGADD & )( wd->getActiveDevice() );
   dd.setHandle( task );
   *utask = ( nanos_fpga_task_t )( task );

   return NANOS_OK;
}

NANOS_API_DEF( nanos_err_t, nanos_fpga_create_periodic_task, ( nanos_fpga_task_t *utask, nanos_wd_t uwd,
   const unsigned int period, const unsigned int num_reps ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_create_periodic_task" ); );

   WD * wd = ( WD * )( uwd );
   const nanos::ext::FPGAWD * fpgaWd = dynamic_cast<const ext::FPGAWD *>( wd );
   const nanos::ext::FPGAProcessorInfo & fpgaInfo =
      ( ( nanos::ext::FPGAProcessor * )( myThread->runningOn() ) )->getFPGAProcessorInfo();
   xtasks_task_id parentTask = 0;
   xtasks_task_handle task;
   xtasks_stat status;

   if ( fpgaWd != NULL ) {
      //NOTE: This wd is an FPGA spawned task
      parentTask = fpgaWd->getHwRuntimeParentId();
   }

   status = xtasksCreatePeriodicTask( ( uintptr_t )( wd ), fpgaInfo.getHandle(), parentTask,
      XTASKS_COMPUTE_ENABLE, num_reps, period, &task );
   if ( status != XTASKS_SUCCESS ) {
      //TODO: If status == XTASKS_ENOMEM, block and wait untill mem is available
      fatal( "Cannot initialize FPGA periodic task info (accId: " <<
             fpgaInfo.getId() << "): " <<
             ( status == XTASKS_ENOMEM ? "XTASKS_ENOMEM" : "XTASKS_ERROR" ) );
   }

   nanos::ext::FPGADD &dd = ( nanos::ext::FPGADD & )( wd->getActiveDevice() );
   dd.setHandle( task );
   *utask = ( nanos_fpga_task_t )( task );

   return NANOS_OK;
}

NANOS_API_DEF( nanos_err_t, nanos_fpga_set_task_arg, ( nanos_fpga_task_t utask, size_t argIdx,
   bool isInput, bool isOutput, uint64_t argValue ) )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "api", "nanos_fpga_set_task_arg" ); );
   xtasks_task_handle task = ( xtasks_task_handle )( utask );

   xtasks_arg_flags argFlags = XTASKS_ARG_FLAG_GLOBAL;
   argFlags |= XTASKS_ARG_FLAG_COPY_IN & -( isInput );
   argFlags |= XTASKS_ARG_FLAG_COPY_OUT & -( isOutput );

   if ( xtasksAddArg( argIdx, argFlags, argValue, task ) != XTASKS_SUCCESS ) {
      return NANOS_UNKNOWN_ERR;
   }

   return NANOS_OK;
}

NANOS_API_DEF( nanos_err_t, nanos_fpga_submit_task, ( nanos_fpga_task_t utask ) )
{
#ifdef NANOS_INSTRUMENTATION_ENABLED
   const nanos::ext::FPGAProcessorInfo & fpgaInfo =
      ( ( nanos::ext::FPGAProcessor * )( myThread->runningOn() ) )->getFPGAProcessorInfo();
   InstrumentBurst instBurst0( "api", "nanos_fpga_submit_task" );
   InstrumentBurst instBurst1( "fpga-accelerator-num", fpgaInfo.getId() + 1 );
#endif //NANOS_INSTRUMENTATION_ENABLED

   xtasks_task_handle task = ( xtasks_task_handle )( utask );
   if ( xtasksSubmitTask( task ) != XTASKS_SUCCESS ) {
      //TODO: If error is XTASKS_ENOMEM we can retry after a while
      fatal( "Error sending a task to the FPGA" );
      return NANOS_UNKNOWN_ERR;
   }

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

NANOS_API_DEF( void, nanos_fpga_create_wd_async, ( const unsigned long long int type, const unsigned char instanceNum,
   const unsigned char numArgs, const unsigned long long int * args,
   const unsigned char numDeps, const unsigned long long int * deps, const unsigned char * depsFlags,
   const unsigned char numCopies, const nanos_fpga_copyinfo_t * copies ) )
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

NANOS_API_DEF( unsigned long long int, nanos_fpga_current_wd, ( void ) )
{
   fatal( "The API nanos_fpga_current_wd can only be called from a FPGA device" );
}

NANOS_API_DEF( nanos_err_t, nanos_fpga_wg_wait_completion, ( unsigned long long int uwg, unsigned char avoid_flush ) )
{
   fatal( "The API nanos_fpga_wg_wait_completion can only be called from a FPGA device" );
}

NANOS_API_DEF( unsigned long long int, nanos_fpga_get_time_cycle, ( void ) )
{
   fatal( "The API nanos_fpga_get_time_cycle can only be called from a FPGA device" );
   return 0;
}

NANOS_API_DEF( unsigned long long int, nanos_fpga_get_time_us, ( void ) )
{
   fatal( "The API nanos_fpga_get_time_us can only be called from a FPGA device" );
   return 0;
}
