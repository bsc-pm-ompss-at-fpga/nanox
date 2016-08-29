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

#include "fpgathread.hpp"
#include "fpgadd.hpp"
#include "fpgamemorytransfer.hpp"
#include "fpgaworker.hpp"
#include "fpgaprocessorinfo.hpp"
#include "instrumentation_decl.hpp"

using namespace nanos;
using namespace nanos::ext;

FPGAThread::FPGAThread(WD &wd, PE *pe, SMPMultiThread *parent, Atomic<int> fpgaDevice) :
   BaseThread( ( unsigned int ) -1, wd, pe, parent),
   _pendingWD(), _hwInstrCounters() {
      setCurrentWD( wd );
   }

void FPGAThread::initializeDependent()
{
   //initialize device
   ( ( FPGAProcessor * ) myThread->runningOn() )->init();
   //initialize instrumentation
   xdmaInitHWInstrumentation();
   //allocate sync buffer to ensure instrumentation data is ready

   xdmaAllocateKernelBuffer( ( void ** )&_syncBuffer, &_syncHandle, sizeof( unsigned int ) );
}


void FPGAThread::runDependent()
{
   verbose( "fpga run dependent" );
   WD &work = getThreadWD();
   setCurrentWD( work );
   SMPDD &dd = ( SMPDD & ) work.activateDevice( getSMPDevice() );
   dd.getWorkFct()( work.getData() );
   ( ( FPGAProcessor * ) myThread->runningOn() )->cleanUp();
}

bool FPGAThread::inlineWorkDependent( WD &wd )
{
   verbose( "fpga nlineWorkDependent" );

   wd.start( WD::IsNotAUserLevelThread );
   //FPGAProcessor* fpga = ( FPGAProcessor * ) myThread->runningOn();

   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );

   return true;
}

void FPGAThread::preOutlineWorkDependent ( WD &wd ) {
   wd.preStart(WorkDescriptor::IsNotAUserLevelThread);
}

void FPGAThread::outlineWorkDependent ( WD &wd ) {
   //wd.start( WD::IsNotAUserLevelThread );
   FPGAProcessor* fpga = ( FPGAProcessor * ) myThread->runningOn();

   fpga->createAndSubmitTask( wd );

   //set flag to allow new opdate
   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );
}

void FPGAThread::yield() {
   verbose("FPGA yield");

   //Synchronizing transfers here seems to yield slightly better performance
   ((FPGAProcessor*)runningOn())->getOutTransferList()->syncAll();
   ((FPGAProcessor*)runningOn())->getInTransferList()->syncAll();
}

//Sync transfers on idle
void FPGAThread::idle( bool debug ) {
    //TODO:get the number of transfers from config
    //TODO: figure put a sensible default
    int n = FPGAConfig::getIdleSyncBurst();
    ((FPGAProcessor*)runningOn())->getOutTransferList()->syncNTransfers(n);
    ((FPGAProcessor*)runningOn())->getInTransferList()->syncNTransfers(n);
}

int FPGAThread::getPendingWDs() const {
   return _pendingWD.size();
}

void FPGAThread::finishPendingWD( int numWD ) {
   int n = std::min(numWD, (int)_pendingWD.size() );
   for (int i=0; i<n; i++) {
      WD * wd = _pendingWD.front();
      //Scheduler::postOutlineWork( wd, false, this );
      //Wait for the task to finish
      FPGAProcessor *fpga = (FPGAProcessor *)runningOn();
      fpga->waitTask( wd );

#ifdef NANOS_INSTRUMENTATION_ENABLED
      readInstrCounters( wd );
#endif
      fpga->deleteTask( wd );
      FPGAWorker::postOutlineWork(wd);
      _pendingWD.pop();
   }
}

void FPGAThread::addPendingWD( WD *wd ) {
   _pendingWD.push( wd );
}

void FPGAThread::finishAllWD() {
   while( !_pendingWD.empty() ) {
      WD * wd = _pendingWD.front();
      //Scheduler::postOutlineWork( wd, false, this );
      //Retreive counter data from HW & clear entry
      //All task transfers have been finished so performance data should be ready
      FPGAProcessor *fpga = ( FPGAProcessor* )runningOn();
      fpga->waitTask( wd );
#ifdef NANOS_INSTRUMENTATION_ENABLED
      readInstrCounters( wd );
#endif
      fpga->deleteTask( wd );
      FPGAWorker::postOutlineWork(wd);
      _pendingWD.pop();
   }
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAThread::readInstrCounters( WD *wd ) {

   FPGAProcessor *fpga = ( FPGAProcessor* )runningOn();
   xdma_instr_times *counters = fpga->getInstrCounters( wd );

   Instrumentation *instr = sys.getInstrumentation();
   DeviceInstrumentation *devInstr =
      fpga->getDeviceInstrumentation();
   DeviceInstrumentation *dmaIn, *dmaOut;

   dmaIn = fpga->getDmaInInstrumentation();
   dmaOut = fpga->getDmaOutInstrumentation();

   //Task execution
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, devInstr, wd ) );
   //Beginning kernel execution is represented as a task switch from NULL to a WD
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, devInstr, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, devInstr, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, devInstr, wd ) );

  //DMA info
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, dmaIn, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskSwitch, dmaIn, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->inTransfer, TaskSwitch, dmaIn, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, dmaIn, wd ) );

   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->start, TaskBegin, dmaOut, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->computation, TaskSwitch, dmaOut, NULL, wd ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskSwitch, dmaOut, wd, NULL ) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( counters->outTransfer, TaskEnd, dmaOut, wd ) );

   xdmaClearTaskTimes( (xdma_instr_times*) counters );
   _hwInstrCounters.erase( wd );

}
#endif

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAThread::setupTaskInstrumentation( WD *wd ) {
   //Set up HW instrumentation
   FPGAProcessor *fpga = ( FPGAProcessor* ) myThread->runningOn();
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
   _hwInstrCounters[ wd ] = hwCounters;

   timestamp = submitInstr->getDeviceTime();
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, wd, NULL) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskEnd, submitInstr, wd) );
}

void FPGAThread::submitInstrSync( WD *wd ) {
   FPGAProcessor *fpga = ( FPGAProcessor* ) myThread->runningOn();
   const xdma_device deviceHandle =
      fpga->getFPGAProcessorInfo()->getDeviceHandle();
   const xdma_channel oChan = fpga->getFPGAProcessorInfo()->getOutputChannel();
   xdma_transfer_handle handle;
   xdmaSubmitKBuffer( _syncHandle, sizeof( unsigned long long int ), 0, XDMA_ASYNC,
         deviceHandle, oChan, &handle );
   _instrSyncHandles[ wd ] = handle;
}
#endif
void FPGAThread::switchTo( WD *work, SchedulerHelper *helper ) {}
void FPGAThread::exitTo( WD *work, SchedulerHelper *helper ) {}
void FPGAThread::switchHelperDependent( WD* oldWD, WD* newWD, void *arg ) {}
void FPGAThread::exitHelperDependent( WD* oldWD, WD* newWD, void *arg ) {}
void FPGAThread::switchToNextThread() {}

BaseThread *FPGAThread::getNextThread() {
   if ( getParent() != NULL ) {
      return getParent()->getNextThread();
   } else {
      return this;
   }
}
