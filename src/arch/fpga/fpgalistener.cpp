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

using namespace nanos;
using namespace ext;

FPGACreateWDListener::FPGARegisteredTasksMap * FPGACreateWDListener::_registeredTasks = NULL;

void FPGAListener::callback( BaseThread* self )
{
   static int maxThreads = FPGAConfig::getMaxThreadsIdleCallback();

   /*!
    * Try to atomically reserve an slot
    * NOTE: The order of if statements cannot be reversed as _count must be always increased to keep
    *       the value coherent after the descrease.
    */
   if ( _count.fetchAndAdd() < maxThreads || maxThreads == -1 ) {
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
   static unsigned int maxCreatedWD = 32; //TODO: Get the value from the FPGAConfig

   /*!
    * Try to atomically reserve an slot
    * NOTE: The order of if statements cannot be reversed as _count must be always increased to keep
    *       the value coherent after the descrease.
    */
   if ( _count.fetchAndAdd() < maxThreads || maxThreads == -1 ) {
      unsigned int cnt = 0;
      xtasks_newtask *task = NULL;

      //TODO: Check if throttole policy allows the task creation
      while ( cnt++ < maxCreatedWD && xtasksTryGetNewTask( &task ) == XTASKS_SUCCESS ) {
         uint64_t args[task->numArgs];
         size_t numCopies = task->numCopies,
                numDeps = 0;

         for ( size_t i = 0; i < task->numArgs; ++i ) {
            numDeps += ( task->args[i].flags != NANOS_ARGFLAG_NONE );
            args[i] = ( uintptr_t )( task->args[i].value );
         }

         WD * createdWd = NULL;
         WD * parentWd = ( WD * )( ( uintptr_t )task->parentId );
         void * data = NULL;
         FPGARegisteredTasksMap::const_iterator infoIt = _registeredTasks->find( task->typeInfo );
         ensure( infoIt != _registeredTasks->end(), " FPGA device trying to create an unregistered task" );
         FPGARegisteredTask * info = infoIt->second;
         // nanos_wd_props_t props = { .mandatory_creation = 1, .tied = 0, .clear_chunk = 0,
         //    .reserved0 = 0, .reserved1 = 0, .reserved2 = 0, .reserved3 = 0, .reserved4 = 0 };
         // nanos_wd_dyn_props_t dynProps = { .tie_to = 0, .priority = 0, .flags = { .is_final = 0, .is_implicit = 0, .is_recover = 0 } };
         nanos_copy_data_t *copies = NULL;
         nanos_region_dimension_internal_t *copiesDimensions = NULL;
         nanos_translate_args_t translator = info->translate;
         nanos_data_access_internal_t dependences[numDeps];
         nanos_region_dimension_t depsDimensions[numDeps]; //< 1 dimension per dependence
         ensure( translator != NULL || numCopies == 0, " If the WD has copies, the translate_args cannot be NULL" );
         sys.createWD( &createdWd, info->numDevices, info->devices, task->numArgs*sizeof(void *), __alignof__(void *), &data,
            parentWd, NULL /*props*/, NULL /*dyn_props*/, numCopies, numCopies > 0 ? &copies : NULL /*copies*/,
            numCopies /*1 dimension per copy*/, numCopies > 0 ? &copiesDimensions: NULL /*dimensions*/,
            translator, NULL /*description*/, NULL /*slicer*/ );
         ensure( createdWd != NULL, " Cannot create a WD in FPGACreateWDListener" );

         //Set the WD input data
         memcpy( data, &args[0], task->numArgs*sizeof( void * ) );

         //Set the dependencies information
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

         //Set the copies information
         for ( size_t cIdx = 0; cIdx < task->numCopies; ++cIdx ) {
            copiesDimensions[cIdx].size = task->copies[cIdx].size;
            copiesDimensions[cIdx].lower_bound = task->copies[cIdx].offset;
            copiesDimensions[cIdx].accessed_length = task->copies[cIdx].accessedLen;

            copies[cIdx].sharing = NANOS_SHARED;
            copies[cIdx].address = task->copies[cIdx].address;
            copies[cIdx].flags.input = task->copies[cIdx].flags & NANOS_ARGFLAG_COPY_IN;
            copies[cIdx].flags.output = task->copies[cIdx].flags & NANOS_ARGFLAG_COPY_OUT;
            ensure( copies[cIdx].flags.input || copies[cIdx].flags.output, " Creating a copy which is not input nor output" );
            copies[cIdx].dimension_count = 1;
            copies[cIdx].dimensions = &copiesDimensions[cIdx];
            copies[cIdx].offset = 0;
         }

         sys.setupWD( *createdWd, parentWd );
         if ( numDeps > 0 ) {
            //NOTE: Cannot call system method as the task has to be submitted into parent WD not current WD
            //sys.submitWithDependencies( *createdWd, numDeps, ( DataAccess * )( dependences ) );

            SchedulePolicy* policy = sys.getDefaultSchedulePolicy();
            policy->onSystemSubmit( *createdWd, SchedulePolicy::SYS_SUBMIT_WITH_DEPENDENCIES );

            parentWd->submitWithDependencies( *createdWd, numDeps , ( DataAccess * )( dependences ) );
         } else {
            sys.submit( *createdWd );
         }
      }
      free(task);
   }
   --_count;
}
