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
#include "clusternode_decl.hpp"
#include "clusterthread_decl.hpp"
#include "debug.hpp"
#include "schedule.hpp"
#include "cachedaccelerator.hpp"

using namespace nanos;
using namespace nanos::ext;

bool ClusterNode::_clusterWorkerCalled = false;

ClusterNode::ClusterNode( int nodeId, memory_space_id_t memId,
   ClusterSupportedArchMap const &archs, const Device **archsArray ) :
   ProcessingElement( archsArray, archs.size(), memId, nodeId,
   0, /* TODO: should be NumaNode, use HWLoc to get the correct value (NIC numa node) */
   true,
   0,
   false ),
   _clusterNode ( nodeId ), _executedWorkDesciptors ( 0 ), _supportedArchsById( archs ) {
}

ClusterNode::~ClusterNode() {
}

WorkDescriptor & ClusterNode::getWorkerWD () const {
   SMPDD * dd = new SMPDD( ( SMPDD::work_fct )0xdeadbeef );
   WD *wd = new WD( dd );
   return *wd;
}

WorkDescriptor & ClusterNode::getMasterWD () const {
   fatal("Attempting to create a cluster master thread");
}

WD & ClusterNode::getMultiWorkerWD ( DD::work_fct workerFun ) const {
   fatal( "getMultiWorkerWD: ClusterNode is not allowed to create MultiThreads" );
}

BaseThread &ClusterNode::createThread ( WorkDescriptor &helper, SMPMultiThread *parent ) {
   // In fact, the GPUThread will run on the CPU, so make sure it canRunIn( SMP )
   ensure( helper.canRunIn( getSMPDevice() ), "Incompatible worker thread" );
   ClusterThread &th = *new ClusterThread( helper, this, parent, _clusterNode );

   return th;
}

BaseThread & ClusterNode::createMultiThread ( WorkDescriptor &wd, unsigned int numPEs, PE **repPEs ) {
   fatal( "ClusterNode is not allowed to create MultiThreads" );
}

bool ClusterNode::supportsUserLevelThreads () const {
   return false;
}

unsigned int ClusterNode::getMyNodeNumber() const {
   return _clusterNode;
}

unsigned int ClusterNode::getClusterNodeNum() const {
   return _clusterNode;
}

//SimpleAllocator & ClusterNode::getAllocator( void ) {
//   return _memSegment;
//}

void ClusterNode::incExecutedWDs() {
   _executedWorkDesciptors++;
}

unsigned int ClusterNode::getExecutedWDs() const {
   return _executedWorkDesciptors;
}

unsigned int ClusterNode::getNodeNum() const {
   return _clusterNode;
}

void ClusterNode::clusterWorker() {
   _clusterWorkerCalled = true;
   if ( sys.getNetwork()->getNodeNum() > 0 ) {
      Scheduler::workerLoop();
      exit(0);
   }
}

ClusterNode::ClusterSupportedArchMap const &ClusterNode::getSupportedArchs() const {
   return _supportedArchsById;
}

bool ClusterNode::inlineWorkDependent ( WD &wd ) {
   fatal( "inline execution is not supported in this architecture (cluster).");
   return true;
}

void ClusterNode::preOutlineWorkDependent ( WD &wd ) {
   ensure( _clusterWorkerCalled, "Trying to send a WD to a remote node but 'ompss_nanox_main' has not been called." <<
      " Has the application been compiled with the --cluster flag?" );
   wd.preStart(WorkDescriptor::IsNotAUserLevelThread);
}

void ClusterNode::outlineWorkDependent ( WD &wd )
{
   SMPDD &dd = ( SMPDD & )wd.getActiveDevice();
   ProcessingElement *pe = this;
   if (dd.getWorkFct() == NULL ) return;

   NANOS_INSTRUMENT ( static Instrumentation *instr = sys.getInstrumentation(); );
   NANOS_INSTRUMENT ( static InstrumentationDictionary *id =
         instr->getInstrumentationDictionary(); );
   NANOS_INSTRUMENT ( static nanos_event_key_t taskOffloadKey =
         id->getEventKey( "cluster-offload-task" ); );
   NANOS_INSTRUMENT ( instr->raiseOpenBurstEvent(
            taskOffloadKey, ( nanos_event_value_t ) wd.getId() ) );

   ( ( ClusterNode * ) pe )->incExecutedWDs();
   sys.getNetwork()->sendWorkMsg( ( ( ClusterNode * ) pe )->getClusterNodeNum(), wd );

   NANOS_INSTRUMENT ( instr->raiseCloseBurstEvent( taskOffloadKey, 0 ) );
}
