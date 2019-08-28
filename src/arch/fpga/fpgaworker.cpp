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

#include "fpgaworker.hpp"
#include "fpgaprocessor.hpp"
#include "schedule.hpp"
#include "instrumentation.hpp"
#include "system.hpp"
#include "os.hpp"
#include "queue.hpp"
#include "simpleallocator.hpp"
#include "fpgawd_decl.hpp"

using namespace nanos;
using namespace ext;

FPGAWorker::FPGARegisteredTasksMap * FPGAWorker::_registeredTasks = NULL;
EventListener * FPGAWorker::_createWdListener = NULL;

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

void FPGAWorker::handleFPGACreatedTasks() {
   //NOTE: Only one thread can handle the creation of tasks at the same time.
   //FIXME: Delete the lock when the FPGA outs ready tasks instead of new tasks
   static Lock handleLock;
   if ( !handleLock.tryAcquire() ) return;

   xtasks_newtask *task = NULL;
   while ( sys.testThrottleTaskIn() ) {
      xtasks_stat stat = xtasksTryGetNewTask( &task );
      if ( stat != XTASKS_SUCCESS ) {
         free(task);
         ensure( stat == XTASKS_PENDING, " Error trying to get a new task from FPGA" );
         break;
      }
      sys.throttleTaskIn(); //TODO: If this call returns false, the thread should block at this point

      FPGARegisteredTasksMap::const_iterator infoIt = _registeredTasks->find( task->typeInfo );
      ensure( infoIt != _registeredTasks->end(), " FPGA device trying to create an unregistered task" );
      FPGARegisteredTask * info = infoIt->second;
      ensure( info->translate != NULL || task->numCopies == 0, " If the WD has copies, the translate_args cannot be NULL" );
      WD * parentWd = ( WD * )( ( uintptr_t )task->parentId );

      NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-create-task", parentWd != NULL ? parentWd->getId() : -1 ) );

      size_t sizeData, alignData, offsetData, sizeDPtrs, offsetDPtrs, sizeCopies, sizeDimensions, offsetCopies,
         offsetDimensions, offsetPMD, offsetSched, totalSize;
      static size_t sizePMD = sys.getPMInterface().getInternalDataSize();
      static size_t sizeSched = sys.getDefaultSchedulePolicy()->getWDDataSize();

      ensure( sizeof( unsigned long long int ) == sizeof( uint64_t ),
         " Unsupported width of unsinged long long int type, it is expected to be 8 bytes" );
      sizeData = sizeof( unsigned long long int )*task->numArgs;
      alignData = __alignof__(unsigned long long int);
      offsetData = NANOS_ALIGNED_MEMORY_OFFSET( 0, sizeof( FPGAWD ), alignData );
      sizeDPtrs = sizeof( DD * )*info->numDevices;
      offsetDPtrs = NANOS_ALIGNED_MEMORY_OFFSET( offsetData, sizeData, __alignof__( DD * ) );
      if ( task->numCopies != 0 ) {
         sizeCopies = sizeof( CopyData )*task->numCopies;
         offsetCopies = NANOS_ALIGNED_MEMORY_OFFSET( offsetDPtrs, sizeDPtrs, __alignof__( nanos_copy_data_t ) );
         sizeDimensions = sizeof( nanos_region_dimension_internal_t )*task->numCopies; //< 1 dimension x copy
         offsetDimensions = NANOS_ALIGNED_MEMORY_OFFSET( offsetCopies, sizeCopies, __alignof__( nanos_region_dimension_internal_t ) );
      } else {
         sizeCopies = sizeDimensions = 0;
         offsetCopies =  offsetDimensions = NANOS_ALIGNED_MEMORY_OFFSET( offsetDPtrs, sizeDPtrs, 1 );
      }
      if ( sizePMD != 0 ) {
         static size_t alignPMD = sys.getPMInterface().getInternalDataAlignment();
         offsetPMD = NANOS_ALIGNED_MEMORY_OFFSET( offsetDimensions, sizeDimensions, alignPMD );
      } else {
         offsetPMD = offsetDimensions;
         sizePMD = sizeDimensions;
      }
      if ( sizeSched != 0 )
      {
         static size_t alignSched = sys.getDefaultSchedulePolicy()->getWDDataAlignment();
         offsetSched = NANOS_ALIGNED_MEMORY_OFFSET( offsetPMD, sizePMD, alignSched );
         totalSize = NANOS_ALIGNED_MEMORY_OFFSET( offsetSched, sizeSched, 1 );
      } else {
         offsetSched = offsetPMD; // Needed by compiler unused variable error
         totalSize = NANOS_ALIGNED_MEMORY_OFFSET( offsetPMD, sizePMD, 1);
      }

      char * chunk = NEW char[totalSize];
      WD * uwd = ( WD * )( chunk );
      unsigned long long int * data = ( unsigned long long int * )( chunk + offsetData );

      DD **devPtrs = ( DD ** )( chunk + offsetDPtrs );
      for ( size_t i = 0; i < info->numDevices; i++ ) {
         devPtrs[i] = ( DD * )( info->devices[i].factory( info->devices[i].arg ) );
      }

      CopyData * copies = ( CopyData * )( chunk + offsetCopies );
      ::bzero( copies, sizeCopies );
      nanos_region_dimension_internal_t * copiesDimensions =
         ( nanos_region_dimension_internal_t * )( chunk + offsetDimensions );

      FPGAWD * createdWd = new (uwd) FPGAWD( info->numDevices, devPtrs, sizeData, alignData, data,
         task->numCopies, ( task->numCopies > 0 ? copies : NULL ), info->translate, info->description.c_str() );

      createdWd->setTotalSize( totalSize );
      createdWd->setVersionGroupId( ( unsigned long )( info->numDevices ) );
      if ( sizePMD > 0 ) {
         sys.getPMInterface().initInternalData( chunk + offsetPMD );
         createdWd->setInternalData( chunk + offsetPMD );
      }
      if ( sizeSched > 0 ){
         sys.getDefaultSchedulePolicy()->initWDData( chunk + offsetSched );
         ScheduleWDData * schedData = reinterpret_cast<ScheduleWDData*>( chunk + offsetSched );
         createdWd->setSchedulerData( schedData, /*ownedByWD*/ false );
      }

      if ( parentWd != NULL ) {
         parentWd->addWork( *createdWd );
      }

      //Set the copies information
      for ( size_t cIdx = 0; cIdx < task->numCopies; ++cIdx ) {
         //Allocate a buffer in the host to handle the data during the task execution
         //NOTE: Not reusing storage between tasks
         void * hostAddr = malloc( task->copies[cIdx].size );

         copiesDimensions[cIdx].size = task->copies[cIdx].size;
         copiesDimensions[cIdx].lower_bound = task->copies[cIdx].offset;
         copiesDimensions[cIdx].accessed_length = task->copies[cIdx].accessedLen;

         copies[cIdx].sharing = NANOS_SHARED;
         copies[cIdx].address = hostAddr;
         copies[cIdx].flags.input = task->copies[cIdx].flags & NANOS_ARGFLAG_COPY_IN;
         copies[cIdx].flags.output = task->copies[cIdx].flags & NANOS_ARGFLAG_COPY_OUT;
         ensure( copies[cIdx].flags.input || copies[cIdx].flags.output, " Creating a copy which is not input nor output" );
         copies[cIdx].dimension_count = 1;
         copies[cIdx].dimensions = &copiesDimensions[cIdx];
         copies[cIdx].offset = 0;

         const uint64_t devAddr = ( uintptr_t )( task->copies[cIdx].address );
         createdWd->setOrigFpgaCopyAddr( cIdx, devAddr );
         if ( copies[cIdx].flags.input ) {
            //Copy the data from the FPGA. It will be copied back before the finalization notification
            fpgaCopyDataFromFPGA( fpgaAllocator->getBufferHandle(),
               devAddr - fpgaAllocator->getBaseAddress(), copiesDimensions[cIdx].size,
               hostAddr );
         }
      }

      sys.setupWD( *createdWd, parentWd );

      //Set the WD input data
      memcpy(data, task->args, sizeof(unsigned long long int)*task->numArgs);

      if ( task->numDeps > 0 ) {
         //Set the dependencies information
         nanos_data_access_internal_t dependences[task->numDeps];
         nanos_region_dimension_t depsDimensions[task->numDeps]; //< 1 dimension per dependence
         for ( size_t dIdx = 0; dIdx < task->numDeps; ++dIdx ) {
            depsDimensions[dIdx].size = 0; //TODO: Obtain this field
            depsDimensions[dIdx].lower_bound = 0;
            depsDimensions[dIdx].accessed_length = 0; //TODO: Obtain this field

            dependences[dIdx].address = ( void * )( ( uintptr_t )( task->deps[dIdx].address ) );
            dependences[dIdx].offset = 0;
            dependences[dIdx].dimensions = &depsDimensions[dIdx];
            dependences[dIdx].flags.input = task->deps[dIdx].flags & NANOS_ARGFLAG_DEP_IN;
            dependences[dIdx].flags.output = task->deps[dIdx].flags & NANOS_ARGFLAG_DEP_OUT;
            dependences[dIdx].flags.can_rename = 0;
            dependences[dIdx].flags.concurrent = 0;
            dependences[dIdx].flags.commutative = 0;
            dependences[dIdx].dimension_count = 1;
         }
         //NOTE: Cannot call system method as the task has to be submitted into parent WD not current WD
         SchedulePolicy* policy = sys.getDefaultSchedulePolicy();
         policy->onSystemSubmit( *createdWd, SchedulePolicy::SYS_SUBMIT_WITH_DEPENDENCIES );

         parentWd->submitWithDependencies( *createdWd, task->numDeps , ( DataAccess * )( dependences ) );
      } else {
         sys.submit( *createdWd );
      }
   }
   handleLock.release();
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

         FPGAWorker::handleFPGACreatedTasks();

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
