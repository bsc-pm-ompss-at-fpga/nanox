/*************************************************************************************/
/*      Copyright 2017-2019 Barcelona Supercomputing Center                          */
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
#include "fpgaprocessor.hpp"
#include "schedule.hpp"
#include "instrumentation.hpp"
#include "system.hpp"
#include "os.hpp"
#include "queue.hpp"

using namespace nanos;
using namespace ext;

bool FPGAWorker::tryOutlineTask( BaseThread * thread ) {
   static int const maxPendingWD = FPGAConfig::getMaxPendingWD();
   static int const finishBurst = FPGAConfig::getFinishWDBurst();

   FPGAProcessor * fpga = ( FPGAProcessor * )( thread->runningOn() );
   WD * oldWd = thread->getCurrentWD();
   WD * wd;
   bool ret = false;

   //check if we have reached maximum pending WD
   //  finalize one (or some of them)
   if ( fpga->getPendingWDs() >= maxPendingWD ) {
      fpga->tryPostOutlineTasks( finishBurst );
      if ( fpga->getPendingWDs() >= maxPendingWD ) {
         return ret;
      }
   }

   // Check queue of tasks waiting for input copies
   if ( !fpga->getWaitInTasks().empty() && fpga->getWaitInTasks().try_pop( wd ) ) {
      goto TEST_IN_READY;
   }
   // Check queue of tasks waiting for memory allocation
   else if ( !fpga->getReadyTasks().empty() && fpga->getReadyTasks().try_pop( wd ) ) {
      goto TEST_PRE_OUTLINE;
   }
   // Check for tasks in the scheduler ready queue
   else if ( (wd = FPGAWorker::getFPGAWD( thread )) != NULL ) {
      ret = true;
      Scheduler::prePreOutlineWork( wd );
      TEST_PRE_OUTLINE:
      if ( Scheduler::tryPreOutlineWork( wd ) ) {
         fpga->preOutlineWorkDependent( *wd );

         //TODO: may need to increment copies version number here
         TEST_IN_READY:
         if ( wd->isInputDataReady() ) {
            ret = true;
            Scheduler::outlineWork( thread, wd );
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
      fpga->tryPostOutlineTasks();
   }
   thread->setCurrentWD( *oldWd );
   return ret;
}

void FPGAWorker::FPGAWorkerLoop() {
   BaseThread *parent = getMyThreadSafe();
   const int init_spins = ( ( SMPMultiThread* ) parent )->getNumThreads();
   const bool use_yield = false;
   unsigned int spins = init_spins;

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
   BaseThread *currentThread = myThread;
   for (;;){
      if ( !parent->isRunning() ) break;

      if ( tryOutlineTask( currentThread ) ) {
         //update instrumentation values & rise event
         NANOS_INSTRUMENT ( nanos_event_value_t values[numEvents]; )
         NANOS_INSTRUMENT ( total_spins+= (init_spins - spins); )
         NANOS_INSTRUMENT ( values[0] = (nanos_event_value_t) total_yields; )
         NANOS_INSTRUMENT ( values[1] = (nanos_event_value_t) time_yields; )
         NANOS_INSTRUMENT ( values[2] = (nanos_event_value_t) total_spins; )
         NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(numEvents, keys, values); )

         spins = init_spins;

         //Reset instrumentation values
         NANOS_INSTRUMENT ( total_yields = 0; )
         NANOS_INSTRUMENT ( time_yields = 0; )
         NANOS_INSTRUMENT ( total_spins = 0; )
      } else {
         spins--;
      }

      if ( spins == 0 ) {

         NANOS_INSTRUMENT ( total_spins += init_spins; )

         spins = init_spins;
         if ( FPGAConfig::getHybridWorkerEnabled() ) {
            //When spins go to 0 it means that there is no work for any fpga accelerator
            // -> get an SMP task
            BaseThread *tmpThread = myThread;
            myThread = parent; //Parent should be already an smp thread
            Scheduler::helperWorkerLoop();
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

#ifdef NANOS_INSTRUMENTATION_ENABLED
         SMPMultiThread *parentM = ( SMPMultiThread * ) parent;
         for ( unsigned int i = 0; i < parentM->getNumThreads(); i += 1 ) {
            BaseThread *insThread = parentM->getThreadVector()[ i ];
            FPGAProcessor * insFpga = ( FPGAProcessor * )( insThread->runningOn() );
            insFpga->handleInstrumentation();
         }
#endif //NANOS_INSTRUMENTATION_ENABLED
      }

      currentThread = parent->getNextThread();
      myThread = currentThread;

   }
   //we may need to chech for remaining WD

   SMPMultiThread *parentM = ( SMPMultiThread * ) parent;
   for ( unsigned int i = 0; i < parentM->getNumThreads(); i += 1 ) {
      myThread = parentM->getThreadVector()[ i ];

#ifdef NANOS_INSTRUMENTATION_ENABLED
      //Handle possbile remaining instrumentation events
      FPGAProcessor * insFpga = ( FPGAProcessor * )( myThread->runningOn() );
      insFpga->handleInstrumentation();
#endif //NANOS_INSTRUMENTATION_ENABLED

      myThread->joined();
   }
   myThread = parent;
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
