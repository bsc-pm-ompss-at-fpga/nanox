/*************************************************************************************/
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

#include "schedule.hpp"
#include "processingelement.hpp"
#include "basethread.hpp"
#include "system.hpp"
#include "config.hpp"
#include "instrumentationmodule_decl.hpp"

using namespace nanos;

void SchedulerConf::config (Config &config)
{
   config.setOptionsSection ( "Core [Scheduler]", "Policy independent scheduler options"  );

   config.registerConfigOption ( "num_spins", new Config::UintVar( _numSpins ), "Determines the amount of spinning before yielding" );
   config.registerArgOption ( "num_spins", "spins" );
   config.registerEnvOption ( "num_spins", "NX_SPINS" );
}

void Scheduler::submit ( WD &wd )
{
   NANOS_INSTRUMENT( InstrumentState inst(NANOS_SCHEDULING) );
   BaseThread *mythread = myThread;

   sys.getSchedulerStats()._createdTasks++;
   sys.getSchedulerStats()._totalTasks++;
   sys.getSchedulerStats()._readyTasks++;

   debug ( "submitting task " << wd.getId() );

   /* handle tied tasks */
   if ( wd.isTied() && wd.isTiedTo() != mythread ) {
      mythread->getTeam()->getSchedulePolicy().queue(wd.isTiedTo(), wd);
      return;
   }

   if ( !wd.canRunIn(*mythread->runningOn()) ) {
      queue(wd);
      return;
   }

   WD *next = mythread->getTeam()->getSchedulePolicy().atSubmit( myThread, wd );

   if ( next ) {
      WD *slice;
      /* enqueue the remaining part of a WD */
      if ( !next->dequeue(&slice) ) queue(*next);

      switchTo ( slice );
   }
}

void Scheduler::updateExitStats ( void )
{
   sys.getSchedulerStats()._totalTasks--;
}

template<class behaviour>
inline void Scheduler::idleLoop ()
{
   NANOS_INSTRUMENT( InstrumentState inst(NANOS_IDLE) );

   const int nspins = sys.getSchedulerConf().getNumSpins();
   int spins = nspins;

   WD *current = myThread->getCurrentWD();
   current->setIdle();
   sys.getSchedulerStats()._idleThreads++;
   for ( ; ; ) {
      BaseThread *thread = getMyThreadSafe();

      if ( !thread->isRunning() ) break;

      if ( thread->getTeam() != NULL ) {
         WD * next = myThread->getNextWD();

         //if ( !next && sys.getSchedulerStats()._readyTasks > 0 ) { // FIXME (#289)
         if ( !next ) {
            NANOS_INSTRUMENT( InstrumentState inst1(NANOS_SCHEDULING) );
            next = behaviour::getWD(thread,current);
            NANOS_INSTRUMENT( inst1.close() );
         }

         if (next) {
            sys.getSchedulerStats()._readyTasks--;
            sys.getSchedulerStats()._idleThreads--;
            NANOS_INSTRUMENT( InstrumentState inst2(NANOS_RUNTIME) );
            behaviour::switchWD(thread,current, next);
            NANOS_INSTRUMENT( inst2.close() );
            sys.getSchedulerStats()._idleThreads++;
            spins = nspins;
            continue;
         }
      }

      spins--;
      if ( spins == 0 ) {
        NANOS_INSTRUMENT( InstrumentState inst3(NANOS_YIELD) );
        thread->yield();
        NANOS_INSTRUMENT( inst3.close() );
        spins = nspins;
      }
      else {
         thread->idle();
      }
   }
   sys.getSchedulerStats()._idleThreads--;
   current->setReady();
}

