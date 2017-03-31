/*************************************************************************************/
/*      Copyright 2015 Barcelona Supercomputing Center                               */
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

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "debug.hpp"
#include "instrumentationmodule_decl.hpp"
#include "instrumentation.hpp"

#include "os.hpp"
#include "pthread.hpp"

#include "basethread.hpp"
#include "schedule.hpp"

#include "smpprocessor.hpp"

#include "system.hpp"

//#include "clusterdevice_decl.hpp"

using namespace nanos;
using namespace nanos::ext;

SMPThread & SMPThread::stackSize( size_t size )
{
   _pthread.setStackSize( size );
   return *this;
}

void SMPThread::runDependent ()
{
   WD &work = getThreadWD();
   setCurrentWD( work );

   SMPDD &dd = ( SMPDD & ) work.activateDevice( getSMPDevice() );

   dd.execute( work );
}

void SMPThread::idle( bool debug )
{
   processTransfers();
   sys.getEventDispatcher().atIdle();
}

void SMPThread::wait()
{
#ifdef NANOS_INSTRUMENTATION_ENABLED
   static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary();
   static nanos_event_key_t cpuid_key = ID->getEventKey("cpuid");
   nanos_event_value_t cpuid_value = (nanos_event_value_t) 0;
   sys.getInstrumentation()->raisePointEvents(1, &cpuid_key, &cpuid_value);
#endif

   _pthread.mutexLock();

   if ( isSleeping() && !hasNextWD() && canBlock() ) {

      /* Only leave team if it's been told to */
      ThreadTeam *team = getTeam() ? getTeam() : getNextTeam();
      if ( team && isLeavingTeam() ) {
         leaveTeam();
      }

      /* Set flag */
      BaseThread::wait();

      NANOS_INSTRUMENT( InstrumentState state_stop(NANOS_STOPPED) );

      /* It is recommended to wait under a while loop to handle spurious wakeups
       * http://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_cond_wait.html
       */
      while ( isSleeping() ) {
         _pthread.condWait();
      }

      NANOS_INSTRUMENT( InstrumentState state_wake(NANOS_WAKINGUP) );

      /* Unset flag */
      BaseThread::resume();

#ifdef NANOS_INSTRUMENTATION_ENABLED
      if ( sys.getSMPPlugin()->getBinding() ) {
         cpuid_value = (nanos_event_value_t) getCpuId() + 1;
      } else if ( sys.isCpuidEventEnabled() ) {
         cpuid_value = (nanos_event_value_t) sched_getcpu() + 1;
      }
      sys.getInstrumentation()->raisePointEvents(1, &cpuid_key, &cpuid_value);
#endif

      if ( getTeam() == NULL ) {
         team = getNextTeam();
         if ( team ) {
            ensure( sys.getPMInterface().isMalleable(),
                  "Only malleable prog. models should dynamically acquire a team" );
            reserve();
            sys.acquireWorker( team, this, true, false, false );
         }
      }
   }
   _pthread.mutexUnlock();

   /* Whether the thread should wait for the cpu to be free before doing some work */
   sys.getThreadManager()->waitForCpuAvailability();
   sys.getThreadManager()->returnMyCpuIfClaimed();

}

void SMPThread::wakeup()
{
   BaseThread::wakeup();
   _pthread.condSignal();
}

int SMPThread::getCpuId() const {
   return _pthread.getCpuId();
}

void SMPThread::processTransfers ()
{
   BaseThread::processTransfers();
   getSMPDevice().tryExecuteTransfer();
}

SMPMultiThread::SMPMultiThread( WD &w, SMPProcessor *pe,
      unsigned int representingPEsCount, PE **representingPEs ) :
   SMPThread ( w, pe, pe ),
   _current( 0 ),
   _totalThreads( representingPEsCount ) {
   setCurrentWD( w );
   if ( representingPEsCount > 0 ) {
      addThreadsFromPEs( representingPEsCount, representingPEs );
   }
}

void SMPMultiThread::addThreadsFromPEs(unsigned int representingPEsCount, PE **representingPEs)
{
   _threads.reserve( representingPEsCount );
   for ( unsigned int i = 0; i < representingPEsCount; i++ )
   {
      _threads[ i ] = &( representingPEs[ i ]->startWorker( this ) );
   }
   _totalThreads = representingPEsCount;
}

void SMPMultiThread::initializeDependent( void ) {

   BaseThread *tmpMyThread = myThread;
   for ( unsigned int i = 0; i<_totalThreads; i++ ) {
      //Change myThread so calls to myThread->... or getMythreadSafe()->...
      //    work as expected and do not try call parent multithread (this)
      myThread = _threads[ i ];
      myThread->initializeDependent();
   }
   myThread = tmpMyThread;
}
