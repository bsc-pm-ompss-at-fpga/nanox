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

#include <iostream>
#include "instrumentation.hpp"
#include "clusterthread_decl.hpp"
#include "clusternode_decl.hpp"
#include "system_decl.hpp"
#include "workdescriptor_decl.hpp"

using namespace nanos;
using namespace ext;

ClusterThread::RunningWDQueue::RunningWDQueue() : _numRunning(0), _completedHead(0), _completedHead2(0), _completedTail(0) {
   for ( unsigned int i = 0; i < MAX_PRESEND; i++ )
   {
      _completedWDs[i] = NULL;
   }
}

ClusterThread::RunningWDQueue::~RunningWDQueue() {
}

void ClusterThread::RunningWDQueue::addRunningWD( WorkDescriptor *wd ) { 
   _numRunning++;
}

unsigned int ClusterThread::RunningWDQueue::numRunningWDs() const {
   return _numRunning.value();
}

void ClusterThread::RunningWDQueue::clearCompletedWDs( ClusterThread *self ) {
   unsigned int lowval = _completedTail % MAX_PRESEND;
   unsigned int highval = ( _completedHead2.value() ) % MAX_PRESEND;
   unsigned int pos = lowval;
   if ( lowval > highval ) highval +=MAX_PRESEND;
   while ( lowval < highval )
   {
      WD *completedWD = _completedWDs[pos];
      Scheduler::postOutlineWork( completedWD, false, self );
      delete[] (char *) completedWD;
      _completedWDs[pos] =(WD *) 0xdeadbeef;
      pos = (pos+1) % MAX_PRESEND;
      lowval += 1;
      _completedTail += 1;
   }
}

void ClusterThread::RunningWDQueue::completeWD( void *remoteWdAddr ) {
   unsigned int realpos = _completedHead++;
   unsigned int pos = realpos %MAX_PRESEND;
   _completedWDs[pos] = (WD *) remoteWdAddr;
   while( !_completedHead2.cswap( realpos, realpos+1) ) {}
   _numRunning--;
}

ClusterThread::ClusterThread( WD &w, PE *pe, SMPMultiThread *parent, int device ) : SMPThread( w, pe, parent ), _clusterNode( device ) {
   setCurrentWD( w );
}

ClusterThread::~ClusterThread() {
}

void ClusterThread::runDependent () {
   WD &work = getThreadWD();
   setCurrentWD( work );

   SMPDD &dd = ( SMPDD & ) work.activateDevice( SMP );

   dd.getWorkFct()( work.getData() );
}


void ClusterThread::inlineWorkDependent ( WD &wd ) {
   fatal( "inline execution is not supported in this architecture (cluster).");
}