void Scheduler::waitOnCondition (GenericSyncCond *condition)
{
   NANOS_INSTRUMENT( InstrumentState inst(NANOS_SYNCHRONIZATION) );

   const int nspins = sys.getSchedulerConf().getNumSpins();
   int spins = nspins; 

   WD * current = myThread->getCurrentWD();

   sys.getSchedulerStats()._idleThreads++;
   current->setSyncCond( condition );
   current->setIdle();
   
   while ( !condition->check() ) {
      BaseThread *thread = getMyThreadSafe();
      
      spins--;
      if ( spins == 0 ) {
         condition->lock();
         if ( !( condition->check() ) ) {
            condition->addWaiter( current );

            NANOS_INSTRUMENT( InstrumentState inst1(NANOS_SCHEDULING) );
            WD *next = thread->getTeam()->getSchedulePolicy().atBlock( thread, current );
            NANOS_INSTRUMENT( inst1.close() );

            if ( next ) {
               sys.getSchedulerStats()._readyTasks--;
               sys.getSchedulerStats()._idleThreads--;
               NANOS_INSTRUMENT( InstrumentState inst2(NANOS_RUNTIME) );
               switchTo ( next );
               NANOS_INSTRUMENT( inst2.close() );
               sys.getSchedulerStats()._idleThreads++;
            }
            else {
               condition->unlock();
               NANOS_INSTRUMENT( InstrumentState inst3(NANOS_YIELD) );
               thread->yield();
               NANOS_INSTRUMENT( inst3.close() );
            }
         } else {
            condition->unlock();
         }
         spins = nspins;
      }

      thread->idle();
   }

   current->setSyncCond( NULL );
   sys.getSchedulerStats()._idleThreads--;
   if ( !current->isReady() ) {
      sys.getSchedulerStats()._readyTasks++;
      current->setReady();
   }
}

void Scheduler::wakeUp ( WD *wd )
{
   NANOS_INSTRUMENT( InstrumentState inst(NANOS_SYNCHRONIZATION) );
   if ( wd->isBlocked() ) {
      sys.getSchedulerStats()._readyTasks++;
      wd->setReady();
      Scheduler::queue( *wd );
   }
}

WD * Scheduler::prefetch( BaseThread *thread, WD &wd )
{
   debug ( "prefetching data for task " << wd.getId() );
   return thread->getTeam()->getSchedulePolicy().atPrefetch( thread, wd );
}

struct WorkerBehaviour
{
   static WD * getWD ( BaseThread *thread, WD *current )
   {
      return thread->getTeam()->getSchedulePolicy().atIdle ( thread );
   }

   static void switchWD ( BaseThread *thread, WD *current, WD *next )
   {
      if (next->started())
        Scheduler::switchTo(next);
      else {
        Scheduler::inlineWork ( next );
        Scheduler::updateExitStats ();
      }
   }
};

void Scheduler::workerLoop ()
{
   idleLoop<WorkerBehaviour>();
}

void Scheduler::queue ( WD &wd )
{
      myThread->getTeam()->getSchedulePolicy().queue( myThread, wd );
}

void Scheduler::inlineWork ( WD *wd )
{
   // run it in the current frame
   WD *oldwd = myThread->getCurrentWD();

   GenericSyncCond *syncCond = oldwd->getSyncCond();
   if ( syncCond != NULL ) {
      syncCond->unlock();
   }

   debug( "switching(inlined) from task " << oldwd << ":" << oldwd->getId() <<
          " to " << wd << ":" << wd->getId() );

   // NANOS_INSTRUMENT( sys.getInstrumentation()->wdLeaveCPU(oldwd) ); FIXME
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch(oldwd, NULL, false) );

   // This ensures that when we return from the inlining is still the same thread
   // and we don't violate rules about tied WD
   wd->tieTo(*oldwd->isTiedTo());
   if (!wd->started())
      wd->start(false);
   myThread->setCurrentWD( *wd );

   //NANOS_INSTRUMENT( sys.getInstrumentation()->wdEnterCPU(wd) ); FIXME
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch( NULL, wd, false) );

   myThread->inlineWorkDependent(*wd);
   wd->done();

   //NANOS_INSTRUMENT( sys.getInstrumentation()->wdLeaveCPU(wd) ); FIXME
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch(wd, NULL, false) );


   debug( "exiting task(inlined) " << wd << ":" << wd->getId() <<
          " to " << oldwd << ":" << oldwd->getId() );


   BaseThread *thread = getMyThreadSafe();
   thread->setCurrentWD( *oldwd );

   //NANOS_INSTRUMENT( sys.getInstrumentation()->wdEnterCPU(oldwd) ); FIXME
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch( NULL, oldwd, false) );

   // While we tie the inlined tasks this is not needed
   // as we will always return to the current thread
   #if 0
   if ( oldwd->isTiedTo() != NULL )
      switchToThread(oldwd->isTiedTo());
   #endif

   ensure(oldwd->isTiedTo() == NULL || thread == oldwd->isTiedTo(), 
          "Violating tied rules " + toString<BaseThread*>(thread) + "!=" + toString<BaseThread*>(oldwd->isTiedTo()));

}

