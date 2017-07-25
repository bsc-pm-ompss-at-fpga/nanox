/*************************************************************************************/
/*      Copyright 2010 Barcelona Supercomputing Center                               */
/*      Copyright 2009 Barcelona Supercomputing Center                               */
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

/*
 * TODO: Support the case where each thread may manage a different number of accelerators
 *       jbosch: This should be supported using different MultiThreads each one with a subset of accels
 */
FPGAProcessor::FPGAProcessor( FPGAProcessorInfo info, memory_space_id_t memSpaceId, Device const * arch ) :
   ProcessingElement( arch, memSpaceId, 0, 0, false, 0, false ), _fpgaProcessorInfo( info ),
   _pendingTasks( ), _readyTasks( ), _waitInTasks( )
#ifdef NANOS_INSTRUMENTATION_ENABLED
   , _dmaSubmitWarnShown( false )
#endif
{
#ifdef NANOS_INSTRUMENTATION_ENABLED
   int id;
   std::string devNum = toString( _fpgaProcessorInfo.getId() );

   id = sys.getNumInstrumentAccelerators();
   _devInstr = FPGAInstrumentation( id, std::string( "FPGA accelerator" ) + devNum );
   sys.addDeviceInstrumentation( &_devInstr );

   id = sys.getNumInstrumentAccelerators();
   _dmaInInstr = FPGAInstrumentation( id, std::string( "DMA in" ) + devNum );
   sys.addDeviceInstrumentation( &_dmaInInstr );

   id = sys.getNumInstrumentAccelerators();
   _dmaOutInstr = FPGAInstrumentation( id, std::string( "DMA in" ) + devNum );
   sys.addDeviceInstrumentation( &_dmaOutInstr );

   id = sys.getNumInstrumentAccelerators();
   _submitInstrumentation = FPGAInstrumentation( id, std::string( "DMA submit" ) + devNum );
   sys.addDeviceInstrumentation( &_submitInstrumentation );
#endif
}

FPGAProcessor::~FPGAProcessor()
{
   ensure( _pendingTasks.empty(), "Queue of FPGA pending tasks is not empty in one FPGAProcessor" );
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
#ifdef NANOS_DEBUG_ENABLED
   xtasks_stat status;
#endif
   xtasks_task_handle task;

   size_t numArgs = wd.getDataSize()/sizeof(uintptr_t);
   ensure( wd.getDataSize()%sizeof(uintptr_t) == 0,
           "WD's data size is not multiple of uintptr_t (All args must be pointers)" );

#ifndef NANOS_DEBUG_ENABLED
   xtasksCreateTask( (uintptr_t)&wd, _fpgaProcessorInfo.getHandle(), XTASKS_COMPUTE_ENABLE, &task );
#else
   status = xtasksCreateTask( (uintptr_t)&wd, _fpgaProcessorInfo.getHandle(), XTASKS_COMPUTE_ENABLE, &task );
   if ( status != XTASKS_SUCCESS ) {
      //TODO: If status == XTASKS_SUCCESS, block and wait untill mem is available
      fatal( "Cannot initialize FPGA task info (accId: " << _fpgaProcessorInfo.getId() << ", #args: "
             << numArgs << "): " << ( status == XTASKS_ENOMEM ? "XTASKS_ENOMEM" : "XTASKS_ERROR" ) );
   }
#endif

   uintptr_t * args = ( uintptr_t * )( wd.getData() );
   static FPGAPinnedAllocator * const allocator = getAllocator();
   static uintptr_t const baseAddress = allocator->getBaseAddress();
   static uintptr_t const baseAddressPhy = allocator->getBaseAddressPhy();
   xtasks_arg_val argValues[numArgs];
   for ( size_t argIdx = 0; argIdx < numArgs; ++argIdx ) {
      size_t  offset = args[argIdx] - baseAddress;
      argValues[argIdx] = baseAddressPhy + offset;
   }
#ifndef NANOS_DEBUG_ENABLED
   xtasksAddArgs( numArgs, XTASKS_GLOBAL, &argValues[0], task );
#else
   status = xtasksAddArgs( numArgs, XTASKS_GLOBAL, &argValues[0], task );
   ensure( status == XTASKS_SUCCESS, "Error adding arguments to a FPGA task" );
#endif

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
xtasks_ins_times *FPGAProcessor::getInstrCounters( FPGATaskInfo_t & task ) {
   xtasks_ins_times *times;
   xtasksGetInstrumentData( task._handle, &times );
   return times;
}

void FPGAProcessor::readInstrCounters( FPGATaskInfo_t & task ) {
   xtasks_ins_times * counters = getInstrCounters( task );
   Instrumentation * instr     = sys.getInstrumentation();
   WD * const wd = task._wd;

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

void FPGAProcessor::waitAndFinishTask( FPGATaskInfo_t & task ) {
   //Scheduler::postOutlineWork( wd, false, this );
   //Wait for the task to finish
   if ( xtasksWaitTask( task._handle ) != XTASKS_SUCCESS ) {
      fatal( "Error waiting for an FPGA task" );
   }

#ifdef NANOS_INSTRUMENTATION_ENABLED
   readInstrCounters( task );
#endif
   xtasksDeleteTask( &(task._handle) );
   //FPGAWorker::postOutlineWork( task._wd );
   Scheduler::postOutlineWork( task._wd, false, myThread, myThread->getCurrentWD() );
}

int FPGAProcessor::getPendingWDs() const {
   return _pendingTasks.size();
}

void FPGAProcessor::finishPendingWD( int numWD ) {
   FPGATaskInfo_t task;
   int cnt = 0;
   while ( cnt++ < numWD && _pendingTasks.try_pop( task ) ) {
      waitAndFinishTask( task );
   }
}

void FPGAProcessor::finishAllWD() {
   FPGATaskInfo_t task;
   while( _pendingTasks.try_pop( task ) ) {
      waitAndFinishTask( task );
   }
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

void FPGAProcessor::outlineWorkDependent ( WD &wd ) {
   //wd.start( WD::IsNotAUserLevelThread );
   xtasks_task_handle handle = createAndSubmitTask( wd );

   //set flag to allow new update
   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );

   _pendingTasks.push( FPGATaskInfo_t( &wd, handle ) );
}
