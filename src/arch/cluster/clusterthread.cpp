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
#include "instrumentation.hpp"
#include "clusterthread_decl.hpp"
#include "clusternode_decl.hpp"
#include "system_decl.hpp"
#include "workdescriptor_decl.hpp"
#include "basethread.hpp"
#include "smpthread.hpp"
#include "netwd_decl.hpp"
#include "clusterconfig.hpp"

#ifdef OpenCL_DEV
#include "opencldd.hpp"
#endif
#ifdef FPGA_DEV
#include "fpgadd.hpp"
#endif

using namespace nanos;
using namespace ext;

ClusterThread::RunningWDQueue::RunningWDQueue() : _numRunning(0), _completedHead(0), _completedHead2(0), _completedTail(0), _waitingDataWDs(), _pendingInitWD( NULL ) {
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
   ensure( _numRunning.value() > 0, "invalid value");
   _numRunning--;
}

ClusterThread::ClusterThread( WD &w, PE *pe, SMPMultiThread *parent, int device )
   : BaseThread( parent->getOsId(), w, pe, parent ), _clusterNode( device ), _lock() {
   setCurrentWD( w );
   _runningWDs = new RunningWDQueue[ClusterConfig::getMaxClusterArchId() + 1];
}

ClusterThread::~ClusterThread() {
}

void ClusterThread::runDependent () {
   WD &work = getThreadWD();
   setCurrentWD( work );

   SMPDD &dd = ( SMPDD & ) work.activateDevice( getSMPDevice() );

   dd.getWorkFct()( work.getData() );
}

void ClusterThread::join() {
   message( "Node " << ( ( ClusterNode * ) this->runningOn() )->getClusterNodeNum() << " executed " <<( ( ClusterNode * ) this->runningOn() )->getExecutedWDs() << " WDs" );
   sys.getNetwork()->sendExitMsg( _clusterNode );
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
   int arch = -1;
   ensure( completedWD->hasActiveDevice(), "At this point, thw WD must have an active device" );
   Device const &wdDevice = *( completedWD->getActiveDevice().getDevice() );
   if ( wdDevice == getSMPDevice() )
   {
      arch = 0;
   }
#ifdef GPU_DEV
   else if ( wdDevice == GPU )
   {
      arch = 1;
   }
#endif
#ifdef OpenCL_DEV
   else if ( wdDevice == OpenCLDev )
   {
      arch = 2;
   }
#endif
#ifdef FPGA_DEV
   else
   {
      bool canRun = false;
      arch = 3;
      for (FPGADeviceMap::iterator it = FPGADD::getDevicesMapBegin();
           it != FPGADD::getDevicesMapEnd(); ++it) {
         if ( wdDevice == *it->second ) {
            canRun = true;
            break;
         }
         arch++;
      }
      if (!canRun) {
         fatal("Unsupported architecture");
      }
   }
#else
   else {
      fatal("Unsupported architecture");
   }
#endif
   _runningWDs[ arch ].completeWD( completedWD );
}
void ClusterThread::addRunningWD( unsigned int archId, WorkDescriptor *wd ) {
   _runningWDs[archId].addRunningWD( wd );
}
unsigned int ClusterThread::numRunningWDs( unsigned int archId ) const {
   return _runningWDs[archId].numRunningWDs();
}
void ClusterThread::clearCompletedWDs( unsigned int archId ) {
   _runningWDs[archId].clearCompletedWDs( this );
}
bool ClusterThread::acceptsWDs( unsigned int archId ) const {
   ensure( archId <= ClusterConfig::getMaxClusterArchId(), "Wrong archId in ClusterThread::acceptsWDs" );
   unsigned int presend_setting = 0;
   switch (archId) {
      case 0: //SMP
         presend_setting = ClusterConfig::getSmpPresend();
         break;
      case 1: //GPU
         presend_setting = ClusterConfig::getGpuPresend();
         break;
      case 2: //OCL
         presend_setting = ClusterConfig::getOclPresend();
         break;
      default: //FPGA
         presend_setting = ClusterConfig::getFpgaPresend();
         break;
   }
   return ( numRunningWDs(archId) < presend_setting );
}

