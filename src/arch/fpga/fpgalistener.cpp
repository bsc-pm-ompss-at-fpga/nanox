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

#include "fpgalistener.hpp"
#include "queue.hpp"

using namespace nanos;
using namespace ext;

//TODO: Get the value from the FPGAConfig
unsigned int FPGAListener::_maxConcurrentThreads = 1;

void FPGAListener::callback( BaseThread* self )
{
   static int const maxPendingWD = FPGAConfig::getMaxPendingWD();
   static int const finishBurst = FPGAConfig::getFinishWDBurst();
   FPGAProcessor * const fpga = getFPGAProcessor();
   ensure( fpga != NULL, "FPGAProcessor pointer in the FPGAListener cannot be NULL" );

   // Read the _count value
   if ( _count >= _maxConcurrentThreads ) {
      fpga->finishPendingWD( finishBurst );
      return;
   }
   // Try to atomically reserve an slot
   if ( _count.fetchAndAdd() < _maxConcurrentThreads ) {
      PE * const selfPE = self->runningOn();
      WD * const selfWD = self->getCurrentWD();
      //verbose("FPGAListener::callback\t Thread " << self->getId() << " gets work for FPGA-PE (" << fpga << ")");

      //Simulate that the SMP thread runs on the FPGA PE
      self->setRunningOn( fpga );

      bool wdExecuted;
      do {
         WD * wd;
         wdExecuted = false;

         if ( fpga->getPendingWDs() > maxPendingWD ) {
            fpga->finishPendingWD( finishBurst );
         }

         if ( !self->isRunning() ) break;

         // Check queue of tasks waiting for input copies
         if ( !fpga->getWaitInTasks().empty() ) {
            if ( fpga->getWaitInTasks().try_pop( wd ) ) {
               if ( wd->isInputDataReady() ) {
                  Scheduler::outlineWork( self, wd );
                  //wd->submitOutputCopies();
                  wdExecuted = true;
               } else {
                  // Task does not have input data in the memory device yet
                  fpga->getWaitInTasks().push( wd );
               }
            }
         }
         // Check queue of tasks waiting for memory allocation
         else if ( !fpga->getReadyTasks().empty() ) {
            if ( fpga->getReadyTasks().try_pop( wd ) ) {
               if ( Scheduler::tryPreOutlineWork( wd ) ) {
                  fpga->preOutlineWorkDependent( *wd );
                  wdExecuted = true;
                  //TODO: may need to increment copies version number here
                  if ( wd->isInputDataReady() ) {
                     Scheduler::outlineWork( self, wd );
                     //wd->submitOutputCopies();
                  } else {
                     // Task does not have input data in the memory device yet
                     fpga->getWaitInTasks().push( wd );
                  }
               } else {
                  // Task does not have memory allocated yet
                  fpga->getReadyTasks().push( wd );
               }
            }
         }
         // Check for tasks in the scheduler ready queue
         else if ( (wd = FPGAWorker::getFPGAWD( self )) != NULL ) {
            Scheduler::prePreOutlineWork( wd );
            wdExecuted = true;
            if ( Scheduler::tryPreOutlineWork( wd ) ) {
               fpga->preOutlineWorkDependent( *wd );

               //TODO: may need to increment copies version number here
               if ( wd->isInputDataReady() ) {
                  Scheduler::outlineWork( self, wd );
                  //wd->submitOutputCopies();
               } else {
                  // Task does not have input data in the memory device yet
                  fpga->getWaitInTasks().push( wd );
               }
            } else {
               // Task does not have memory allocated yet
               fpga->getReadyTasks().push( wd );
            }
         } else {
            //we may be waiting for the last tasks to finalize or
            //waiting for some dependence to be released
            fpga->finishPendingWD( finishBurst );
            //myThread->finishPendingWD(1);
         }
      } while ( wdExecuted );

      //verbose("FPGAListener::callback\t Thread " << self->getId() << " leaves");
      //ensure(selfWD == self->getCurrentWD(), "Exiting FPGAListener::callback with a different running WD");
      //Restore the running PE of SMP Thread and the running WD (just in case)
      self->setCurrentWD( *selfWD );
      self->setRunningOn( selfPE );
   }
   --_count;
}
