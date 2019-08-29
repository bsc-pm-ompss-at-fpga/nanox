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

#include "queue.hpp"
#include "fpgaprocessor.hpp"
#include "fpgadd.hpp"
#include "fpgathread.hpp"
#include "fpgaconfig.hpp"
#include "fpgaworker.hpp"
#include "fpgaprocessorinfo.hpp"
#include "instrumentationmodule_decl.hpp"
#include "instrumentation.hpp"
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

std::vector< FPGAProcessor* > *nanos::ext::fpgaPEs;

/*
 * TODO: Support the case where each thread may manage a different number of accelerators
 *       jbosch: This should be supported using different MultiThreads each one with a subset of accels
 */
FPGAProcessor::FPGAProcessor( FPGAProcessorInfo info, memory_space_id_t memSpaceId, Device const * arch ) :
   ProcessingElement( arch, memSpaceId, 0, 0, false, 0, false ), _fpgaProcessorInfo( info ),
   _runningTasks( 0 ), _readyTasks( ), _waitInTasks( )
#if defined(NANOS_DEBUG_ENABLED) || defined(NANOS_INSTRUMENTATION_ENABLED)
   , _totalTasks( 0 )
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
   ensure( _runningTasks.value() == 0, " There are running task in one FPGAProcessor" );
   ensure( _readyTasks.empty(), " Queue of FPGA ready tasks is not empty in one FPGAProcessor" );
   ensure( _waitInTasks.empty(), " Queue of FPGA input waiting tasks is not empty in one FPGAProcessor" );
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
   fatal( "Attempting to create a FPGA master thread" );
}

BaseThread & FPGAProcessor::createThread ( WorkDescriptor &helper, SMPMultiThread *parent )
{
   ensure( helper.canRunIn( getSMPDevice() ), " Incompatible worker thread" );
   FPGAThread  &th = *NEW FPGAThread( helper, this, parent );
   return th;
}

void FPGAProcessor::createTask( WD &wd, WD *parentWd ) {
   xtasks_stat status;
   xtasks_task_handle task, parentTask = NULL;

   if ( parentWd != NULL ) {
      FPGADD &dd = ( FPGADD & )( parentWd->getActiveDevice() );
      parentTask = ( xtasks_task_handle )( dd.getHandle() );
   }

   status = xtasksCreateTask( ( uintptr_t )( &wd ), _fpgaProcessorInfo.getHandle(), parentTask,
      XTASKS_COMPUTE_ENABLE, &task );
   if ( status != XTASKS_SUCCESS ) {
      //TODO: If status == XTASKS_ENOMEM, block and wait untill mem is available
      fatal( "Cannot initialize FPGA task info (accId: " <<
             _fpgaProcessorInfo.getId() << "): " <<
             ( status == XTASKS_ENOMEM ? "XTASKS_ENOMEM" : "XTASKS_ERROR" ) );
   }

   FPGADD &dd = ( FPGADD & )( wd.getActiveDevice() );
   dd.setHandle( task );
}

void FPGAProcessor::setTaskArg( WD &wd, size_t argIdx, bool isInput, bool isOutput, uint64_t argValue ) {
   xtasks_task_handle task;

   FPGADD &dd = ( FPGADD & )( wd.getActiveDevice() );
   task = ( xtasks_task_handle )( dd.getHandle() );

   xtasks_arg_flags argFlags = XTASKS_ARG_FLAG_GLOBAL;
   argFlags |= XTASKS_ARG_FLAG_COPY_IN & -( isInput );
   argFlags |= XTASKS_ARG_FLAG_COPY_OUT & -( isOutput );

   if ( xtasksAddArg( argIdx, argFlags, argValue, task ) != XTASKS_SUCCESS ) {
      fatal( "Error adding argument to a task" );
   }

}

