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

#ifdef NANOS_RESILIENCY_ENABLED
#include <stdint.h>
#include "taskexecutionexception.hpp"
#include "memcontroller_decl.hpp"

#ifdef HAVE_CXX11
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
   bool retry = false;
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

      sys.getExceptionStats().incrDiscardedTasks();
   } else {
      while (true) {
         try {
            // Call to the user function
            getWorkFct()( wd.getData() );
         } catch (nanos::TaskExecutionException& e) {
            e.handle();
         } catch (std::exception& e) {
            std::string s = "Error: Uncaught exception ";
            s += typeid(e).name();
            s += ". Thrown in task ";
            s += wd.getId();
            s += ". \n";
            s += e.what();
            // Unexpected error: terminate execution
            fatal(s);
         } catch (...) {
            // Unexpected error: terminate execution
            fatal("Error: Uncaught exception (unknown type). Thrown in task " << wd.getId() << ". ");
         }
         // Only retry when ...
         retry = wd.isInvalid()// ... the execution failed,
         && wd.isRecoverable()//  ... the task is able to recover (pragma)
         && (wd.getParent() == NULL || !wd.getParent()->isInvalid())// and there is not an invalid parent.
         && num_tries < sys.getTaskMaxRetries();// This last condition avoids unlimited re-execution.

         if (!retry)
         break;

         // This is exceuted only on re-execution
         num_tries++;
         restore(wd);

         sys.getExceptionStats().incrRecoveredTasks();
      }
   }
#else
   // Workdescriptor execution
   getWorkFct()(wd.getData());
#endif
}

#ifdef NANOS_RESILIENCY_ENABLED
bool SMPDD::recover( TaskExecutionException const& err ) {
   bool result = true;

   static size_t page_size = sysconf(_SC_PAGESIZE);

   // Recover system from failures
   switch(err.getSignal()){
      case SIGSEGV:

         switch(err.getSignalInfo().si_code) {
            case SEGV_MAPERR: /* Address not mapped to object.  */
               debug("Resiliency: SEGV_MAPERR error recovery is still not supported.")
               break;// case SEGV_MAPERR
            case SEGV_ACCERR: /* Invalid permissions for mapped object.  */
               bool restored = false;
               uintptr_t page_addr = (uintptr_t)err.getSignalInfo().si_addr;
               // Align faulting address with virtual page address
               page_addr &= ~(page_size - 1);
#ifdef HAVE_CXX11
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
}
#endif
