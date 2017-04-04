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

FPGAThread::FPGAThread( WD &wd, PE *pe, SMPMultiThread *parent ) :
   BaseThread( ( unsigned int ) -1, wd, pe, parent),
   _lock()/*, _hwInstrCounters()*/ {
      setCurrentWD( wd );
   }

void FPGAThread::initializeDependent()
{
   //initialize instrumentation
   //xdmaInitHWInstrumentation();
   //jbosch: Disabling the previous call because it is called several times (SMPMultiWorker)
   //        Moving the call after the xdmaInit

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
   //NOTE: Cleanup is done in the FPGA plugin (maybe PE is not running any thread)
   //( ( FPGAProcessor * ) this->runningOn() )->cleanUp();
}

void FPGAThread::yield() {
   verbose("FPGA yield");
#if 0
   //Synchronizing transfers here seems to yield slightly better performance
   ((FPGAProcessor*)runningOn())->getOutTransferList()->syncAll();
   ((FPGAProcessor*)runningOn())->getInTransferList()->syncAll();
#endif
   static int const finishBurst = FPGAConfig::getFinishWDBurst();
   ((FPGAProcessor*)runningOn())->finishPendingWD( finishBurst );
}

//Sync transfers on idle
void FPGAThread::idle( bool debug ) {
#if 0
    //TODO:get the number of transfers from config
    //TODO: figure put a sensible default
    int n = FPGAConfig::getIdleSyncBurst();
    ((FPGAProcessor*)runningOn())->getOutTransferList()->syncNTransfers(n);
    ((FPGAProcessor*)runningOn())->getInTransferList()->syncNTransfers(n);
#endif
   static int const finishBurst = FPGAConfig::getFinishWDBurst();
   ((FPGAProcessor*)runningOn())->finishPendingWD( finishBurst );
}

void FPGAThread::switchToNextThread() {}

BaseThread *FPGAThread::getNextThread() {
   if ( getParent() != NULL ) {
      return getParent()->getNextThread();
   } else {
      return this;
   }
}
