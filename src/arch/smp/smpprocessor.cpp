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

#include "smpprocessor.hpp"
#include "schedule.hpp"
#include "debug.hpp"
#include "config.hpp"
#include "basethread.hpp"
#include <iostream>

using namespace nanos;
using namespace nanos::ext;

bool SMPProcessor::_useUserThreads = true;
size_t SMPProcessor::_threadsStackSize = 0;
System::CachePolicyType SMPProcessor::_cachePolicy = System::DEFAULT;
size_t SMPProcessor::_cacheDefaultSize = 1048580;

SMPProcessor::SMPProcessor( int bindingId, const CpuSet& bindingList,
      memory_space_id_t memId, bool active, unsigned int numaNode, unsigned int socket ) :
   PE( &getSMPDevice(), memId, 0 /* always local node */, numaNode, true, socket, true ),
   _bindingId( bindingId ), _bindingList( bindingList ),
   _reserved( false ), _active( active ), _futureThreads( 0 ) {}

void SMPProcessor::prepareConfig ( Config &config )
{
   config.registerConfigOption( "user-threads", NEW Config::FlagOption( _useUserThreads, false), "Disable User Level Threads" );
   config.registerArgOption( "user-threads", "disable-ut" );

   config.registerConfigOption ( "thread-stack-size", NEW Config::SizeVar( _threadsStackSize ), "Defines thread stack size" );
   config.registerArgOption( "thread-stack-size", "thread-stack-size" );
   config.registerEnvOption( "thread-stack-size", "OMP_STACKSIZE" );
}

WorkDescriptor & SMPProcessor::getWorkerWD () const
{
   SMPDD  *dd = NEW SMPDD( ( SMPDD::work_fct )Scheduler::workerLoop );
   DeviceData **dd_ptr = NEW DeviceData*[1];
   dd_ptr[0] = (DeviceData*)dd;

   WD * wd = NEW WD( 1, dd_ptr, 0, 1, 0, 0, NULL, NULL, "SMP Worker" );

   return *wd;
}

WorkDescriptor & SMPProcessor::getMultiWorkerWD ( DD::work_fct workerFun ) const
{
   SMPDD * dd = NEW SMPDD( workerFun );
   WD *wd = NEW WD( dd, 0, 1, 0, 0, NULL, NULL, "SMP MultiWorker" );
   wd->_mcontrol.preInit();

   return *wd;
}

WorkDescriptor & SMPProcessor::getMasterWD () const
{
   SMPDD  *dd = NEW SMPDD();
   DeviceData **dd_ptr = NEW DeviceData*[1];
   dd_ptr[0] = (DeviceData*)dd;

   WD * wd = NEW WD( 1, dd_ptr, 0, 1, 0, 0, NULL, NULL, "SMP Main" );

   return *wd;
}

BaseThread &SMPProcessor::createThread ( WorkDescriptor &helper, SMPMultiThread *parent )
{
   ensure( helper.canRunIn( getSMPDevice() ),"Incompatible worker thread" );
   SMPThread &th = *NEW SMPThread( helper, this, this );
   th.stackSize( _threadsStackSize ).useUserThreads( _useUserThreads );

   return th;
}

BaseThread &SMPProcessor::createMultiThread ( WorkDescriptor &helper, unsigned int numPEs, PE **repPEs )
{
   ensure( helper.canRunIn( getSMPDevice() ),"Incompatible worker thread" );
   SMPThread &th = *NEW SMPMultiThread( helper, this, numPEs, repPEs );
   th.stackSize(_threadsStackSize).useUserThreads(_useUserThreads);

   return th;
}

SMPThread &SMPProcessor::associateThisThread( bool untieMain ) {

   WD & master = getMasterWD();
   WD & worker = getWorkerWD();

   NANOS_INSTRUMENT (sys.getInstrumentation()->raiseOpenPtPEvent ( NANOS_WD_DOMAIN, (nanos_event_id_t) master.getId(), 0, 0 ); )
   NANOS_INSTRUMENT (InstrumentationContextData *icd = master.getInstrumentationContextData() );
   NANOS_INSTRUMENT (icd->setStartingWD(true) );

   SMPThread &thread = (SMPThread &)createThread( worker );

   thread.initMain();
   thread.setMainThread();
   thread.associate( &master );
   worker._mcontrol.preInit();

   getThreads().push_back( &thread );

   if ( !untieMain ) {
      master.tieTo(thread);
   }

   return thread;
}

void SMPProcessor::setNumFutureThreads( unsigned int nthreads ) {
   _futureThreads = nthreads;
}

unsigned int SMPProcessor::getNumFutureThreads() const {
   return _futureThreads;
}

bool SMPProcessor::inlineWorkDependent ( WD &wd )
{
   // Now the WD will be inminently run
   wd.start(WD::IsNotAUserLevelThread);

   SMPDD &dd = ( SMPDD & )wd.getActiveDevice();

   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("user-code") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = wd.getId() );
   NANOS_INSTRUMENT ( if ( wd.isRuntimeTask() ) { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseOpenStateEvent ( NANOS_RUNTIME ) );
   NANOS_INSTRUMENT ( } else { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseOpenStateAndBurst ( NANOS_RUNNING, key, val ) );
   NANOS_INSTRUMENT ( } );

   //if ( sys.getNetwork()->getNodeNum() > 0 ) std::cerr << "Starting wd " << wd.getId() << std::endl;

   dd.execute( wd );

   NANOS_INSTRUMENT ( if ( wd.isRuntimeTask() ) { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseCloseStateEvent() );
   NANOS_INSTRUMENT ( } else { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseCloseStateAndBurst ( key, val ) );
   NANOS_INSTRUMENT ( } );
   return true;
}

// This is executed in between switching stacks
void SMPProcessor::switchHelperDependent ( WD *oldWD, WD *newWD, void *oldState  )
{
   SMPDD & dd = ( SMPDD & )oldWD->getActiveDevice();
   dd.setState( (intptr_t *) oldState );
}

void SMPProcessor::switchTo ( WD *wd, SchedulerHelper *helper )
{
   // wd MUST have an active SMP Device when it gets here
   ensure( wd->hasActiveDevice(),"WD has no active SMP device" );
   SMPDD &dd = ( SMPDD & )wd->getActiveDevice();
   ensure( dd.hasStack(), "DD has no stack for ULT");
   WD * currentWD = myThread->getCurrentWD();

   ::switchStacks(
       ( void * ) currentWD,
       ( void * ) wd,
       ( void * ) dd.getState(),
       ( void * ) helper );
}

void SMPProcessor::exitTo ( WD *wd, SchedulerHelper *helper)
{
   // wd MUST have an active SMP Device when it gets here
   ensure( wd->hasActiveDevice(),"WD has no active SMP device" );
   SMPDD &dd = ( SMPDD & )wd->getActiveDevice();
   ensure( dd.hasStack(), "DD has no stack for ULT");
   WD * currentWD = myThread->getCurrentWD();

   //TODO: optimize... we don't really need to save a context in this case
   ::switchStacks(
      ( void * ) currentWD,
      ( void * ) wd,
      ( void * ) dd.getState(),
      ( void * ) helper );
}
