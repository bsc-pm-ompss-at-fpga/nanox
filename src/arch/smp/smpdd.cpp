/**************************************************************************/
/*      Copyright 2010 Barcelona Supercomputing Center                    */
/*      Copyright 2009 Barcelona Supercomputing Center                    */
/*                                                                        */
/*      This file is part of the NANOS++ library.                         */
/*                                                                        */
/*      NANOS++ is free software: you can redistribute it and/or modify   */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or  */
/*      (at your option) any later version.                               */
/*                                                                        */
/*      NANOS++ is distributed in the hope that it will be useful,        */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of    */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     */
/*      GNU Lesser General Public License for more details.               */
/*                                                                        */
/*      You should have received a copy of the GNU Lesser General Public License  */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.  */
/**************************************************************************/

#include "smpdd.hpp"
#include "schedule.hpp"
#include "debug.hpp"
#include "system.hpp"
#include "smp_ult.hpp"
#include "instrumentation.hpp"
#include <string>
#include <unistd.h>

#ifdef NANOS_RESILIENCY_ENABLED
#include <stdint.h>
#include "taskexception.hpp"
#include "memcontroller_decl.hpp"

#ifdef NANOS_FAULT_INJECTION
#include "mpoison.h"
#endif
#endif

using namespace nanos;
using namespace nanos::ext;


SMPDevice nanos::ext::SMP("SMP");

size_t SMPDD::_stackSize = 32 * 1024;

/*!
 \brief Registers the Device's configuration options
 \param reference to a configuration object.
 \sa Config System
 */
void SMPDD::prepareConfig ( Config &config )
{
   /*!
    Get the stack size from system configuration
    */
   size_t size = sys.getDeviceStackSize();
   if (size > 0)
      _stackSize = size;

   /*!
    Get the stack size for this device
    */
   config.registerConfigOption ( "smp-stack-size", NEW Config::SizeVar( _stackSize ), "Defines SMP workdescriptor stack size" );
   config.registerArgOption("smp-stack-size", "smp-stack-size");
   config.registerEnvOption("smp-stack-size", "NX_SMP_STACK_SIZE");
}

