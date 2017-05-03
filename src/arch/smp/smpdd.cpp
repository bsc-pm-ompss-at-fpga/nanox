/*************************************************************************************/
/*      Copyright 2015 Barcelona Supercomputing Center                               */
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

#include "smpdd.hpp"

#include "instrumentation.hpp"
#include "smpdevice.hpp"
#include "smp_ult.hpp"
#include "system.hpp"

#ifdef NANOS_RESILIENCY_ENABLED
#include "exception/operationfailure.hpp"
#include "exception/executionfailure.hpp"
#include "exception/taskrecoveryfailed.hpp"
#endif

using namespace nanos;
using namespace nanos::ext;

//SMPDevice nanos::ext::SMP("SMP");

SMPDevice &nanos::ext::getSMPDevice() {
   return sys._getSMPDevice();
}

size_t SMPDD::_stackSize = 256*1024;

/*!
 \brief Registers the Device's configuration options
 \param reference to a configuration object.
 \sa Config System
 */
void SMPDD::prepareConfig ( Config &config )
{
   //! \note Get the stack size from system configuration
   size_t size = sys.getDeviceStackSize();
   if (size > 0) _stackSize = size;

   //! \note Get the stack size for this specific device
   config.registerConfigOption ( "smp-stack-size", NEW Config::SizeVar( _stackSize ), "Defines SMP::task stack size" );
   config.registerArgOption("smp-stack-size", "smp-stack-size");
   config.registerEnvOption("smp-stack-size", "NX_SMP_STACK_SIZE");
}

void SMPDD::initStack ( WD *wd )
{
   _state = ::initContext(_stack, _stackSize, &workWrapper, wd, (void *) Scheduler::exit, 0);
}

void SMPDD::workWrapper ( WD &wd )
{
   SMPDD &dd = (SMPDD &) wd.getActiveDevice();

   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("user-code") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = wd.getId() );
   NANOS_INSTRUMENT ( if ( wd.isRuntimeTask() ) { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseOpenStateEvent ( NANOS_RUNTIME ) );
   NANOS_INSTRUMENT ( } else { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseOpenStateAndBurst ( NANOS_RUNNING, key, val ) );
   NANOS_INSTRUMENT ( } );
#endif

   dd.execute(wd);

   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseStateAndBurst ( key, val ) );
   NANOS_INSTRUMENT ( if ( wd.isRuntimeTask() ) { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseCloseStateEvent() );
   NANOS_INSTRUMENT ( } else { );
   NANOS_INSTRUMENT (    sys.getInstrumentation()->raiseCloseStateAndBurst ( key, val ) );
   NANOS_INSTRUMENT ( } );
#endif
}

void SMPDD::lazyInit ( WD &wd, bool isUserLevelThread, WD *previous )
{
   verbose("Task ", wd.getId(), " initialization");
   if (isUserLevelThread) {
      if (previous == NULL) {
         _stack = (void *) NEW char[_stackSize];
         verbose("   new stack created: ", _stackSize, " bytes");
      } else {
         verbose("   reusing stacks");
         SMPDD &oldDD = (SMPDD &) previous->getActiveDevice();
         std::swap(_stack, oldDD._stack);
      }
      initStack(&wd);
   }
}

SMPDD * SMPDD::copyTo ( void *toAddr )
{
   SMPDD *dd = new (toAddr) SMPDD(*this);
   return dd;
}

#ifndef NANOS_RESILIENCY_ENABLED

void SMPDD::execute ( WD &wd ) throw ()
{
   // Workdescriptor execution
   getWorkFct()(wd.getData());
}

#else

void SMPDD::execute ( WD &wd ) throw ()
{
   if ( !wd.isAbleToExecute() ) {
      wd.propagateInvalidationAndGetRecoverableAncestor();
      debug ( "Resiliency: Task ", wd.getId(), " is flagged as invalid. Skipping it.");

      NANOS_INSTRUMENT ( static nanos_event_key_t task_discard_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-task-operation") );
      NANOS_INSTRUMENT ( nanos_event_value_t task_discard_val = (nanos_event_value_t ) NANOS_FT_DISCARD );
      NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_discard_key, &task_discard_val) );

      error::FailureStats<error::DiscardedTask>::increase();
   } else {
      bool restart = false;
      do {
         try {

            // Call to the user function
            getWorkFct()( wd.getData() );

         } catch (nanos::error::OperationFailure& failure) {
            debug("Resiliency: error detected during task ", wd.getId(), " execution.");
            nanos::error::ExecutionFailure handle( failure );
         }

         restart = wd.isExecutionRepeatable();

         if ( restart ) {
            try {
               error::FailureStats<error::TaskRecovery>::increase();
               wd.restore();

               debug( "Task ", wd.getId(), " is being re-executed." );
               NANOS_INSTRUMENT ( static nanos_event_key_t task_reexec_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-task-operation") );
               NANOS_INSTRUMENT ( nanos_event_value_t task_reexec_val = (nanos_event_value_t ) NANOS_FT_RESTART );
               NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_reexec_key, &task_reexec_val) );
            } catch ( error::TaskRecoveryFailed &ex ) {
               debug( "Recovering from restore error in task ", wd.getId(), "." );
               restart = false;
            }
         }
      } while (restart);
   }
}

#endif
