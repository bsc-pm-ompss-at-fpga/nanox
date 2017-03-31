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
#include "fpgamemorytransfer.hpp"
#include "instrumentationmodule_decl.hpp"
#include "smpprocessor.hpp"
#include "fpgapinnedallocator.hpp"
#include "fpgainstrumentation.hpp"

#include "libxdma.h"

using namespace nanos;
using namespace nanos::ext;

//Lock FPGAProcessor::_initLock;
//TODO: We should have an FPGAPinnedAllocator for each FPGAProcessor and only have one FPGAProcessor
//      with several FPGADevices
FPGAPinnedAllocator FPGAProcessor::_allocator( 1024*1024*64 );
/*
 * TODO: Support the case where each thread may manage a different number of accelerators
 *       jbosch: This should be supported using different MultiThreads each one with a subset of accels
 */
FPGAProcessor::FPGAProcessor( int const accId, memory_space_id_t memSpaceId, Device const * arch ) :
   ProcessingElement( arch, memSpaceId, 0, 0, false, 0, false ), _accelId( accId ),
   _pendingTasks( ), _readyTasks( ), _waitInTasks( )
{
   _fpgaProcessorInfo = NEW FPGAProcessorInfo;
   _inputTransfers = NEW FPGAMemoryInTransferList();
   _outputTransfers = NEW FPGAMemoryOutTransferList(*this);
}

FPGAProcessor::~FPGAProcessor()
{
   delete _fpgaProcessorInfo;
   delete _inputTransfers;
   delete _outputTransfers;
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
   NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REQ_CHANNEL_EVENT);
   status = xdmaOpenChannel(devices[_accelId], XDMA_TO_DEVICE, XDMA_CH_NONE, &iChan);
   NANOS_FPGA_CLOSE_RUNTIME_EVENT;

   if (status)
       warning0("Error opening DMA input channel: " << status);

   debug0("got input channel " << iChan );

   _fpgaProcessorInfo->setInputChannel( iChan );

   NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REQ_CHANNEL_EVENT );
   status = xdmaOpenChannel(devices[_accelId], XDMA_FROM_DEVICE, XDMA_CH_NONE, &oChan);
   NANOS_FPGA_CLOSE_RUNTIME_EVENT;
   if (status || !oChan)
       warning0("Error opening DMA output channel");

   debug0("got output channel " << oChan );

   _fpgaProcessorInfo->setOutputChannel( oChan );
   delete devices;
}

void FPGAProcessor::cleanUp()
{

    //release channels
    xdma_status status;
    xdma_channel tmpChannel;

    //wait for remaining transfers that could remain
    _inputTransfers->syncAll();
    _outputTransfers->syncAll();

    debug("Release DMA channels");
    NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REL_CHANNEL_EVENT );
    tmpChannel = _fpgaProcessorInfo->getInputChannel();
    debug("release input channel " << _fpgaProcessorInfo->getInputChannel() );
    status = xdmaCloseChannel( &tmpChannel );
    debug("  channel released");
    //Update the new channel as it may be modified by closing the channel
    _fpgaProcessorInfo->setInputChannel( tmpChannel );
    NANOS_FPGA_CLOSE_RUNTIME_EVENT;

    if ( status )
        warning ( "Failed to release input dma channel" );

    NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REL_CHANNEL_EVENT );
    tmpChannel = _fpgaProcessorInfo->getOutputChannel();
    debug("release output channel " << _fpgaProcessorInfo->getOutputChannel() );
    status = xdmaCloseChannel( &tmpChannel );
    debug("  channel released");
    _fpgaProcessorInfo->setOutputChannel( tmpChannel );
    NANOS_FPGA_CLOSE_RUNTIME_EVENT;

    if ( status )
        warning ( "Failed to release output dma channel" );
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
   Instrumentation *instr = sys.getInstrumentation();
   DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
   unsigned long long timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      warning("Could not read accelerator clock (dma submit start)");
   }

   instr->addDeviceEvent(
           Instrumentation::DeviceEvent( timestamp, TaskBegin, submitInstr, wd ) );
   instr->addDeviceEvent(
           Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, NULL, wd) );
}

static void dmaSubmitEnd( FPGAProcessor *fpga, const WD *wd ) {
   Instrumentation *instr = sys.getInstrumentation();
   //FIXME: Emit the accelerator ID
   DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
   unsigned long long timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      warning("Could not read accelerator clock (dma submit end)");
   }

   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, wd, NULL) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskEnd, submitInstr, wd) );
}
#endif  //NANOS_INSTRUMENTATION_ENABLED