void SMPDD::initStack ( WD *wd )
{
   _state = ::initContext(_stack, _stackSize, &workWrapper, wd,
         (void *) Scheduler::exit, 0);
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
   if (isUserLevelThread) {
      if (previous == NULL)
         _stack = NEW
         intptr_t[_stackSize];
      else {
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

void SMPDD::execute ( WD &wd ) throw ()
{
#ifdef NANOS_RESILIENCY_ENABLED
   unsigned num_tries = 0;

   if (wd.isInvalid() || (wd.getParent() != NULL && wd.getParent()->isInvalid())) {
      /*
       *  TODO Optimization?
       *  It could be better to skip this work if workdescriptor is flagged as invalid
       *  before allocating a new stack for the task and, perhaps,
       *  skip data copies of dependences.
       */
      wd.setInvalid(true);
      debug ( "Resiliency: Task " << wd.getId() << " is flagged as invalid. Skipping it.");

#ifdef NANOS_INSTRUMENTATION_ENABLED
      NANOS_INSTRUMENT ( static nanos_event_key_t task_discard_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("task-discard") );
      NANOS_INSTRUMENT ( nanos_event_value_t task_discard_val = (nanos_event_value_t ) wd.getId() );
      NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_discard_key, &task_discard_val) );
#endif

      sys.getExceptionStats().incrDiscardedTasks();
   } else {
      while (true) {
         try {
            // Call to the user function
            getWorkFct()( wd.getData() );
         } catch (nanos::TaskException& e) {
            debug("Resiliency: error detected during task " << wd.getId() << " execution.");
            sys.getExceptionStats().incrExecutionErrors();
            e.handleExecutionError( );
         } catch (std::exception& e) {
            std::string s = "Error: Uncaught exception ";
            s += typeid(e).name();
            s += ". Thrown in task ";
            s += wd.getId();
            s += ". \n";
            s += e.what();

            sys.getExceptionStats().incrExecutionErrors();
            if( sys.isSummaryEnabled() )
               sys.executionSummary();

            // Unexpected error: terminate execution
            fatal(s);
         } catch (...) {
            sys.getExceptionStats().incrExecutionErrors();
            // Unexpected error: terminate execution
            fatal("Error: Uncaught exception (unknown type). Thrown in task " << wd.getId() << ". ");
            if( sys.isSummaryEnabled() )
               sys.executionSummary();
         }

         /* 
          * A task is only re-executed when the following conditions meet:
          * 1) The execution was invalid
          * 2) The task is marked as recoverable
          * 3) The task parent is not invalid (it doesn't make sense to recover ourselves if our parent is going to undo our work)
          * 4) The task has not run out of trials (a limit is set to avoid infinite loop)
          */ 
         if ( wd.isInvalid() 
            && wd.isRecoverable() // Execution invalid and task recoverable
            && ( wd.getParent() == NULL || !wd.getParent()->isInvalid() ) // Our parent is not invalid (if we got one)
         ){
            if ( num_tries < sys.getTaskMaxRetries() ) {// We are still able to retry
               sys.getExceptionStats().incrRecoveredTasks();
               num_tries++;
               restore(wd);
            } else {
               // Giving up retrying...
               wd.getParent()->setInvalid( true );
               return;
            }
         } else {
            debug( "Exiting task " << wd.getId() << "." );
            // Nothing left to do, either task execution was OK or the recovery has to be done by an ancestor.
            return;
         }
         debug( "Task " << wd.getId() << " is being re-executed." );
#ifdef NANOS_INSTRUMENTATION_ENABLED
         NANOS_INSTRUMENT ( static nanos_event_key_t task_reexec_key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("task-reexecution") );
         NANOS_INSTRUMENT ( nanos_event_value_t task_reexec_val = (nanos_event_value_t ) wd.getId() );
         NANOS_INSTRUMENT ( sys.getInstrumentation()->raisePointEvents(1, &task_reexec_key, &task_reexec_val) );
#endif
      }
   }
#else
   // Workdescriptor execution
   getWorkFct()(wd.getData());
#endif
}

#ifdef NANOS_RESILIENCY_ENABLED
bool SMPDD::recover( TaskException const& err ) {
   bool result = true;

   static size_t page_size = sysconf(_SC_PAGESIZE);

   // Recover system from failures
   switch(err.getSignal()){
      case SIGSEGV:

         switch(err.getSignalInfo().si_code) {
            case SEGV_MAPERR: /* Address not mapped to object.  */
               // SEGV_MAPERR error recovery is not fully supported
               break;// case SEGV_MAPERR
            case SEGV_ACCERR: /* Invalid permissions for mapped object.  */
               uintptr_t page_addr = (uintptr_t)err.getSignalInfo().si_addr;
               // Align faulting address with virtual page address
               page_addr &= ~(page_size - 1);
#ifdef NANOS_FAULT_INJECTION
               if( sys.isPoisoningEnabled() ) {
                  if( mpoison_unblock_page(page_addr) == 0 ) {
                     debug("Resiliency: Page restored! Address: 0x" << std::hex << page_addr);
                     result = true;
                  } else {
                     debug("Resiliency: Error while restoring page. Address: 0x" << std::hex << page_addr);
                     result = false;
                  }
               } else {
                 result = false;// TODO Page unimplemented for real errors (not simulations)
               }
#else
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


void SMPDD::restore( WD & wd ) {
#ifdef NANOS_INSTRUMENTATION_ENABLED
   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("restore") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = wd.getId() );
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );
#endif
   debug ( "Resiliency: Task " << wd.getId() << " is being recovered to be re-executed further on.");
   // Wait for successors to finish.
   wd.waitCompletion();

   // Restore the data
   wd._mcontrol.restoreBackupData();

   while ( !wd._mcontrol.isDataRestored( wd ) ) {
      myThread->idle();
   }

   debug ( "Resiliency: Task " << wd.getId() << " recovery complete.");
   // Reset invalid state
   wd.setInvalid(false);

#ifdef NANOS_INSTRUMENTATION_ENABLED
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
#endif
}
#endif
