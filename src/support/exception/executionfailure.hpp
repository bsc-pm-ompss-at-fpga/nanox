/*************************************************************************************/
/*      Copyright 2009-2015 Barcelona Supercomputing Center                          */
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

#ifndef EXECUTION_FAILURE_HPP
#define EXECUTION_FAILURE_HPP

#include "operationfailure.hpp"

#include "failurestats.hpp"
#include "instrumentation.hpp"
#include "workdescriptor.hpp"

namespace nanos {
namespace error {

class ExecutionFailure {
   private:
      OperationFailure& _failedOperation;
   public:
      ExecutionFailure( OperationFailure& operation ) :
            _failedOperation( operation )
      {
         FailureStats<ExecutionFailure>::increase();

         NANOS_INSTRUMENT ( static nanos_event_key_t task_discard_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-task-operation") );
         NANOS_INSTRUMENT ( nanos_event_value_t task_discard_val = (nanos_event_value_t ) NANOS_FT_EXEC_FAILURE );
         NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_discard_key, &task_discard_val) );

         WorkDescriptor &task = operation.getTask();
         task.increaseFailedExecutions();

         WorkDescriptor* recoverableAncestor = nullptr;
         if( task.isExecutionRepeatable() ) {
            recoverableAncestor = task.propagateInvalidationAndGetRecoverableAncestor();
         } else if( task.getParent() != nullptr ) {
            recoverableAncestor = task.getParent()->propagateInvalidationAndGetRecoverableAncestor();
         }

         if( !recoverableAncestor ) {
            fatal( "Could not find a recoverable task when recovering from ", operation.what() );
         } else {
            debug("Resiliency: execution error detected ", operation.what() );
         }
      }
};

} // namespace error
} // namespace nanos

#endif // EXECUTION_FAILURE_HPP
