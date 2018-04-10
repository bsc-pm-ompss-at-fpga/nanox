/*************************************************************************************/
/*      Copyright 2018 Barcelona Supercomputing Center                               */
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
#include "wddeque.hpp"
#include "plugin.hpp"
#include "system.hpp"

namespace nanos {
   namespace ext {

      class NodePolicy : public SchedulePolicy
      {
         public:
            using SchedulePolicy::queue;
            static bool       _usePriority;
            static bool       _useSmartPriority;
         private:
            /** \brief Round-robin Node Scheduler data associated to a team of threads
              *
              */
            struct TeamData : public ScheduleTeamData
            {
               std::vector<WDPool *>      _topLevelQueues;
               WDPool                    *_childLevelsQueue;

               TeamData () : ScheduleTeamData(), _topLevelQueues(), _childLevelsQueue( NULL )
               {
                  //NOTE: Assuming that node ids are: [0, sys.getNumClusterNodes())
                  if ( _usePriority || _useSmartPriority ) {
                     for ( unsigned int i = 0; i < sys.getNetwork()->getNumNodes(); ++i ) {
                        _topLevelQueues.push_back( NEW WDPriorityQueue<>( false /* enableDeviceCounter */, true /* optimise option */ ) );
                     }
                     _childLevelsQueue = NEW WDPriorityQueue<>( true /* enableDeviceCounter */, true /* optimise option */ );
                  } else {
                     for ( unsigned int i = 0; i < sys.getNetwork()->getNumNodes(); ++i ) {
                        _topLevelQueues.push_back( NEW WDDeque( false /* enableDeviceCounter */ ) );
                     }
                     _childLevelsQueue = NEW WDDeque( true /* enableDeviceCounter */ );
                  }
               }

               ~TeamData () {
                  for ( size_t i = 0; i < _topLevelQueues.size(); ++i ) {
                     delete _topLevelQueues[i];
                  }
                  _topLevelQueues.clear();
                  delete _childLevelsQueue;
               }
            };

            /* disable copy and assigment */
            explicit NodePolicy ( const NodePolicy & );
            const NodePolicy & operator= ( const NodePolicy & );

            Atomic<unsigned int>    _lastNodeNum;

         public:
            // constructor
            NodePolicy() : SchedulePolicy ( "Round-robin Node" ), _lastNodeNum( 0 )
            {
               /* If priorities are disabled by the user and detected
                  by the compiler, disable them. If enabled by the
                  user (default) and not detected by the compiler,
                  disable too
                */
              _usePriority = _usePriority && sys.getPrioritiesNeeded();
            }

            // destructor
            virtual ~NodePolicy() {}

            virtual size_t getTeamDataSize () const { return sizeof(TeamData); }
            virtual size_t getThreadDataSize () const { return 0; }

            virtual ScheduleTeamData * createTeamData ()
            {
               return NEW TeamData();
            }

            virtual ScheduleThreadData * createThreadData ()
            {
               return NULL;
            }

            /*!
             * \brief This method performs the main task of the smart priority
             * scheduler, which is to propagate the priority of a WD to its
             * immediate predecessors. It is meant to be invoked from
             * DependenciesDomain::submitWithDependenciesInternal.
             * \param [in/out] predecessor The preceding DependableObject.
             * \param [in] successor DependableObject whose WD priority has to be
             * propagated.
             */
            void atSuccessor ( DependableObject &successor, DependableObject &predecessor )
            {
               if ( ! _useSmartPriority ) return;

               WD *pred = ( WD* ) predecessor.getRelatedObject();
               if ( pred == NULL ) return;

               WD *succ = ( WD* ) successor.getRelatedObject();
               if ( succ == NULL ) {
                  fatal( "SmartPriority::successorFound  successor->getRelatedObject() is NULL" );
               }

               debug ( "Propagating priority from "
                  << (void*)succ << ":" << succ->getId() << " to "
                  << (void*)pred << ":"<< pred->getId()
                  << ", old priority: " << pred->getPriority()
                  << ", new priority: " << std::max( pred->getPriority(),
                  succ->getPriority() )
               );

               // Propagate priority
               if ( pred->getPriority() < succ->getPriority() ) {
                  pred->setPriority( succ->getPriority() );

                  // Reorder
                  WDPriorityQueue<> *q = NULL;
                  TeamData &tdata = (TeamData &) *myThread->getTeam()->getScheduleData();
                  if ( pred->getDepth() == 1 ) {
                     unsigned int memSpaceId = pred->isTiedToLocation();
                     unsigned int nodeNum = memSpaceId > 0 ? sys.getSeparateMemory( memSpaceId ).getNodeNumber() : 0;
                     q = (WDPriorityQueue<> *) tdata._topLevelQueues[nodeNum];
                  } else {
                     q = (WDPriorityQueue<> *) tdata._childLevelsQueue;
                  }
                  q->reorderWD( pred );
               }
            }

