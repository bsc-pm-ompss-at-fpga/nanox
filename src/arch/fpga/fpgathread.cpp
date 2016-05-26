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

void FPGAThread::initializeDependent()
{
   //initialize device
   ( ( FPGAProcessor * ) myThread->runningOn() )->init();
   //initialize instrumentation
   xdmaInitHWInstrumentation();

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
   FPGAProcessor* fpga = ( FPGAProcessor * ) myThread->runningOn();

   //Instrumentation event to record in which accelerator the task is running
   NANOS_INSTRUMENT (InstrumentBurst i("accelerator#", fpga->getActiveAcc()+1) );
   //set flag to allow new opdate
   fpga->setUpdate(true);
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

   //Instrumentation event to record in which accelerator the task is running
   NANOS_INSTRUMENT (InstrumentBurst i("accelerator#", fpga->getActiveAcc()+1) );
   //set flag to allow new opdate
   fpga->setUpdate(true);
   FPGADD &dd = ( FPGADD & )wd.getActiveDevice();
   ( dd.getWorkFct() )( wd.getData() );
}

void FPGAThread::yield() {
   verbose("FPGA yield");

   //Synchronizing transfers here seems to yield slightly better performance
   ((FPGAProcessor*)runningOn())->getOutTransferList()->syncAll();
   ((FPGAProcessor*)runningOn())->getInTransferList()->syncAll();

   SMPThread::yield();
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
#ifdef NANOS_INSTRUMENTATION_ENABLED
      readInstrCounters( wd );
#endif
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
#ifdef NANOS_INSTRUMENTATION_ENABLED
      readInstrCounters( wd );
#endif
      FPGAWorker::postOutlineWork(wd);
      _pendingWD.pop();
   }
}

#ifdef NANOS_INSTRUMENTATION_ENABLED

static int __attribute__((noinline)) checkCounter(volatile xdma_instr_times *counters ) {
    //std::cerr << "counter: " << counters->outTransfer << std::endl;
    return (counters->outTransfer != 1);
}

void FPGAThread::readInstrCounters( WD *wd ) {

   std::pair< xdma_instr_times*, int > &instEntry = _hwInstrCounters[ wd ];
   volatile xdma_instr_times *counters = instEntry.first;
   int activeAcc = instEntry.second;

   //TODO: Submit data to instrumentation layer

   Instrumentation *instr = sys.getInstrumentation();
   FPGAProcessor *fpga = ( FPGAProcessor* )runningOn();
   DeviceInstrumentation *devInstr =
      fpga->getDeviceInstrumentation( activeAcc );
   DeviceInstrumentation *dmaIn, *dmaOut;

   dmaIn = fpga->getDmaInInstrumentation( activeAcc );
   dmaOut = fpga->getDmaOutInstrumentation( activeAcc );
   //std::cout << "check counter... " << std::endl;
   while ( ! checkCounter(counters) );
   //std::cout << "ok" << std::endl;

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
   int activeAcc = fpga->getActiveAcc();
   const xdma_device deviceHandle =
      fpga->getFPGAProcessorInfo()[activeAcc].getDeviceHandle();
   debug( "FPGA: Setting instrumentation for acc " << fpga->getActiveAcc() );
//   const xdma_device deviceHandle =
//      ( ( FPGAProcessor * ) myThread->runningOn() )->getFPGAProcessorInfo()->getDeviceHandle();
   xdma_instr_times * hwCounters;
   xdmaSetupTaskInstrument(deviceHandle, &hwCounters);
   hwCounters->outTransfer = 1;
   _hwInstrCounters[ wd ] = std::make_pair( hwCounters, activeAcc );
}
#endif
