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

#include "libxdma.h"

using namespace nanos;
using namespace nanos::ext;

//Lock FPGAProcessor::_initLock;

/*
 * TODO: Support the case where each thread may manage a different number of accelerators
 *       jbosch: This should be supported using different MultiThreads each one with a subset of accels
 */
FPGAProcessor::FPGAProcessor( int const accId, memory_space_id_t memSpaceId, Device const * arch ) :
   ProcessingElement( arch, memSpaceId, 0, 0, false, 0, false ), _accelId( accId ),
   _pendingTasks( ), _readyTasks( ), _waitInTasks( )
#ifdef NANOS_INSTRUMENTATION_ENABLED
   , _dmaSubmitWarnShown( false )
#endif
{
   _fpgaProcessorInfo = NEW FPGAProcessorInfo;
}

FPGAProcessor::~FPGAProcessor()
{
   delete _fpgaProcessorInfo;
}

void FPGAProcessor::init()
{
   int fpgaCount = FPGAConfig::getFPGACount();
   xdma_device *devices = NEW xdma_device[fpgaCount];
   xdma_status status;
   int copiedDevices = -1;
   status = xdmaGetDevices(fpgaCount,  devices, &copiedDevices);
   ensure0(status == XDMA_SUCCESS, "Error getting FPGA devices information");
   ensure0(copiedDevices > _accelId, "Number of devices from the xdma library is smaller the used index");

   xdma_channel iChan, oChan;
   _fpgaProcessorInfo->setDeviceHandle( devices[_accelId] );

   //open input channel
   NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REQ_CHANNEL_EVENT );
   status = xdmaGetDeviceChannel(devices[_accelId], XDMA_TO_DEVICE, &iChan);
   NANOS_FPGA_CLOSE_RUNTIME_EVENT;

   if (status)
       warning0("Error opening DMA input channel: " << status);

   debug0("got input channel " << iChan );

   _fpgaProcessorInfo->setInputChannel( iChan );

   NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REQ_CHANNEL_EVENT );
   status = xdmaGetDeviceChannel(devices[_accelId], XDMA_FROM_DEVICE, &oChan);
   NANOS_FPGA_CLOSE_RUNTIME_EVENT;
   if (status || !oChan)
       warning0("Error opening DMA output channel");

   debug0("got output channel " << oChan );

   _fpgaProcessorInfo->setOutputChannel( oChan );
   delete devices;
}

void FPGAProcessor::cleanUp()
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
static void dmaSubmitStart( FPGAProcessor *fpga, const WD *wd ) {
   uint64_t timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      if ( !fpga->_dmaSubmitWarnShown ) {
         warning("Could not read accelerator " << fpga->getAccelId() <<
                 " clock (dma submit start). [Warning only shown once]");
         fpga->_dmaSubmitWarnShown = true;
      }
   } else {
      Instrumentation *instr = sys.getInstrumentation();
      DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
      instr->addDeviceEvent(
              Instrumentation::DeviceEvent( timestamp, TaskBegin, submitInstr, wd ) );
      instr->addDeviceEvent(
              Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, NULL, wd) );
   }
}

static void dmaSubmitEnd( FPGAProcessor *fpga, const WD *wd ) {
   uint64_t timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      if ( !fpga->_dmaSubmitWarnShown ) {
         warning("Could not read accelerator " << fpga->getAccelId() <<
                 " clock (dma submit end). [Warning only shown once]");
         fpga->_dmaSubmitWarnShown = true;
      }
   } else {
      Instrumentation *instr = sys.getInstrumentation();
      //FIXME: Emit the accelerator ID
      DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
      instr->addDeviceEvent(
            Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, wd, NULL) );
      instr->addDeviceEvent(
            Instrumentation::DeviceEvent( timestamp, TaskEnd, submitInstr, wd) );
   }
}
#endif  //NANOS_INSTRUMENTATION_ENABLED