void Scheduler::switchHelper (WD *oldWD, WD *newWD, void *arg)
{
   GenericSyncCond *syncCond = oldWD->getSyncCond();
   if ( syncCond != NULL ) {
      oldWD->setBlocked();
      syncCond->unlock();
   } else {
      Scheduler::queue( *oldWD );
   }

   //NANOS_INSTRUMENT( sys.getInstrumentation()->wdLeaveCPU(oldWD) ); FIXME
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch(oldWD, NULL, false) );
   myThread->switchHelperDependent(oldWD, newWD, arg);

   myThread->setCurrentWD( *newWD );
   //NANOS_INSTRUMENT( sys.getInstrumentation()->wdEnterCPU(newWD) ); FIXME
   NANOS_INSTRUMENT( sys.getInstrumentation()->wdSwitch( NULL, newWD, false) );
}

void Scheduler::switchTo ( WD *to )
{
   if ( myThread->runningOn()->supportsUserLevelThreads() ) {
      if (!to->started())
         to->start(true);
      
      debug( "switching from task " << myThread->getCurrentWD() << ":" << myThread->getCurrentWD()->getId() <<
          " to " << to << ":" << to->getId() );
          
      myThread->switchTo( to, switchHelper );
   } else {
      inlineWork(to);
      delete to;
   }
}

void Scheduler::yield ()
{
   NANOS_INSTRUMENT( InstrumentState inst(NANOS_SCHEDULING) );
   WD *next = myThread->getTeam()->getSchedulePolicy().atYield( myThread, myThread->getCurrentWD() );

   if ( next ) {
      switchTo(next);
   }
}

void Scheduler::switchToThread ( BaseThread *thread )
{
   while ( getMyThreadSafe() != thread )
        yield();
}

void Scheduler::exitHelper (WD *oldWD, WD *newWD, void *arg)
{
    myThread->exitHelperDependent(oldWD, newWD, arg);
    NANOS_INSTRUMENT ( sys.getInstrumentation()->wdSwitch(oldWD,newWD,true) );
    delete oldWD;
    myThread->setCurrentWD( *newWD );
}

struct ExitBehaviour
{
   static WD * getWD ( BaseThread *thread, WD *current )
   {
      return thread->getTeam()->getSchedulePolicy().atExit( thread, current );
   }

   static void switchWD ( BaseThread *thread, WD *current, WD *next )
   {
      Scheduler::exitTo(next);
   }
};

void Scheduler::exitTo ( WD *to )
 {
    WD *current = myThread->getCurrentWD();

    if (!to->started()) to->start(true,current);

    debug( "exiting task " << myThread->getCurrentWD() << ":" << myThread->getCurrentWD()->getId() <<
          " to " << to << ":" << to->getId() );

    myThread->exitTo ( to, Scheduler::exitHelper );
}

void Scheduler::exit ( void )
{
   // At this point the WD work is done, so we mark it as such and look for other work to do
   // Deallocation doesn't happen here because:
   // a) We are still running in the WD stack
   // b) Resources can potentially be reused by the next WD

   updateExitStats ();

   WD *oldwd = myThread->getCurrentWD();
   oldwd->done();
   oldwd->clear();

   idleLoop<ExitBehaviour>();

   fatal("A thread should never return from Scheduler::exit");
}
