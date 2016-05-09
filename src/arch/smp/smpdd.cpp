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
#include "exception/restorefailure.hpp"
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
#ifdef NANOS_INSTRUMENTATION_ENABLED
   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("user-code") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = wd.getId() );
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenStateAndBurst ( NANOS_RUNNING, key, val ) );
#endif

   dd.execute(wd);

#ifdef NANOS_INSTRUMENTATION_ENABLED
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseStateAndBurst ( key, val ) );
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
   unsigned num_tries = 0;

   if ( !wd.isAbleToExecute() ) {
      /*
       *  TODO Optimization?
       *  It could be better to skip this work if workdescriptor is flagged as invalid
       *  before allocating a new stack for the task and, perhaps,
       *  skip data copies of dependences.
       */
      wd.propagateInvalidationAndGetRecoverableAncestor();
      debug ( "Resiliency: Task ", wd.getId(), " is flagged as invalid. Skipping it.");

      NANOS_INSTRUMENT ( static nanos_event_key_t task_discard_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-task-operation") );
      NANOS_INSTRUMENT ( nanos_event_value_t task_discard_val = (nanos_event_value_t ) NANOS_FT_DISCARD );
      NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_discard_key, &task_discard_val) );

      error::FailureStats<error::DiscardedTask>::increase();
   } else {
      bool restart = true;
      do {
         try {
            // Call to the user function
            getWorkFct()( wd.getData() );

#ifdef NANOS_FAULT_INJECTION
            //
            // This is the place where execution returns after the execution of the task.
            // We check if any manual flags were set for fault injection
            //
            if (sys.getFaultyAddress() != 0) {
               wd.setInvalid(true);
               sys.setFaultyAddress(0);
            }
#endif
         } catch (nanos::error::OperationFailure& failure) {

            debug("Resiliency: error detected during task ", wd.getId(), " execution.");
            nanos::error::ExecutionFailure handle( failure );
         }

         /* 
          * A task is only re-executed when the following conditions meet:
          * 1) The execution was invalid
          * 2) The task is marked as recoverable
          * 3) The task parent is not invalid (it doesn't make sense to recover ourselves if our parent is going to undo our work)
          * 4) The task has not run out of trials (a limit is set to avoid infinite loop)
          */ 
         try {
            if ( wd.isExecutionRepeatable() ) {// Our parent is not invalid (if we got one)
               if ( num_tries < sys.getTaskMaxRetrials() ) {// We are still able to retry
                  error::FailureStats<error::TaskRecovery>::increase();
                  num_tries++;
                  restore(wd);
               } else {
                  debug( "Task ", wd.getId(), " is not being recovered again. Number of trials exhausted." );
                  // Giving up retrying...
                  wd.getParent()->propagateInvalidationAndGetRecoverableAncestor();
                  restart = false;
               }
            } else {
               debug( "Exiting task ", wd.getId(), "." );
               // Nothing left to do, either task execution was OK or the recovery has to be done by an ancestor.
               restart = false;
            }
         } catch ( error::RestoreFailure &ex ) {
            debug( "Recovering from restore error in task ", wd.getId(), "." );
            restart = false;
         }
         if(restart) {
            debug( "Task ", wd.getId(), " is being re-executed." );

            NANOS_INSTRUMENT ( static nanos_event_key_t task_reexec_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-task-operation") );
            NANOS_INSTRUMENT ( nanos_event_value_t task_reexec_val = (nanos_event_value_t ) NANOS_FT_RESTART );
            NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_reexec_key, &task_reexec_val) );
         }
      } while (restart);
   }
}
#endif

#ifdef NANOS_RESILIENCY_ENABLED
#if 0
bool SMPDD::recover( TaskException const& err ) {
   bool result = true;

   static size_t page_size = sysconf(_SC_PAGESIZE);

   // Recover system from failures
   switch(err.getSignal()){
      case SIGSEGV:

         switch(err.getSignalInfo().si_code) {
            case SEGV_MAPERR: /* Address not mapped to object.  */
               message( "SEGV_MAPERR error recovery is not supported yet." );
               return false;
            case SEGV_ACCERR: /* Invalid permissions for mapped object.  */
               uintptr_t page_addr = (uintptr_t)err.getSignalInfo().si_addr;
               // Align faulting address with virtual page address
               page_addr &= ~(page_size - 1);
#ifdef NANOS_FAULT_INJECTION
               if( sys.isPoisoningEnabled() ) {
                  result = mpoison_unblock_page(page_addr) == 0;
               } else {
                 message( "Recover function called but fault injection is disabled. This might be produced by a real error." );
                 result = false;// TODO Page unimplemented for real errors (not simulations)
               }
#else
               // We still don't know how to recover from failures other than those injected.
               message( "Recover function called but fault injection is disabled. This might be produced by a real error." );
               result = false;
#endif
               break;// case SEGV_ACCERR
         }
         break;
      default:
         break;
   }
   return result;
}
#endif

void SMPDD::restore( WD & wd ) {
   debug ( "Resiliency: Task ", wd.getId(), " is being recovered to be re-executed further on.");
   // Wait for successors to finish.
   wd.waitCompletion();

   // Restore the data
   wd._mcontrol.restoreBackupData();

   while ( !wd._mcontrol.isDataRestored( wd ) ) {
      myThread->idle();
   }
   debug ( "Resiliency: Task ", wd.getId(), " recovery complete.");
   // Reset invalid state
   wd.setInvalid(false);
}
#endif