void ClusterThread::idle( bool debug )
{
   // poll the network as the parent thread
   BaseThread *orig_myThread = myThread;
   BaseThread *parent = myThread->getParent();
   myThread = parent;
   sys.getNetwork()->poll(0);
   myThread = orig_myThread;

   if ( !_pendingRequests.empty() ) {
      std::set<void *>::iterator it = _pendingRequests.begin();
      while ( it != _pendingRequests.end() ) {
         GetRequest *req = (GetRequest *) (*it);
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

void ClusterThread::initializeDependent( void ) {}
void ClusterThread::switchToNextThread() {}

void ClusterThread::lock() {
   _lock.acquire();
}

void ClusterThread::unlock() {
   _lock.release();
}

bool ClusterThread::tryLock() {
   return _lock.tryAcquire();
}

bool ClusterThread::hasAPendingWDToInit( unsigned int arch_id ) const {
   return _runningWDs[arch_id].hasAPendingWDToInit();
}

bool ClusterThread::RunningWDQueue::hasAPendingWDToInit() const {
   return _pendingInitWD != NULL;
}

WD *ClusterThread::getPendingInitWD( unsigned int arch_id ) {
   return _runningWDs[arch_id].getPendingInitWD();
}

WD *ClusterThread::RunningWDQueue::getPendingInitWD() {
   WD *wd = _pendingInitWD;
   _pendingInitWD = NULL;
   return wd;
}

void ClusterThread::setPendingInitWD( unsigned int arch_id, WD *wd ) {
   _runningWDs[arch_id].setPendingInitWD( wd );
}

void ClusterThread::RunningWDQueue::setPendingInitWD( WD *wd ) {
   _pendingInitWD = wd;
}

bool ClusterThread::RunningWDQueue::hasWaitingDataWDs() const {
   return !_waitingDataWDs.empty();
}

bool ClusterThread::hasWaitingDataWDs( unsigned int archId ) const {
   return _runningWDs[archId].hasWaitingDataWDs();
}
WD* ClusterThread::getWaitingDataWD( unsigned int archId ) {
   return _runningWDs[archId].getWaitingDataWD();
}

WD *ClusterThread::RunningWDQueue::getWaitingDataWD() {
   WD *wd = _waitingDataWDs.front();
   _waitingDataWDs.pop_front();
//std::cerr << "popped a wd ( " << wd << " )" << wd->getId() << ", count is " << _waitingDataWDs.size() << std::endl;
   return wd;
}

void ClusterThread::addWaitingDataWD( unsigned int archId, WD *wd ) {
   _runningWDs[archId].addWaitingDataWD( wd );
}

void ClusterThread::RunningWDQueue::addWaitingDataWD( WD *wd ) {
   _waitingDataWDs.push_back( wd );
//std::cerr << "Added a wd ( " << wd << " )" << wd->getId() << ", count is " << _waitingDataWDs.size() << std::endl;
}

void ClusterThread::setupSignalHandlers() {
   std::cerr << __FUNCTION__ << ": unimplemented in ClusterThread." << std::endl;
}


WD * ClusterThread::getClusterWD( BaseThread *thread )
{
   WD * wd = NULL;
   if ( thread->getTeam() != NULL ) {
      wd = thread->getNextWD();
      if ( wd ) {
         if ( !thread->runningOn()->canRun( *wd ) )
         { // found a non compatible wd in "nextWD", ignore it
            wd = thread->getTeam()->getSchedulePolicy().atIdle ( thread, 0 );
            //if(wd!=NULL)std::cerr << "GN got a wd with depth " <<wd->getDepth() << std::endl;
         } else {
            //thread->resetNextWD();
           // std::cerr << "FIXME" << std::endl;
         }
      } else {
         wd = thread->getTeam()->getSchedulePolicy().atIdle ( thread, 0 );
         //if(wd!=NULL)std::cerr << "got a wd with depth " <<wd->getDepth() << std::endl;
      }
   }
   return wd;
}

void ClusterThread::workerClusterLoop ()
{
   BaseThread *parent = myThread;
   BaseThread *current_thread = ( myThread = myThread->getNextThread() );
   const int init_spins = ( ( SMPMultiThread* ) parent )->getNumThreads();
   int spins = init_spins;

   NANOS_INSTRUMENT ( static Instrumentation *instr = sys.getInstrumentation(); );
   NANOS_INSTRUMENT ( static InstrumentationDictionary *id =
           instr->getInstrumentationDictionary(); );
   NANOS_INSTRUMENT ( static nanos_event_key_t nodeSelectKey =
           id->getEventKey( "cluster-select-node" ); );

   for ( ; ; ) {
      if ( !parent->isRunning() ) break;

      bool isIdle = true;

      if ( parent != current_thread ) // if parent == myThread, then there are no "soft" threads and just do nothing but polling.
      {
         ClusterThread *myClusterThread = ( ClusterThread * ) current_thread;
         if ( myClusterThread->tryLock() ) {
            ClusterNode *thisNode = ( ClusterNode * ) current_thread->runningOn();

            NANOS_INSTRUMENT ( instr->raiseOpenBurstEvent(
                        nodeSelectKey, thisNode->getNodeNum() ); );

            ClusterNode::ClusterSupportedArchMap const &archs = thisNode->getSupportedArchs();
            for ( ClusterNode::ClusterSupportedArchMap::const_iterator it = archs.begin();
                  it != archs.end(); it++ ) {
               unsigned int arch_id = it->first;
               thisNode->setActiveDevice( it->second );
               myClusterThread->clearCompletedWDs( arch_id );
               if ( myClusterThread->hasWaitingDataWDs( arch_id ) ) {
                  isIdle = false; //< Cluster worker is not idle

                  WD * wd_waiting = myClusterThread->getWaitingDataWD( arch_id );
                  if ( wd_waiting->isInputDataReady() ) {
                     myClusterThread->addRunningWD( arch_id, wd_waiting );
                     Scheduler::outlineWork( current_thread, wd_waiting );
                  } else {
                     myClusterThread->addWaitingDataWD( arch_id, wd_waiting );


                     // Try to get a WD normally, this is needed because otherwise we will keep only checking the WaitingData WDs
                     if ( myClusterThread->hasAPendingWDToInit( arch_id ) ) {
                        WD * wd = myClusterThread->getPendingInitWD( arch_id );
                        if ( Scheduler::tryPreOutlineWork(wd) ) {
                           current_thread->runningOn()->preOutlineWorkDependent( *wd );
                           //std::cerr << "GOT A PENDIGN WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                           if ( wd->isInputDataReady() ) {
                              myClusterThread->addRunningWD( arch_id, wd );
                              //NANOS_INSTRUMENT( InstrumentState inst2(NANOS_OUTLINE_WORK, true); );
                              Scheduler::outlineWork( current_thread, wd );
                              //NANOS_INSTRUMENT( inst2.close(); );
                           } else {
                              myClusterThread->addWaitingDataWD( arch_id, wd );
                           }
                        } else {
                           //std::cerr << "REPEND WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                           myClusterThread->setPendingInitWD( arch_id, wd );
                        }
                     } else {
                        if ( myClusterThread->acceptsWDs( arch_id ) )
                        {
                           WD * wd = getClusterWD( current_thread );
                           if ( wd )
                           {
                              Scheduler::prePreOutlineWork(wd);
                              if ( Scheduler::tryPreOutlineWork(wd) ) {
                                 current_thread->runningOn()->preOutlineWorkDependent( *wd );
                                 if ( wd->isInputDataReady() ) {
                                    //std::cerr << "SUCCED WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                                    myClusterThread->addRunningWD( arch_id, wd );
                                    //NANOS_INSTRUMENT( InstrumentState inst2(NANOS_OUTLINE_WORK, true); );
                                    Scheduler::outlineWork( current_thread, wd );
                                    //NANOS_INSTRUMENT( inst2.close(); );
                                 } else {
                                    myClusterThread->addWaitingDataWD( arch_id, wd );
                                 }
                              } else {
                                 //std::cerr << "ADDED A PENDIGN WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                                 myClusterThread->setPendingInitWD( arch_id, wd );
                              }
                           }
                        }// else { std::cerr << "Max presend reached "<<myClusterThread->getId()  << std::endl; }
                     }
                  }
               } else {
                  if ( myClusterThread->hasAPendingWDToInit( arch_id ) ) {
                     isIdle = false; //< Cluster worker is not idle

                     WD * wd = myClusterThread->getPendingInitWD( arch_id );
                     if ( Scheduler::tryPreOutlineWork(wd) ) {
                        current_thread->runningOn()->preOutlineWorkDependent( *wd );
                        //std::cerr << "GOT A PENDIGN WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                        if ( wd->isInputDataReady() ) {
                           myClusterThread->addRunningWD( arch_id, wd );
                           //NANOS_INSTRUMENT( InstrumentState inst2(NANOS_OUTLINE_WORK, true); );
                           Scheduler::outlineWork( current_thread, wd );
                           //NANOS_INSTRUMENT( inst2.close(); );
                        } else {
                           myClusterThread->addWaitingDataWD( arch_id, wd );
                        }
                     } else {
                        //std::cerr << "REPEND WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                        myClusterThread->setPendingInitWD( arch_id, wd );
                     }
                  } else {
                     if ( myClusterThread->acceptsWDs( arch_id ) )
                     {
                        WD * wd = getClusterWD( current_thread );
                        if ( wd ) {
                           isIdle = false; //< Cluster worker is not idle

                           Scheduler::prePreOutlineWork(wd);
                           if ( Scheduler::tryPreOutlineWork(wd) ) {
                              current_thread->runningOn()->preOutlineWorkDependent( *wd );
                              if ( wd->isInputDataReady() ) {
                                 //std::cerr << "SUCCED WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                                 myClusterThread->addRunningWD( arch_id, wd );
                                 //NANOS_INSTRUMENT( InstrumentState inst2(NANOS_OUTLINE_WORK, true); );
                                 Scheduler::outlineWork( current_thread, wd );
                                 //NANOS_INSTRUMENT( inst2.close(); );
                              } else {
                                 myClusterThread->addWaitingDataWD( arch_id, wd );
                              }
                           } else {
                              //std::cerr << "ADDED A PENDIGN WD for thd " << current_thread->getId() <<" wd is " << wd->getId() << std::endl;
                              myClusterThread->setPendingInitWD( arch_id, wd );
                           }
                        }
                     }// else { std::cerr << "Max presend reached "<<myClusterThread->getId()  << std::endl; }
                  }
               }
            }
            NANOS_INSTRUMENT ( instr->raiseCloseBurstEvent( nodeSelectKey, 0 ); );
            myClusterThread->unlock();
         }
      }
      //sys.getNetwork()->poll(parent->getId());
      if ( myThread->processTransfers() ) {
         isIdle = false; //< Worker has done some useful work in processTransfers -> It is not Idle
      }

      if ( !isIdle ) {
         spins = init_spins;
      } else if ( --spins == 0 ) {
         //The worker is idle and the max. number of spins has been reached
         spins = init_spins;

         static bool hybridEnabled = ClusterConfig::getHybridWorkerEnabled();
         if ( hybridEnabled ) {
            BaseThread *tmpThread = myThread;
            myThread = parent; //Parent should be already an smp thread

            //Try to get one SMP task
            Scheduler::helperWorkerLoop();

            static bool eventDispEneabled = ClusterConfig::getHybridWorkerPlusEnabled();
            if ( eventDispEneabled ) {
               //Call the event dispatcher module
               sys.getEventDispatcher().atIdle();
            }

            myThread = tmpThread;
         }

         //TODO: Add yield stuff
      }

      current_thread = ( myThread = myThread->getNextThread() );
   }

   SMPMultiThread *parentM = ( SMPMultiThread * ) parent;
   for ( unsigned int i = 0; i < parentM->getNumThreads(); i += 1 ) {
      myThread = parentM->getThreadVector()[ i ];
      myThread->joined();
   }
   myThread = parent;
}
