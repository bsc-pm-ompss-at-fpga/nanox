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
   sys.getEventDispatcher().atIdle();
}

void SMPThread::wait()
{
#ifdef NANOS_INSTRUMENTATION_ENABLED
   static Instrumentation *INS = sys.getInstrumentation();
   static InstrumentationDictionary *ID = INS->getInstrumentationDictionary();
   static nanos_event_key_t cpuid_key = ID->getEventKey("cpuid");
   nanos_event_value_t cpuid_value = (nanos_event_value_t) 0;
   Instrumentation::Event events[3];
   INS->createPointEvent( &events[0], cpuid_key, cpuid_value );
   INS->createStateEvent( &events[1], NANOS_STOPPED );
   INS->createStateEvent( &events[2], NANOS_WAKINGUP );
#endif

   /* Lend CPU to DLB if possible */
   sys.getThreadManager()->lendCpu(this);

   _pthread.mutexLock();

   if ( isSleeping() && !hasNextWD() && canBlock() ) {

      /* Only leave team if it's been told to */
      ThreadTeam *team = getTeam() ? getTeam() : getNextTeam();
      if ( team && isLeavingTeam() ) {
         leaveTeam();
      }

      /* Set flag */
      BaseThread::wait();

#ifdef NANOS_INSTRUMENTATION_ENABLED
      /* Add event cpuid = 0 and state STOPPED */
      INS->addEventList( 2, events );
#endif

      /* It is recommended to wait under a while loop to handle spurious wakeups
       * http://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_cond_wait.html
       */
      while ( isSleeping() ) {
         _pthread.condWait();
      }

      /* Unset flag */
      BaseThread::resume();

#ifdef NANOS_INSTRUMENTATION_ENABLED
      if ( sys.getSMPPlugin()->getBinding() ) {
         cpuid_value = (nanos_event_value_t) getCpuId() + 1;
      } else if ( sys.isCpuidEventEnabled() ) {
         cpuid_value = (nanos_event_value_t) sched_getcpu() + 1;
      }
      INS->createPointEvent( &events[0], cpuid_key, cpuid_value );
      INS->returnPreviousStateEvent( &events[1] );
      /* Add event cpuid and end of STOPPED state */
      INS->addEventList( 2, events );
      /* Add state WAKINGUP */
      INS->addEventList( 1, &events[2] );
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

   /* Wait if the CPU is not yet available, or check whether it's been reclaimed */
   sys.getThreadManager()->waitForCpuAvailability();

#ifdef NANOS_INSTRUMENTATION_ENABLED
   /* Add end of WAKINGUP state */
   INS->returnPreviousStateEvent( &events[2] );
   INS->addEventList( 1, &events[2] );
#endif
}

void SMPThread::wakeup()
{
   BaseThread::wakeup();
   _pthread.condSignal();
}

bool SMPThread::processTransfers ()
{
   bool ret = false;
   ret |= BaseThread::processTransfers();
   ret |= getSMPDevice().tryExecuteTransfer();
   return ret;
}

int SMPThread::getCpuId() const {
   return _pthread.getCpuId();
}

SMPMultiThread::SMPMultiThread( WD &w, SMPProcessor *pe,
      unsigned int representingPEsCount, PE **representingPEs ) :
   SMPThread ( w, pe, pe ),
   _current( 0 ) {
   setCurrentWD( w );
   if ( representingPEsCount > 0 ) {
      addThreadsFromPEs( representingPEsCount, representingPEs );
   }
}

void SMPMultiThread::addThreadsFromPEs(unsigned int representingPEsCount, PE **representingPEs)
{
   _threads.reserve( _threads.size() + representingPEsCount );
   for ( unsigned int i = 0; i < representingPEsCount; i++ )
   {
      _threads.push_back( &( representingPEs[ i ]->startWorker( this ) ) );
   }
}

void SMPMultiThread::initializeDependent( void )
{
   BaseThread *tmpMyThread = myThread;
   for ( unsigned int i = 0; i < _threads.size(); i++ ) {
      //Change myThread so calls to myThread->... or getMythreadSafe()->...
      //    work as expected and do not try call parent multithread (this)
      myThread = _threads[ i ];
      myThread->initializeDependent();
   }
   myThread = tmpMyThread;
}

void SMPMultiThread::enterTeam( TeamData *data ) {
   //Enter parent thread into the team
   BaseThread::enterTeam( data );

   //Enter all sub-threads into the team
   for ( unsigned int i = 0; i < _threads.size(); i++ ) {
      _threads[ i ]->enterTeam( data );
   }
}

void SMPMultiThread::leaveTeam( void ) {
   //Remove all sub-threads from the team without deleting the teamData as it is shared
   BaseThread *tmpMyThread = myThread;
   for ( unsigned int i = 0; i < _threads.size(); i++ ) {
      //Change myThread so calls to myThread->... or getMythreadSafe()->...
      //    work as expected
      myThread = _threads[ i ];
      myThread->leaveTeamNoDeleteTeamData();
   }
   myThread = tmpMyThread;

   //Remove parent thread from the team and delete the teamData
   BaseThread::leaveTeam();
}

void SMPMultiThread::setLeaveTeam( bool leave ) {
   //Do it for each sub-thread
   for ( unsigned int i = 0; i < _threads.size(); i++ ) {
      _threads[ i ]->setLeaveTeam( leave );
   }

   //Do it for parent thread
   BaseThread::setLeaveTeam( leave );
}
