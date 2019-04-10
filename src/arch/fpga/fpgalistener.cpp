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

#include "simpleallocator.hpp"
#include "fpgalistener.hpp"
#include "libxtasks_wrapper.hpp"
#include "nanos-fpga.h"
#include "queue.hpp"
#include "fpgawd_decl.hpp"

using namespace nanos;
using namespace ext;

FPGACreateWDListener::FPGARegisteredTasksMap * FPGACreateWDListener::_registeredTasks = NULL;
FPGACreateWDListener * FPGACreateWDListener::_listener = NULL;

void FPGAListener::callback( BaseThread* self )
{
   static int maxThreads = FPGAConfig::getMaxThreadsIdleCallback();

   /*!
    * Try to atomically reserve an slot
    * NOTE: The order of if statements cannot be reversed as _count must be always increased to keep
    *       the value coherent after the descrease.
    */
   if ( _count.fetchAndAdd() < maxThreads || maxThreads == -1 ) {
      NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-listener", "outline" ) );

      PE * const selfPE = self->runningOn();
      WD * const selfWD = self->getCurrentWD();
      FPGAProcessor * const fpga = getFPGAProcessor();
      //verbose("FPGAListener::callback\t Thread " << self->getId() << " gets work for FPGA-PE (" << fpga << ")");

      //Simulate that the SMP thread runs on the FPGA PE
      self->setRunningOn( fpga );

      FPGAWorker::tryOutlineTask( self );

      //Restore the running PE of SMP Thread and the running WD (just in case)
      self->setCurrentWD( *selfWD );
      self->setRunningOn( selfPE );
   }
   --_count;
}

void FPGACreateWDListener::callback( BaseThread* self )
{
   static int maxThreads = 1; //NOTE: The task creation order for the same accelerator must be ensured

   /*!
    * Try to atomically reserve an slot
    * NOTE: The order of if statements cannot be reversed as _count must be always increased to keep
    *       the value coherent after the descrease.
    */
   if ( _count.fetchAndAdd() < maxThreads || maxThreads == -1 ) {
      NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-listener", "create-wd" ) );

      xtasks_newtask *task = NULL;
      while ( sys.testThrottleTaskIn() ) {
         if ( xtasksTryGetNewTask( &task ) != XTASKS_SUCCESS ) {
            free(task);
            break;
         }
         sys.throttleTaskIn(); //TODO: If this call returns false, the thread should block at this point

         FPGARegisteredTasksMap::const_iterator infoIt = _registeredTasks->find( task->typeInfo );
         ensure( infoIt != _registeredTasks->end(), " FPGA device trying to create an unregistered task" );
         FPGARegisteredTask * info = infoIt->second;
         ensure( info->translate != NULL || task->numCopies == 0, " If the WD has copies, the translate_args cannot be NULL" );

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

         WD * parentWd = ( WD * )( ( uintptr_t )task->parentId );
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
         size_t numDeps = 0;
         for ( size_t i = 0; i < task->numArgs; ++i ) {
            numDeps += ( task->args[i].flags != NANOS_ARGFLAG_NONE );
            data[i] = task->args[i].value;
         }

         if ( numDeps > 0 ) {
            //Set the dependencies information
            nanos_data_access_internal_t dependences[numDeps];
            nanos_region_dimension_t depsDimensions[numDeps]; //< 1 dimension per dependence
            for ( size_t aIdx = 0, dIdx = 0; aIdx < task->numArgs; ++aIdx ) {
               if ( task->args[aIdx].flags != NANOS_ARGFLAG_NONE ) {
                  depsDimensions[dIdx].size = 0; //TODO: Obtain this field
                  depsDimensions[dIdx].lower_bound = 0;
                  depsDimensions[dIdx].accessed_length = 0; //TODO: Obtain this field

                  dependences[dIdx].address = ( void * )( ( uintptr_t )( task->args[aIdx].value ) );
                  dependences[dIdx].offset = 0;
                  dependences[dIdx].dimensions = &depsDimensions[dIdx];
                  dependences[dIdx].flags.input = task->args[aIdx].flags & NANOS_ARGFLAG_DEP_IN;
                  dependences[dIdx].flags.output = task->args[aIdx].flags & NANOS_ARGFLAG_DEP_OUT;
                  dependences[dIdx].flags.can_rename = 0;
                  dependences[dIdx].flags.concurrent = 0;
                  dependences[dIdx].flags.commutative = 0;
                  dependences[dIdx].dimension_count = 1;
                  ++dIdx;
               }
            }
            //NOTE: Cannot call system method as the task has to be submitted into parent WD not current WD
            SchedulePolicy* policy = sys.getDefaultSchedulePolicy();
            policy->onSystemSubmit( *createdWd, SchedulePolicy::SYS_SUBMIT_WITH_DEPENDENCIES );

            parentWd->submitWithDependencies( *createdWd, numDeps , ( DataAccess * )( dependences ) );
         } else {
            sys.submit( *createdWd );
         }
      }
   }
   --_count;
}