void ClusterThread::outlineWorkDependent ( WD &wd )
{
   unsigned int i;
   SMPDD &dd = ( SMPDD & )wd.getActiveDevice();
   ProcessingElement *pe = this->runningOn();
   if (dd.getWorkFct() == NULL ) return;

   wd.start(WorkDescriptor::IsNotAUserLevelThread);
   wd.getGE()->setNode( ( ( ClusterNode * ) pe )->getClusterNodeNum() );

   unsigned int totalDimensions = 0;
   for (i = 0; i < wd.getNumCopies(); i += 1) {
      totalDimensions += wd.getCopies()[i].getNumDimensions();
   }

   size_t totalBufferSize = wd.getDataSize() + 
      sizeof(int) + wd.getNumCopies() * sizeof( CopyData ) + 
      sizeof(int) + totalDimensions * sizeof( nanos_region_dimension_t ) + 
      sizeof(int) + wd.getNumCopies() * sizeof( uint64_t );

   char *buff = new char[ totalBufferSize ];


   // Copy WD data to tmp buffer
   if ( wd.getDataSize() > 0 )
   {
      memcpy( &buff[ 0 ], wd.getData(), wd.getDataSize() );
   }

   // Set the number of copies
   *((int *) &buff[ wd.getDataSize() ] ) = wd.getNumCopies();

   // Set the number of dimensions
   *((int *) &buff[ wd.getDataSize() + sizeof( int ) + wd.getNumCopies() * sizeof( CopyData ) ] ) = totalDimensions;

   // Copy CopyData and dimension entries
   CopyData *newCopies = ( CopyData * ) ( buff + wd.getDataSize() + sizeof( int ) );
   nanos_region_dimension_internal_t *dimensions = ( nanos_region_dimension_internal_t * ) ( buff + wd.getDataSize() + sizeof( int ) + wd.getNumCopies() * sizeof( CopyData ) + sizeof( int ) );
   
   uintptr_t dimensionIndex = 0;
   for (i = 0; i < wd.getNumCopies(); i += 1) {
      new ( &newCopies[i] ) CopyData( wd.getCopies()[i] );
      memcpy( &dimensions[ dimensionIndex ], wd.getCopies()[i].getDimensions(), sizeof( nanos_region_dimension_internal_t ) * wd.getCopies()[i].getNumDimensions());
      newCopies[i].setDimensions( ( nanos_region_dimension_internal_t const *  ) dimensionIndex ); // This is the index because it makes no sense to send an address over the network
      //newCopies[i].setBaseAddress( pe->getAddress( wd, wd.getCopies()[i].getAddress(), newCopies[i].getSharing() ) );
      newCopies[i].setBaseAddress( (void *) ( wd._ccontrol.getAddress( i ) - wd.getCopies()[i].getOffset() ) );
      //message( "New get address: " << (void *) wd._ccontrol.getAddress( i) );
      dimensionIndex += wd.getCopies()[i].getNumDimensions();
   }

   //std::cerr << "run remote task, target pe: " << pe << " node num " << (unsigned int) ((ClusterNode *) pe)->getClusterNodeNum() << " " << (void *) &wd << ":" << (unsigned int) wd.getId() << " data size is " << wd.getDataSize() << " copies " << wd.getNumCopies() << " dimensions " << dimensionIndex << std::endl;
   //NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("user-code") );
   //NANOS_INSTRUMENT ( nanos_event_value_t val = wd.getId() );
   //NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenStateAndBurst ( NANOS_RUNNING, key, val ) );


#ifdef GPU_DEV
   int arch = -1;
   if (wd.canRunIn( GPU) )
   {
      arch = 1;
   }
   else if (wd.canRunIn( SMP ) )
   {
      arch = 0;
   }
#else
   int arch = 0;
#endif

   ( ( ClusterNode * ) pe )->incExecutedWDs();
   sys.getNetwork()->sendWorkMsg( ( ( ClusterNode * ) pe )->getClusterNodeNum(), dd.getWorkFct(), wd.getDataSize(), wd.getId(), /* this should be the PE id */ arch, totalBufferSize, buff, wd.getTranslateArgs(), arch, (void *) &wd );

}

void ClusterThread::join() {
   unsigned int i;
   message( "Node " << ( ( ClusterNode * ) this->runningOn() )->getClusterNodeNum() << " executed " <<( ( ClusterNode * ) this->runningOn() )->getExecutedWDs() << " WDs" );
   for ( i = 1; i < sys.getNetwork()->getNumNodes(); i++ )
      sys.getNetwork()->sendExitMsg( i );
}

void ClusterThread::start() {
}

BaseThread * ClusterThread::getNextThread ()
{
   BaseThread *next;
   if ( getParent() != NULL )
   {
      next = getParent()->getNextThread();
   }
   else
   {
      next = this;
   }
   return next;
}

void ClusterThread::notifyOutlinedCompletionDependent( WD *completedWD ) {
#ifdef GPU_DEV
   int arch = -1;
   if ( completedWD->canRunIn( GPU ) )
   {
      arch = 1;
   }
   else if ( completedWD->canRunIn( SMP ) )
   {
      arch = 0;
   }
#else
   int arch = 0;
#endif
   if ( arch < 0 ) 
      std::cerr << "unhandled arch" << std::endl;
   _runningWDs[ arch ].completeWD( completedWD );
}

void ClusterThread::addRunningWDSMP( WorkDescriptor *wd ) { 
   _runningWDs[0].addRunningWD( wd );
}
unsigned int ClusterThread::numRunningWDsSMP() const {
   return _runningWDs[0].numRunningWDs();
}
void ClusterThread::clearCompletedWDsSMP2( ) {
   _runningWDs[0].clearCompletedWDs( this );
}

void ClusterThread::addRunningWDGPU( WorkDescriptor *wd ) { 
   _runningWDs[1].addRunningWD( wd );
}

unsigned int ClusterThread::numRunningWDsGPU() const {
   return _runningWDs[1].numRunningWDs();
}

void ClusterThread::clearCompletedWDsGPU2( ) {
   _runningWDs[1].clearCompletedWDs( this );
}

void ClusterThread::idle()
{
   sys.getNetwork()->poll(0);

   if ( !_pendingRequests.empty() ) {
      std::set<void *>::iterator it = _pendingRequests.begin();
      while ( it != _pendingRequests.end() ) {
         ext::ClusterDevice::GetRequest *req = (ext::ClusterDevice::GetRequest *) (*it);
         if ( req->isCompleted() ) {
           std::set<void *>::iterator toBeDeletedIt = it;
           it++;
           _pendingRequests.erase(toBeDeletedIt);
           req->clear();
           delete req;
         } else {
            it++;
         }
      }
   }
}

bool ClusterThread::isCluster() {
   return true;
}
