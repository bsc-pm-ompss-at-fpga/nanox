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

#include "fpgalistener.hpp"
#include "libxtasks_wrapper.hpp"
#include "queue.hpp"

using namespace nanos;
using namespace ext;

void FPGAListener::callback( BaseThread* self )
{
   static int maxThreads = FPGAConfig::getMaxThreadsIdleCallback();

   /*!
    * Try to atomically reserve an slot
    * NOTE: The order of if statements cannot be reversed as _count must be always increased to keep
    *       the value coherent after the descrease.
    */
   if ( _count.fetchAndAdd() < maxThreads || maxThreads == -1 ) {
      NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-listener", "outline" ) );

      PE * const selfPE = self->runningOn();
      WD * const selfWD = self->getCurrentWD();
      FPGAProcessor * const fpga = getFPGAProcessor();
      //verbose("FPGAListener::callback\t Thread " << self->getId() << " gets work for FPGA-PE (" << fpga << ")");

      //Simulate that the SMP thread runs on the FPGA PE
      self->setRunningOn( fpga );

      FPGAWorker::tryOutlineTask( self );

      //Restore the running PE of SMP Thread and the running WD (just in case)
      self->setCurrentWD( *selfWD );
      self->setRunningOn( selfPE );
   }
   --_count;
}

void FPGACreateWDListener::callback( BaseThread* self )
{
   static int maxThreads = 1; //NOTE: The task creation order for the same accelerator must be ensured

   /*!
    * Try to atomically reserve an slot
    * NOTE: The order of if statements cannot be reversed as _count must be always increased to keep
    *       the value coherent after the descrease.
    */
   if ( _count.fetchAndAdd() < maxThreads || maxThreads == -1 ) {
      NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-listener", "create-wd" ) );
      FPGAWorker::handleFPGACreatedTasks();
   }
   --_count;
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAInstrumentationListener::callback( BaseThread* self )
{
   static int maxThreads = 1; //< Should be enough, throughput is not important here

   /*!
    * Try to atomically reserve an slot
    */
   if ( _count.fetchAndAdd() < maxThreads && _fpgas != NULL) {
      NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-listener", "instrumentation" ) );

      for ( FPGAPEsVector::iterator it = _fpgas->begin(); it != _fpgas->end(); it++ ) {
         FPGAProcessor * fpga = *it;
         fpga->handleInstrumentation();
      }
   }
   --_count;
}
#endif //NANOS_INSTRUMENTATION_ENABLED