xdma_task_handle FPGAProcessor::createAndSubmitTask( WD &wd ) {
#ifdef NANOS_DEBUG_ENABLED
   xdma_status status;
#endif
   xdma_task_handle task;
   int numCopies;
   CopyData *copies;
   int numInputs, numOutputs;
   numCopies = wd.getNumCopies();
   copies = wd.getCopies();
   numInputs = 0;
   numOutputs = 0;
   for ( int i=0; i<numCopies; i++ ) {
      if ( copies[i].isInput() ) {
         numInputs++;
      }
      if ( copies[i].isOutput() ) {
         numOutputs++;
      }
   }

#ifndef NANOS_DEBUG_ENABLED
   xdmaInitTask( _accelId, numInputs, XDMA_COMPUTE_ENABLE, numOutputs, &task );
#else
   status = xdmaInitTask( _accelId, numInputs, XDMA_COMPUTE_ENABLE, numOutputs, &task );
   if ( status != XDMA_SUCCESS ) {
      //TODO: If status == XDMA_ENOMEM, block and wait untill mem is available
      fatal( "Cannot initialize FPGA task info (accId: " << _accelId << ", #in: " << numInputs << ", #out: "
             << numOutputs << "): " << ( status == XDMA_ENOMEM ? "XDMA_ENOMEM" : "XDMA_ERROR" ) );
   }
#endif
   int inputIdx, outputIdx;
   inputIdx = 0; outputIdx = 0;
   for ( int i=0; i<numCopies; i++ ) {
      //Get handle & offset based on copy address
      int size;
      uint64_t srcAddress, baseAddress, offset;
      xdma_buf_handle copyHandle;

      size = copies[i].getSize();
      srcAddress = wd._mcontrol.getAddress( i );
      baseAddress = (uint64_t)_allocator.getBasePointer( (void *)srcAddress, size );
      ensure( baseAddress > 0,
         "Trying to register an invalid FPGA data copy. The memory region is not registered in the FPGA Allocator." );
      offset = srcAddress - baseAddress;
      copyHandle = _allocator.getBufferHandle( (void *)baseAddress );

      if ( copies[i].isInput() ) {
         xdmaAddDataCopy(&task, inputIdx, XDMA_GLOBAL, XDMA_TO_DEVICE,
            &copyHandle, size, offset);
         inputIdx++;
      }
      if ( copies[i].isOutput() ) {
         xdmaAddDataCopy(&task, outputIdx, XDMA_GLOBAL, XDMA_FROM_DEVICE, &copyHandle,
               size, offset);
         outputIdx++;
      }

   }
#ifdef NANOS_INSTRUMENTATION_ENABLED
    dmaSubmitStart( this, &wd );
#endif
   if (xdmaSendTask(_fpgaProcessorInfo->getDeviceHandle(), &task) != XDMA_SUCCESS) {
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
         Instrumentation::DeviceEvent( counters->start, TaskBegin, _devInstr, wd ) );
   //Beginning kernel execution is represented as a task switch from NULL to a WD
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, _devInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, _devInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, _devInstr, wd ) );

  //DMA info
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, _dmaInInstr, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskSwitch, _dmaInInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, _dmaInInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, _dmaInInstr, wd ) );

   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, _dmaOutInstr, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, _dmaOutInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskSwitch, _dmaOutInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, _dmaOutInstr, wd ) );

   xdmaClearTaskTimes( (xdma_instr_times*) counters );
   //_hwInstrCounters.erase( wd );

}

#if 0
void FPGAThread::setupTaskInstrumentation( WD *wd ) {
   //Set up HW instrumentation
   FPGAProcessor *fpga = ( FPGAProcessor* ) this->runningOn();
   const xdma_device deviceHandle =
      fpga->getFPGAProcessorInfo()->getDeviceHandle();

   //Instrument instrumentation setup
   Instrumentation *instr = sys.getInstrumentation();
   DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
   unsigned long long timestamp;
   xdmaGetDeviceTime( &timestamp );
   instr->addDeviceEvent(
           Instrumentation::DeviceEvent( timestamp, TaskBegin, submitInstr, wd ) );
   instr->addDeviceEvent(
           Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, NULL, wd) );

   xdma_instr_times * hwCounters;
   xdmaSetupTaskInstrument(deviceHandle, &hwCounters);
   hwCounters->outTransfer = 1;
   //_hwInstrCounters[ wd ] = hwCounters;

   timestamp = submitInstr->getDeviceTime();
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, wd, NULL) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskEnd, submitInstr, wd) );
}

void FPGAThread::submitInstrSync( WD *wd ) {
   FPGAProcessor *fpga = ( FPGAProcessor* ) this->runningOn();
   const xdma_device deviceHandle =
      fpga->getFPGAProcessorInfo()->getDeviceHandle();
   const xdma_channel oChan = fpga->getFPGAProcessorInfo()->getOutputChannel();
   xdma_transfer_handle handle;
   xdmaSubmitKBuffer( _syncHandle, sizeof( unsigned long long int ), 0, XDMA_ASYNC,
         deviceHandle, oChan, &handle );
   _instrSyncHandles[ wd ] = handle;
}
#endif
#endif

void FPGAProcessor::waitAndFinishTask( FPGATaskInfo_t & task ) {
   //Scheduler::postOutlineWork( wd, false, this );
   //Wait for the task to finish
   xdmaWaitTask( task._handle );

#ifdef NANOS_INSTRUMENTATION_ENABLED
   readInstrCounters( task );
#endif
   xdmaDeleteTask( &(task._handle) );
   //FPGAWorker::postOutlineWork( task._wd );
   Scheduler::postOutlineWork( task._wd, false, myThread );
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

   /*
    * NOTE: We have to call submitOutputCopies before pushing the task in the queue
    *       but matbe this is not the best place to do this acction.
    */
   //wd.submitOutputCopies();
   _pendingTasks.push( FPGATaskInfo_t( &wd, handle ) );
}