void FPGAProcessor::submitTask( WD &wd ) {
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-accelerator-num", _fpgaProcessorInfo.getId() + 1 ) );

   //xtasks_stat status;
   xtasks_task_handle task;

   FPGADD &dd = ( FPGADD & )( wd.getActiveDevice() );
   task = ( xtasks_task_handle )( dd.getHandle() );

   if ( xtasksSubmitTask( task ) != XTASKS_SUCCESS ) {
      //TODO: If error is XTASKS_ENOMEM we can retry after a while
      fatal( "Error sending a task to the FPGA" );
   }

}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAProcessor::handleInstrumentation() {
   if ( FPGAConfig::isInstrDisabled() ) return;

   static Instrumentation * ins = sys.getInstrumentation();
   size_t maxEvents = FPGAConfig::getNumInstrEvents();
   xtasks_ins_event *events = NEW xtasks_ins_event[maxEvents];
   Instrumentation::DeviceEvent *deviceEvents = NEW Instrumentation::DeviceEvent[maxEvents];

   xtasksGetInstrumentData( getFPGAProcessorInfo().getHandle(), events, maxEvents );

   unsigned int writeEv = 0;
   for ( unsigned int readEv = 0;
           readEv < maxEvents && events[readEv].eventType != XTASKS_EVENT_TYPE_INVALID;
           readEv++ )
   {
      //Emit extrae events
      xtasks_ins_event &event = events[readEv];
      switch ( event.eventType ) {
         case XTASKS_EVENT_TYPE_BURST_OPEN:
            ins->createDeviceBurstEvent( &deviceEvents[writeEv++],
                  event.eventId, event.value, event.timestamp );
            break;
         case XTASKS_EVENT_TYPE_BURST_CLOSE:
            ins->closeDeviceBurstEvent( &deviceEvents[writeEv++],
                  event.eventId, event.value, event.timestamp );
            break;
         case XTASKS_EVENT_TYPE_POINT:
            ins->createDevicePointEvent( &deviceEvents[writeEv++],
                  event.eventId, event.value, event.timestamp );
            break;
         default:
            warning( "Ignoring unknown fpga event type: " << event.eventType );
      }
   }

   if (writeEv > 0) {
      ins->addDeviceEventList( _devInstr, writeEv, deviceEvents );
   }

   delete[] events;
   delete[] deviceEvents;
}
#endif

int FPGAProcessor::getPendingWDs() const {
   return _runningTasks.value();
}

bool FPGAProcessor::inlineWorkDependent( WD &wd )
{
   wd.start( WD::IsNotAUserLevelThread );
#if defined(NANOS_DEBUG_ENABLED) || defined(NANOS_INSTRUMENTATION_ENABLED)
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
   //NOTE: Check again that the task can be outlined to the accel as it may come from system::outline
   static unsigned int const maxPendingWD = FPGAConfig::getMaxPendingWD();
   while ( _runningTasks >= maxPendingWD ) {
      //TODO: Check if at this point is better to create a SynchronizedCondition
      tryPostOutlineTasks();
   }

   wd.preStart(WorkDescriptor::IsNotAUserLevelThread);
#if defined(NANOS_DEBUG_ENABLED) || defined(NANOS_INSTRUMENTATION_ENABLED)
   ++_totalTasks;
#endif
}

void FPGAProcessor::outlineWorkDependent ( WD &wd )
{
   //wd.start( WD::IsNotAUserLevelThread );
   createTask( wd, wd.getParent() );

   //set flag to allow new update
   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );

   ++_runningTasks;
#ifdef NANOS_INSTRUMENTATION_ENABLED
   ++_totalRunningTasks;
   instrumentPoint( "fpga-run-tasks", _totalRunningTasks.value() );
#endif

   submitTask( wd );
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
         ensure( wd->getNumComponents() == 0,
            " Executing postOutlineWork for a FPGA task with alive children not supported. Should be fixed with a taskwait." );
         ret = true;

#ifdef NANOS_INSTRUMENTATION_ENABLED
         --_totalRunningTasks;
         instrumentPoint( "fpga-run-tasks", _totalRunningTasks.value() );
         InstrumentBurst instBurst( "fpga-finish-task", wd->getId() );
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
         ensure( xStat == XTASKS_PENDING, " Error trying to get a finished FPGA task" );
         break;
      }
   }
   return ret;
}

bool FPGAProcessor::tryAcquireExecLock() {
   return _execLock.tryAcquire();
}

void FPGAProcessor::releaseExecLock() {
   return _execLock.release();
}