            /*!
            *  \brief Enqueue a work descriptor in the readyQueue of the passed thread
            *  \param thread pointer to the thread to which readyQueue the task must be appended
            *  \param wd a reference to the work descriptor to be enqueued
            *  \sa TeamData, WD and BaseThread
            */
            virtual void queue ( BaseThread *thread, WD &wd )
            {
               TeamData &data = ( TeamData & ) *thread->getTeam()->getScheduleData();
               static unsigned int numNodes = data._topLevelQueues.size();
               BaseThread *targetThread = wd.isTiedTo();
               WDPool *q = NULL;

               if ( targetThread ) {
                  targetThread->addNextWD( &wd );
                  return;
               } else if ( wd.getDepth() > 1 ) {
                  q = data._childLevelsQueue;
               } else if ( wd.isTiedLocation() ) {
                  unsigned int memSpaceId = wd.isTiedToLocation();
                  unsigned int nodeNum = memSpaceId > 0 ? sys.getSeparateMemory( memSpaceId ).getNodeNumber() : 0;
                  ensure( nodeNum < numNodes,
                     "Trying to enqueue task in node '" << nodeNum << "' but only '" << numNodes << "' exist" );

                  q = data._topLevelQueues[nodeNum];
               } else {
                  unsigned int nodeNum = ( _lastNodeNum.fetchAndAdd() )%numNodes;
                  unsigned int memSpaceId = sys.getMemorySpaceIdOfClusterNode( nodeNum );

                  wd.tieToLocation( memSpaceId );
                  q = data._topLevelQueues[nodeNum];
               }

               q->push_front( &wd );
               sys.getThreadManager()->unblockThread( thread );
            }

            /*!
            *  \brief Function called when a new task must be created: the new created task
            *          is directly queued (Breadth-First policy)
            *  \param thread pointer to the thread to which belongs the new task
            *  \param wd a reference to the work descriptor of the new task
            *  \sa WD and BaseThread
            */
            virtual WD * atSubmit ( BaseThread *thread, WD &newWD )
            {
               queue( thread,newWD );
               return 0;
            }


            WD *atIdle ( BaseThread *thread, int numSteal );

            WD * atPrefetch ( BaseThread *thread, WD &current )
            {
               WD * found = current.getImmediateSuccessor(*thread);
               if ( found && (_usePriority || _useSmartPriority) ) {
                  warning( "NodeScheduler::atBeforeExit with priorities not implemented yet" );
                  //TODO: queue found WD if there is a task with higher priority
               }
               return found != NULL ? found : atIdle(thread,false);
            }

            WD * atBeforeExit ( BaseThread *thread, WD &current, bool schedule )
            {
               WD * found = schedule ? current.getImmediateSuccessor(*thread) : NULL;
               if ( found && (_usePriority || _useSmartPriority) ) {
                  warning( "NodeScheduler::atBeforeExit with priorities not implemented yet" );
                  //TODO: queue found WD if there is a task with higher priority
               }
               return found;
            }

            bool reorderWD ( BaseThread *t, WD *wd )
            {
               //! \bug FIXME flags of priority must be in queue
               if ( _usePriority || _useSmartPriority ) {
                  WDPriorityQueue<> *q = (WDPriorityQueue<> *) wd->getMyQueue();
                  return q? q->reorderWD( wd ) : true;
               } else {
                  return true;
               }
            }

            bool usingPriorities() const
            {
               return _usePriority || _useSmartPriority;
            }

            bool testDequeue()
            {
               TeamData &tdata = (TeamData &) *myThread->getTeam()->getScheduleData();
               unsigned int memSpaceId = myThread->runningOn()->getMemorySpaceId();
               unsigned int nodeNum = memSpaceId > 0 ? sys.getSeparateMemory( memSpaceId ).getNodeNumber() : 0;
               return tdata._topLevelQueues[nodeNum]->testDequeue() || tdata._childLevelsQueue->testDequeue();
            }
      };



      /*!
       *  \brief Function called by the scheduler when a thread becomes idle to schedule it
       *  \param thread pointer to the thread to be scheduled
       *  \sa BaseThread
       */
      WD * NodePolicy::atIdle ( BaseThread *thread, int numSteal )
      {
         WorkDescriptor * wd = thread->getNextWD();
         if ( wd ) return wd;

         TeamData &data = ( TeamData & ) *thread->getTeam()->getScheduleData();

         //! 1st: Schedule the children tasks
         wd = data._childLevelsQueue->pop_front( thread );
         if ( wd == NULL ) {
            //! 2nd: Schedule the top level tasks
            unsigned int memSpaceId = thread->runningOn()->getMemorySpaceId();
            unsigned int nodeNum = memSpaceId > 0 ? sys.getSeparateMemory( memSpaceId ).getNodeNumber() : 0;
            wd = data._topLevelQueues[nodeNum]->pop_front( thread );
         }
         return wd;
      }

      bool NodePolicy::_usePriority = true;
      bool NodePolicy::_useSmartPriority = false;

      class NodeSchedPlugin : public Plugin
      {
         public:
            NodeSchedPlugin() : Plugin( "Round-robin Node scheduling Plugin", 1 ) {}

            virtual void config( Config& cfg )
            {

               cfg.setOptionsSection( "Round-robin Node module", "Round-robin Node scheduling module" );

               cfg.registerConfigOption ( "schedule-priority", NEW Config::FlagOption( NodePolicy::_usePriority ), "Priority queue used as ready task queue");
               cfg.registerArgOption( "schedule-priority", "schedule-priority" );

               cfg.registerConfigOption ( "schedule-smart-priority", NEW Config::FlagOption( NodePolicy::_useSmartPriority ), "Smart priority queue propagates high priorities to predecessors");
               cfg.registerArgOption( "schedule-smart-priority", "schedule-smart-priority" );


            }

            virtual void init() {
               sys.setDefaultSchedulePolicy(NEW NodePolicy());
            }
      };

   }
}

DECLARE_PLUGIN("sched-node",nanos::ext::NodeSchedPlugin);
