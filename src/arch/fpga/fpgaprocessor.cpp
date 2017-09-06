/*************************************************************************************/
/*      Copyright 2017 Barcelona Supercomputing Center                               */
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

#include "queue.hpp"
#include "fpgaprocessor.hpp"
#include "fpgadd.hpp"
#include "fpgathread.hpp"
#include "fpgaconfig.hpp"
#include "fpgaworker.hpp"
#include "fpgaprocessorinfo.hpp"
#include "instrumentationmodule_decl.hpp"
#include "smpprocessor.hpp"
#include "fpgapinnedallocator.hpp"
#include "fpgainstrumentation.hpp"
#include "simpleallocator.hpp"

#include "libxtasks.h"

using namespace nanos;
using namespace nanos::ext;

#ifdef NANOS_INSTRUMENTATION_ENABLED
Atomic<size_t> FPGAProcessor::_totalRunningTasks( 0 );
#endif

/*
 * TODO: Support the case where each thread may manage a different number of accelerators
 *       jbosch: This should be supported using different MultiThreads each one with a subset of accels
 */
FPGAProcessor::FPGAProcessor( FPGAProcessorInfo info, memory_space_id_t memSpaceId, Device const * arch ) :
   ProcessingElement( arch, memSpaceId, 0, 0, false, 0, false ), _fpgaProcessorInfo( info ),
   _runningTasks( 0 ), _readyTasks( ), _waitInTasks( )
#ifdef NANOS_INSTRUMENTATION_ENABLED
   , _dmaSubmitWarnShown( false )
#endif
{
#ifdef NANOS_INSTRUMENTATION_ENABLED
   if ( !FPGAConfig::isInstrDisabled() ) {
      int id;
      std::string devNum = toString( _fpgaProcessorInfo.getId() );

      id = sys.getNumInstrumentAccelerators();
      _devInstr = FPGAInstrumentation( id, std::string( "FPGA accelerator " ) + devNum );
      sys.addDeviceInstrumentation( &_devInstr );

      id = sys.getNumInstrumentAccelerators();
      _dmaInInstr = FPGAInstrumentation( id, std::string( "DMA in " ) + devNum );
      sys.addDeviceInstrumentation( &_dmaInInstr );

      id = sys.getNumInstrumentAccelerators();
      _dmaOutInstr = FPGAInstrumentation( id, std::string( "DMA out " ) + devNum );
      sys.addDeviceInstrumentation( &_dmaOutInstr );

      id = sys.getNumInstrumentAccelerators();
      _submitInstrumentation = FPGAInstrumentation( id, std::string( "DMA submit " ) + devNum );
      sys.addDeviceInstrumentation( &_submitInstrumentation );
   }
#endif
}

FPGAProcessor::~FPGAProcessor()
{
   ensure( _runningTasks.value() == 0, "There are running task in one FPGAProcessor" );
   ensure( _readyTasks.empty(), "Queue of FPGA ready tasks is not empty in one FPGAProcessor" );
   ensure( _waitInTasks.empty(),  "Queue of FPGA input waiting tasks is not empty in one FPGAProcessor" );
}

WorkDescriptor & FPGAProcessor::getWorkerWD () const
{
   //SMPDD *dd = NEW SMPDD( ( SMPDD::work_fct )Scheduler::workerLoop );
   SMPDD *dd = NEW SMPDD( ( SMPDD::work_fct )FPGAWorker::FPGAWorkerLoop );
   WD *wd = NEW WD( dd );
   return *wd;
}

WD & FPGAProcessor::getMasterWD () const
{
   fatal("Attempting to create a FPGA master thread");
}

BaseThread & FPGAProcessor::createThread ( WorkDescriptor &helper, SMPMultiThread *parent )
{
   ensure( helper.canRunIn( getSMPDevice() ), "Incompatible worker thread" );
   FPGAThread  &th = *NEW FPGAThread( helper, this, parent );
   return th;
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAProcessor::dmaSubmitStart( const WD *wd )
{
   if ( FPGAConfig::isInstrDisabled() ) return;

   uint64_t timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      if ( !_dmaSubmitWarnShown ) {
         warning("Could not read accelerator " << getAccelId() <<
                 " clock (dma submit start). [Warning only shown once]");
         _dmaSubmitWarnShown = true;
      }
   } else {
      Instrumentation *instr = sys.getInstrumentation();
      instr->addDeviceEvent( Instrumentation::DeviceEvent(
         timestamp, TaskBegin, &_submitInstrumentation, wd ) );
      instr->addDeviceEvent( Instrumentation::DeviceEvent(
         timestamp, TaskSwitch, &_submitInstrumentation, NULL, wd ) );
   }
}

void FPGAProcessor::dmaSubmitEnd( const WD *wd )
{
   if ( FPGAConfig::isInstrDisabled() ) return;

   uint64_t timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      if ( !_dmaSubmitWarnShown ) {
         warning("Could not read accelerator " << getAccelId() <<
                 " clock (dma submit end). [Warning only shown once]");
         _dmaSubmitWarnShown = true;
      }
   } else {
      Instrumentation *instr = sys.getInstrumentation();
      //FIXME: Emit the accelerator ID
      instr->addDeviceEvent( Instrumentation::DeviceEvent(
         timestamp, TaskSwitch, &_submitInstrumentation, wd, NULL ) );
      instr->addDeviceEvent( Instrumentation::DeviceEvent(
         timestamp, TaskEnd, &_submitInstrumentation, wd ) );
   }
}
#endif  //NANOS_INSTRUMENTATION_ENABLED

