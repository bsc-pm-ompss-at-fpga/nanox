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

#include "fpgaworker.hpp"
#include "schedule.hpp"
#include "fpgathread.hpp"
#include "instrumentation.hpp"
#include "system.hpp"
#include "os.hpp"

using namespace nanos;
using namespace ext;

void FPGAWorker::FPGAWorkerLoop() {
   BaseThread *parent = getMyThreadSafe();
   const int init_spins = ( ( SMPMultiThread* ) parent )->getNumThreads();
   const bool use_yield = false;
   unsigned int spins = init_spins;
   int maxPendingWD = FPGAConfig::getMaxPendingWD();
   int finishBurst = FPGAConfig::getFinishWDBurst();

   NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )

   NANOS_INSTRUMENT ( static nanos_event_key_t total_yields_key = ID->getEventKey("num-yields"); )
   NANOS_INSTRUMENT ( static nanos_event_key_t time_yields_key = ID->getEventKey("time-yields"); )
   NANOS_INSTRUMENT ( static nanos_event_key_t total_spins_key  = ID->getEventKey("num-spins"); )

   //Create an event array in order to rise all events at once

   NANOS_INSTRUMENT ( const int numEvents = 3; )
   NANOS_INSTRUMENT ( nanos_event_key_t keys[numEvents]; )

   NANOS_INSTRUMENT ( keys[0] = total_yields_key; )
   NANOS_INSTRUMENT ( keys[1] = time_yields_key; )
   NANOS_INSTRUMENT ( keys[2] = total_spins_key; )

   NANOS_INSTRUMENT ( unsigned long long total_spins = 0; )  /* Number of spins by idle phase*/
   NANOS_INSTRUMENT ( unsigned long long total_yields = 0; ) /* Number of yields by idle phase */
   NANOS_INSTRUMENT ( unsigned long long time_yields = 0; ) /* Time of yields by idle phase */

   myThread = parent->getNextThread();
   FPGAThread *currentThread = ( FPGAThread* )myThread;
   for (;;){
      //check if we have reached maximum pending WD
      //  finalize one (or some of them)
      //FPGAThread *myThread = (FPGAThread*)getMyThreadSafe();
      if (currentThread->_lock.tryAcquire()) {
         if ( currentThread->getPendingWDs() > maxPendingWD ) {
             currentThread->finishPendingWD( finishBurst );
         }

         if ( !parent->isRunning() ) {
            currentThread->_lock.release();
            break;
         }
         //get next WD
         WD *wd = getFPGAWD(currentThread);
         if ( wd ) {
            //update instrumentation values & rise event
            NANOS_INSTRUMENT ( nanos_event_value_t values[numEvents]; )
            NANOS_INSTRUMENT ( total_spins+= (init_spins - spins); )
            NANOS_INSTRUMENT ( values[0] = (nanos_event_value_t) total_yields; )
            NANOS_INSTRUMENT ( values[1] = (nanos_event_value_t) time_yields; )
            NANOS_INSTRUMENT ( values[2] = (nanos_event_value_t) total_spins; )
            NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(numEvents, keys, values); )

            Scheduler::prePreOutlineWork(wd);
            if ( Scheduler::tryPreOutlineWork(wd) ) {
               currentThread->runningOn()->preOutlineWorkDependent( *wd );
            }
            //TODO: may need to increment copies version number here
            if ( wd->isInputDataReady() ) {
               Scheduler::outlineWork( currentThread, wd );
            } else {
               //do whatever is needed if input is not ready
               //wait or whatever, for instance, sync needed copies
            }
            //add to the list of pending WD
            wd->submitOutputCopies();
            currentThread->addPendingWD( wd );
            spins = init_spins;

            //Scheduler::postOutlineWork( wd, false, myThread ); <--moved to fpga thread

            //Reset instrumentation values
            NANOS_INSTRUMENT ( total_yields = 0; )
            NANOS_INSTRUMENT ( time_yields = 0; )
            NANOS_INSTRUMENT ( total_spins = 0; )

         } else {
            spins--;
            //we may be waiting for the last tasks to finalize or
            //waiting for some dependence to be released
            currentThread->finishAllWD();
            //myThread->finishPendingWD(1);
         }
         currentThread->_lock.release();
      }

      if ( spins == 0 ) {

         NANOS_INSTRUMENT ( total_spins += init_spins; )

         spins = init_spins;
         if ( FPGAConfig::getHybridWorkerEnabled() ) {
             //When spins go to 0 it means that there is no work for any fpga accelerator
             // -> get an SMP task
             BaseThread *tmpThread = myThread;
             myThread = parent; //Parent should be already an smp thread
             Scheduler::workerLoop( true );
             myThread = tmpThread;
         }

         //do not limit number of yields disregard of configuration options
         if ( use_yield ) {
            NANOS_INSTRUMENT ( total_yields++; )
            NANOS_INSTRUMENT ( unsigned long long begin_yield = (unsigned long long) ( OS::getMonotonicTime() * 1.0e9  ); )

            currentThread->yield();

            NANOS_INSTRUMENT ( unsigned long long end_yield = (unsigned long long) ( OS::getMonotonicTime() * 1.0e9 ); );
            NANOS_INSTRUMENT ( time_yields += ( end_yield - begin_yield ); );

            spins = init_spins;
         } else {
             //idle if we do not yield
             currentThread->idle(false);
         }
      }

      currentThread = ( FPGAThread * )parent->getNextThread();
      myThread = currentThread;

   }
   //we may need to chech for remaining WD

   //Let the threads exit the team

   SMPMultiThread *parentM = ( SMPMultiThread * ) parent;
   for ( unsigned int i = 0; i < parentM->getNumThreads(); i += 1 ) {
      myThread = parentM->getThreadVector()[ i ];

      // Acquire the lock and leave it acquired to avoid any further access
      currentThread = ( FPGAThread * )myThread;
      currentThread->_lock.acquire();

      myThread->leaveTeam();
      myThread->joined();
   }
   myThread = parent;
   //let the parent thread leave the team
   myThread->leaveTeam();
   myThread->joined();
}

WD * FPGAWorker::getFPGAWD(BaseThread *thread) {
   WD* wd = NULL;
   if ( thread->getTeam() != NULL ) {
      wd = thread->getNextWD();
      if ( !wd ) {
         wd = thread->getTeam()->getSchedulePolicy().atIdle ( thread, 0 );
      }
   }
   return wd;
}

void FPGAWorker::postOutlineWork( WD *wd ) {
   wd->waitOutputCopies();
   //NOTE: Scheduler::updateExitStats is called inside Scheduler::finishWork
   //Scheduler::updateExitStats (*wd);
   Scheduler::finishWork(wd, true);
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch(wd, NULL, true) );
}
