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

#ifndef _ASYNC_THREAD_ELEMENT
#define _ASYNC_THREAD_ELEMENT

#include "asyncthread_decl.hpp"
#include "system.hpp"


namespace nanos {

// Macro's to instrument the code and make it cleaner
#define ASYNC_THREAD_CREATE_EVENT(x)   NANOS_INSTRUMENT( \
		sys.getInstrumentation()->raiseOpenBurstEvent ( sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey( "async-thread" ), (x) ); )

#define ASYNC_THREAD_CLOSE_EVENT       NANOS_INSTRUMENT( \
		sys.getInstrumentation()->raiseCloseBurstEvent ( sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey( "async-thread" ), 0 ); )

typedef enum {
   ASYNC_THREAD_NULL_EVENT,                  /* 0 */
   ASYNC_THREAD_INLINE_WORK_DEP_EVENT,       /* 1 */
   ASYNC_THREAD_PRE_RUN_EVENT,               /* 2 */
   ASYNC_THREAD_RUN_EVENT,                   /* 3 */
   ASYNC_THREAD_POST_RUN_EVENT,              /* 4 */
   ASYNC_THREAD_SCHEDULE_EVENT,              /* 5 */
//   ASYNC_THREAD_WAIT_INPUTS_EVENT,           /* 5 */
   ASYNC_THREAD_CHECK_WD_INPUTS_EVENT,       /* 6 */
   ASYNC_THREAD_CHECK_WD_OUTPUTS_EVENT,      /* 7 */
   ASYNC_THREAD_CP_DATA_IN_EVENT,            /* 8 */
   ASYNC_THREAD_CP_DATA_OUT_EVENT,           /* 9 */
   ASYNC_THREAD_CHECK_EVTS_EVENT,           /* 10 */
   ASYNC_THREAD_PROCESS_EVT_EVENT,          /* 11 */   /* WARNING!! Value hard-coded in asyncthread.hpp */
   ASYNC_THREAD_SYNCHRONIZE_EVENT,          /* 12 */
} AsyncThreadState_t;

inline void AsyncThread::checkEvents()
{
   _recursiveCounter++;

   // Save the event counter because the list of pending events can be increased over the loop
   // and we don't want to have an infinite loop if we keep adding events forever
   unsigned int max = _pendingEventsCounter;
   for ( unsigned int i = 0; i < max; i++ ) {
      GenericEvent * evt = _pendingEvents[i];
      if ( evt->isRaised() ) {
         NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent (
               sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey( "async-thread" ),
               /* ASYNC_THREAD_PROCESS_EVT_EVENT */ 11 ); )
         _previousWD = this->getCurrentWD();

         WD * wd = evt->getWD();
         this->setCurrentWD( *wd );
         evt->setCompleted();

         // Move to next step if WD's event is raised
         //while ( evt->hasNextAction() ) {
            //evt->processNextAction();
            //Action * action = evt->getNextAction();
            //action->run();
            //delete action;
         //}
         evt->processActions();

         // finishWork() function will modify thread's current WD because the active WD will be deleted at that point.
         this->setCurrentWD( *_previousWD );

         NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent(
               sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey( "async-thread" ), 0 ); )
      }
   }

   if ( _recursiveCounter == 1 ) {
      // Delete completed events
      while ( _pendingEventsCounter && _pendingEvents.front()->isCompleted() ) {
         _pendingEvents.erase( _pendingEvents.begin() );
         _pendingEventsCounter--;
      }
   }

   _recursiveCounter--;
}

inline void AsyncThread::checkEvents( WD * wd )
{
   // Save the event counter because the list of pending events can be increased over the loop
   // and we don't want to have an infinite loop if we keep adding events forever
   unsigned int max = _pendingEventsCounter;
   for ( unsigned int i = 0; i < max; i++ ) {
      GenericEvent * evt = _pendingEvents[i];
      if ( evt->getWD() == wd ) {
         if ( evt->isRaised() ) {
            evt->setCompleted();
            // Move to next step if WD's event is raised
            while ( evt->hasNextAction() ) {
               Action * action = evt->getNextAction();
               action->run();
               delete action;
            }
         }
      }
   }

   // WARNING: Do not delete completed events here because it can interfere with AsyncThread::checkEvents()
   // Completed events should only be erased at AsyncThread::checkEvents(), or when we are completely sure
   // that we are not in the middle of the loops of both AsyncThread::checkEvents() and
   // AsyncThread::checkEvents( WD * wd )
}

inline bool AsyncThread::processTransfers ()
{
   unsigned int localCounter = _pendingEventsCounter;
   disableGettingWork();
   checkEvents();
   enableGettingWork();
   //NOTE: Return if some useful work has been done
   return ( localCounter > _pendingEventsCounter || _pendingEventsCounter != 0 );
}

inline bool AsyncThread::canGetWork()
{
   return BaseThread::canGetWork() && ( int ) _runningWDsCounter < getMaxPrefetch();
}

} // namespace nanos

#endif //_ASYNC_THREAD_ELEMENT