xtasks_task_handle FPGAProcessor::createAndSubmitTask( WD &wd ) {
   xtasks_stat status;
   xtasks_task_handle task;

   NANOS_INSTRUMENT( InstrumentBurst( "fpga-accelerator-num", _fpgaProcessorInfo.getId() + 1 ) );

   size_t numArgs = wd.getDataSize()/sizeof(uintptr_t);
   ensure( wd.getDataSize()%sizeof(uintptr_t) == 0,
           "WD's data size is not multiple of uintptr_t (All args must be pointers)" );

   status = xtasksCreateTask( (uintptr_t)&wd, _fpgaProcessorInfo.getHandle(), XTASKS_COMPUTE_ENABLE, &task );
   if ( status != XTASKS_SUCCESS ) {
      //TODO: If status == XTASKS_ENOMEM, block and wait untill mem is available
      fatal( "Cannot initialize FPGA task info (accId: " <<
             _fpgaProcessorInfo.getId() << ", #args: " << numArgs << "): " <<
             ( status == XTASKS_ENOMEM ? "XTASKS_ENOMEM" : "XTASKS_ERROR" ) );
   }

   uintptr_t * args = ( uintptr_t * )( wd.getData() );
   static FPGAPinnedAllocator * const allocator = getAllocator();
   static uintptr_t const baseAddress = allocator->getBaseAddress();
   static uintptr_t const baseAddressPhy = allocator->getBaseAddressPhy();
   xtasks_arg_val argValues[numArgs];
   for ( size_t argIdx = 0; argIdx < numArgs; ++argIdx ) {
      size_t  offset = args[argIdx] - baseAddress;
      argValues[argIdx] = baseAddressPhy + offset;
   }

   status = xtasksAddArgs( numArgs, XTASKS_GLOBAL, &argValues[0], task );
   if ( status != XTASKS_SUCCESS ) {
      ensure( status == XTASKS_SUCCESS, "Error adding arguments to a FPGA task" );
   }

#ifdef NANOS_INSTRUMENTATION_ENABLED
   dmaSubmitStart( &wd );
#endif
   if ( xtasksSubmitTask( task ) != XTASKS_SUCCESS ) {
      //TODO: If error is XDMA_ENOMEM we can retry after a while
      fatal("Error sending a task to the FPGA");
   }
#ifdef NANOS_INSTRUMENTATION_ENABLED
   dmaSubmitEnd( &wd );
#endif
   return task;
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAProcessor::readInstrCounters( WD * const wd, xtasks_task_handle & task ) {
   if ( FPGAConfig::isInstrDisabled() ) return;

   Instrumentation * instr = sys.getInstrumentation();

   //Get the counters
   xtasks_ins_times * counters;
   xtasksGetInstrumentData( task, &counters );

   //Task execution
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskBegin, &_devInstr, wd ) );
   //Beginning kernel execution is represented as a task switch from NULL to a WD
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, &_devInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, &_devInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskEnd, &_devInstr, wd ) );

   //DMA info
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, &_dmaInInstr, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskSwitch, &_dmaInInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, &_dmaInInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskEnd, &_dmaInInstr, wd ) );

   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskBegin, &_dmaOutInstr, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, &_dmaOutInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskSwitch, &_dmaOutInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, &_dmaOutInstr, wd ) );
}
#endif

int FPGAProcessor::getPendingWDs() const {
   return _runningTasks.value();
}

bool FPGAProcessor::inlineWorkDependent( WD &wd )
{
   verbose( "fpga inlineWorkDependent" );

   wd.start( WD::IsNotAUserLevelThread );
   //FPGAProcessor* fpga = ( FPGAProcessor * ) this->runningOn();

   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );

   return true;
}

void FPGAProcessor::preOutlineWorkDependent ( WD &wd ) {
   wd.preStart(WorkDescriptor::IsNotAUserLevelThread);
}

void FPGAProcessor::outlineWorkDependent ( WD &wd )
{
   //wd.start( WD::IsNotAUserLevelThread );
   createAndSubmitTask( wd );
   ++_runningTasks;
#ifdef NANOS_INSTRUMENTATION_ENABLED
   ++_totalRunningTasks;
   instrumentPoint( "fpga-run-tasks", _totalRunningTasks.value() );
#endif

   //set flag to allow new update
   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );
}

bool FPGAProcessor::tryPostOutlineTasks( size_t max )
{
   bool ret = false;
   xtasks_task_handle xHandle;
   xtasks_task_id xId;
   xtasks_stat xStat;
   for ( size_t cnt = 0; cnt < max; ++cnt ) {
      xStat = xtasksTryGetFinishedTaskAccel( _fpgaProcessorInfo.getHandle(), &xHandle, &xId );
      if ( xStat == XTASKS_SUCCESS ) {
         WD * wd = (WD *)xId;
         ret = true;

#ifdef NANOS_INSTRUMENTATION_ENABLED
         --_totalRunningTasks;
         instrumentPoint( "fpga-run-tasks", _totalRunningTasks.value() );
         InstrumentBurst( "fpga-finish-task", wd->getId() );
         readInstrCounters( wd, xHandle );
#endif
         xtasksDeleteTask( &xHandle );
         --_runningTasks;
         Scheduler::postOutlineWork( wd, false, myThread, myThread->getCurrentWD() );
      } else {
         ensure( xStat == XTASKS_PENDING, "Error trying to get a finished FPGA task" );
         break;
      }
   }
   return ret;
}
