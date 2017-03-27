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

int FPGAProcessor::_accelSeed = 0;
Lock FPGAProcessor::_initLock;
//TODO: We should have an FPGAPinnedAllocator for each FPGAProcessor and only have one FPGAProcessor
//      with several FPGADevices
FPGAPinnedAllocator FPGAProcessor::_allocator( 1024*1024*64 );
/*
 * TODO: Support the case where each thread may manage a different number of accelerators
 */
FPGAProcessor::FPGAProcessor( const Device *arch,  memory_space_id_t memSpaceId ) :
   ProcessingElement( arch, memSpaceId, 0, 0, false, 0, false ),
   _fpgaDevice(0)
{
   _initLock.acquire();
   _accelBase = _accelSeed;
   _accelSeed++;
   _initLock.release();

   _fpgaProcessorInfo = NEW FPGAProcessorInfo;
   _inputTransfers = NEW FPGAMemoryInTransferList();
   _outputTransfers = NEW FPGAMemoryOutTransferList(*this);
}

FPGAProcessor::~FPGAProcessor(){
    delete _fpgaProcessorInfo;
    delete _outputTransfers;
    delete _inputTransfers;
}

void FPGAProcessor::init()
{
   int fpgaCount = FPGAConfig::getFPGACount();
   xdma_device *devices = NEW xdma_device[fpgaCount];
   xdma_status status;
   int copiedDevices = -1;
   status = xdmaGetDevices(fpgaCount,  devices, &copiedDevices);
   ensure(copiedDevices > _accelBase, "Number of devices from the xdma library is smaller the used index");

   xdma_channel iChan, oChan;
   _fpgaProcessorInfo->setDeviceHandle( devices[_accelBase] );

   //open input channel
   NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REQ_CHANNEL_EVENT);
   status = xdmaOpenChannel(devices[_accelBase], XDMA_TO_DEVICE, XDMA_CH_NONE, &iChan);
   NANOS_FPGA_CLOSE_RUNTIME_EVENT;

   if (status)
       warning("Error opening DMA input channel: " << status);

   debug("got input channel " << iChan );

   _fpgaProcessorInfo->setInputChannel( iChan );

   NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_REQ_CHANNEL_EVENT );
   status = xdmaOpenChannel(devices[_accelBase], XDMA_FROM_DEVICE, XDMA_CH_NONE, &oChan);
   NANOS_FPGA_CLOSE_RUNTIME_EVENT;
   if (status || !oChan)
       warning ("Error opening DMA output channel");

   debug("got output channel " << oChan );

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
   FPGAThread  &th = *NEW FPGAThread( helper, this, parent, _fpgaDevice );
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

void FPGAProcessor::createAndSubmitTask( WD &wd ) {

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

   xdmaInitTask( _accelBase, numInputs, XDMA_COMPUTE_ENABLE, numOutputs, &task );
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
      ensure(baseAddress > 0, "Trying to register an invalid FPGA data copy. The memory region is not registered in the FPGA Allocator.");
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
   _pendingTasks[&wd] = task;
}

void FPGAProcessor::waitTask( WD *wd ) {
   xdma_task_handle task;

   task = _pendingTasks[wd];
   xdmaWaitTask(task);
}

void FPGAProcessor::deleteTask( WD *wd ) {
   //XXX Delete task from task map?
   xdma_task_handle task = _pendingTasks[wd];
   xdmaDeleteTask(&task);

}

xdma_instr_times *FPGAProcessor::getInstrCounters( WD *wd ) {

   xdma_task_handle task = _pendingTasks[wd];
   xdma_instr_times *times;
   xdmaGetInstrumentData(task, &times);
   return times;
}