xdma_task_handle FPGAProcessor::createAndSubmitTask( WD &wd ) {
#ifdef NANOS_DEBUG_ENABLED
   xdma_status status;
#endif
   xdma_task_handle task;

   size_t numArgs = wd.getDataSize()/sizeof(uintptr_t);
   ensure( wd.getDataSize()%sizeof(uintptr_t) == 0,
           "WD's data size is not multiple of uintptr_t (All args must be pointers)" );

#ifndef NANOS_DEBUG_ENABLED
   xdmaInitTask( _accelId, XDMA_COMPUTE_ENABLE, &task );
#else
   status = xdmaInitTask( _accelId, XDMA_COMPUTE_ENABLE, &task );
   if ( status != XDMA_SUCCESS ) {
      //TODO: If status == XDMA_ENOMEM, block and wait untill mem is available
      fatal( "Cannot initialize FPGA task info (accId: " << _accelId << ", #args: " << numArgs <<
             "): " << ( status == XDMA_ENOMEM ? "XDMA_ENOMEM" : "XDMA_ERROR" ) );
   }
#endif

   uintptr_t * args = ( uintptr_t * )( wd.getData() );
   FPGAPinnedAllocator * const allocator = getAllocator();
   xdma_buf_handle handle = allocator->getBufferHandle();
   uintptr_t const baseAddress = allocator->getBaseAddress();
   ensure( baseAddress > 0,
           "The base address of FPGA Allocator is not valid and FPGA task cannot be sent" );
   for ( size_t argIdx = 0; argIdx < numArgs; ++argIdx ) {
      uintptr_t addr = args[argIdx];
      size_t  offset = addr - baseAddress;
      xdmaAddArg( task, argIdx, XDMA_GLOBAL, handle, offset );
   }
#ifdef NANOS_INSTRUMENTATION_ENABLED
   dmaSubmitStart( this, &wd );
#endif
   if ( xdmaSendTask(_fpgaProcessorInfo->getDeviceHandle(), task) != XDMA_SUCCESS ) {
      //TODO: If error is XDMA_ENOMEM we can retry after a while
      fatal("Error sending a task to the FPGA");
   }
#ifdef NANOS_INSTRUMENTATION_ENABLED
   dmaSubmitEnd( this, &wd );
#endif
   return task;
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
xdma_instr_times *FPGAProcessor::getInstrCounters( FPGATaskInfo_t & task ) {
   xdma_instr_times *times;
   xdmaGetInstrumentData( task._handle, &times );
   return times;
}

void FPGAProcessor::readInstrCounters( FPGATaskInfo_t & task ) {
   xdma_instr_times * counters = getInstrCounters( task );
   Instrumentation * instr     = sys.getInstrumentation();
   WD * const wd = task._wd;

   //Task execution
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskBegin, _devInstr, wd ) );
   //Beginning kernel execution is represented as a task switch from NULL to a WD
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, _devInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, _devInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskEnd, _devInstr, wd ) );

   //DMA info
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, _dmaInInstr, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskSwitch, _dmaInInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, _dmaInInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskEnd, _dmaInInstr, wd ) );

   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskBegin, _dmaOutInstr, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, _dmaOutInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskSwitch, _dmaOutInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, _dmaOutInstr, wd ) );

   xdmaClearTaskTimes( (xdma_instr_times*) counters );
   //_hwInstrCounters.erase( wd );

}
#endif

void FPGAProcessor::waitAndFinishTask( FPGATaskInfo_t & task ) {
   //Scheduler::postOutlineWork( wd, false, this );
   //Wait for the task to finish
   if ( xdmaWaitTask( task._handle ) != XDMA_SUCCESS ) {
      fatal( "Error waiting for an FPGA task" );
   }

#ifdef NANOS_INSTRUMENTATION_ENABLED
   readInstrCounters( task );
#endif
   xdmaDeleteTask( &(task._handle) );
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
   xdma_task_handle handle = createAndSubmitTask( wd );

   //set flag to allow new opdate
   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );

   _pendingTasks.push( FPGATaskInfo_t( &wd, handle ) );
}
