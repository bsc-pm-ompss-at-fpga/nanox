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
#include "libxtasks_wrapper.hpp"

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
#ifdef NANOS_DEBUG_ENABLED
   , _totalTasks( 0 )
#endif
#ifdef NANOS_INSTRUMENTATION_ENABLED
   , _dmaSubmitWarnShown( false )
#endif
{
#ifdef NANOS_INSTRUMENTATION_ENABLED
   if ( !FPGAConfig::isInstrDisabled() ) {
      _devInstr = FPGAInstrumentation( _fpgaProcessorInfo );
      sys.addDeviceInstrumentation( &_devInstr );
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

void FPGAProcessor::createAndSubmitTask( WD &wd, WD *parentWd ) {
   xtasks_stat status;
   xtasks_task_handle task, parentTask = NULL;

   NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-accelerator-num", _fpgaProcessorInfo.getId() + 1 ) );

   if (parentWd != NULL) {
      FPGADD &dd = ( FPGADD & )( parentWd->getActiveDevice() );
      parentTask = ( xtasks_task_handle )( dd.getHandle() );
   }

   size_t numArgs = wd.getDataSize()/sizeof(uintptr_t);
   ensure( wd.getDataSize()%sizeof(uintptr_t) == 0,
           "WD's data size is not multiple of uintptr_t (All args must be pointers)" );

   status = xtasksCreateTask( (uintptr_t)&wd, _fpgaProcessorInfo.getHandle(), parentTask,
      XTASKS_COMPUTE_ENABLE, &task );
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

   if ( xtasksSubmitTask( task ) != XTASKS_SUCCESS ) {
      //TODO: If error is XDMA_ENOMEM we can retry after a while
      fatal("Error sending a task to the FPGA");
   }

   FPGADD &dd = ( FPGADD & )( wd.getActiveDevice() );
   dd.setHandle( task );
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAProcessor::readInstrCounters( WD * const wd, xtasks_task_handle & task ) {
   if ( FPGAConfig::isInstrDisabled() ) return;

   static Instrumentation * ins     = sys.getInstrumentation();
   static nanos_event_key_t inKey   = ins->getInstrumentationDictionary()->getEventKey( "device-copy-in" );
   static nanos_event_key_t execKey = ins->getInstrumentationDictionary()->getEventKey( "device-task-execution" );
   static nanos_event_key_t outKey  = ins->getInstrumentationDictionary()->getEventKey( "device-copy-out" );
   nanos_event_value_t val = ( nanos_event_value_t )( wd->getId() );

   //Get the counters
   xtasks_ins_times * counters;
   xtasksGetInstrumentData( task, &counters );

   //Raise the events
   ins->raiseDeviceBurstEvent( _devInstr, inKey,   val, counters->start,       counters->inTransfer  );
   ins->raiseDeviceBurstEvent( _devInstr, execKey, val, counters->inTransfer,  counters->computation );
   ins->raiseDeviceBurstEvent( _devInstr, outKey,  val, counters->computation, counters->outTransfer );
}
#endif

int FPGAProcessor::getPendingWDs() const {
   return _runningTasks.value();
}

bool FPGAProcessor::inlineWorkDependent( WD &wd )
{
   wd.start( WD::IsNotAUserLevelThread );
#ifdef NANOS_DEBUG_ENABLED
   ++_totalTasks;
#endif

   outlineWorkDependent( wd );
   while ( !wd.isDone() ) {
      myThread->idle();
      //! NOTE: FPGAProcessor::tryPostOutlineTasks must be directly called here because the fpga-idle-callback
      //        may be disabled. Therefore, it won't be called inside myThread->idle() through the EventDispatcher
      tryPostOutlineTasks();
   }

   return true;
}

void FPGAProcessor::preOutlineWorkDependent ( WD &wd ) {
   wd.preStart(WorkDescriptor::IsNotAUserLevelThread);
#ifdef NANOS_DEBUG_ENABLED
   ++_totalTasks;
#endif
}

void FPGAProcessor::outlineWorkDependent ( WD &wd )
{
   //wd.start( WD::IsNotAUserLevelThread );
   createAndSubmitTask( wd, wd.getParent() );
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
         InstrumentBurst instBurst( "fpga-finish-task", wd->getId() );
         readInstrCounters( wd, xHandle );
#endif
         xtasksDeleteTask( &xHandle );
         --_runningTasks;
         if ( wd->isOutlined() ) {
            //Only delete tasks executed using outlineWorkDependent
            Scheduler::postOutlineWork( wd, true /* schedule */, myThread );
            delete[] (char *) wd;
         } else {
            //Mark inline tasks as done, they will be finished from the Scheduler
            wd->setDone();
         }
      } else {
         ensure( xStat == XTASKS_PENDING, "Error trying to get a finished FPGA task" );
         break;
      }
   }
   return ret;
}
