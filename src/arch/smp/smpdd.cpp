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
#include "schedule.hpp"
#include "debug.hpp"
#include "system.hpp"
#include "smp_ult.hpp"
#include "instrumentation.hpp"
#include "taskexecutionexception.hpp"
#include <string>
#include <signal.h>
//#include "errorgen.hpp"

using namespace nanos;
using namespace nanos::ext;

SMPDevice nanos::ext::SMP("SMP");

size_t SMPDD::_stackSize = 256*1024;

//Atomic<bool> error_injected = false;

//! \brief Registers the Device's configuration options
//! \param reference to a configuration object.
//! \sa Config System
void SMPDD::prepareConfig ( Config &config )
{
   //! \note Get the stack size from system configuration
   size_t size = sys.getDeviceStackSize();
   if (size > 0) _stackSize = size;

   //! \note Get the stack size for this specific device
   config.registerConfigOption ( "smp-stack-size", NEW Config::SizeVar( _stackSize ), "Defines SMP::task stack size" );
   config.registerArgOption("smp-stack-size", "smp-stack-size");
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
   verbose0("Task " << wd.getId() << " initialization"); 
   if (isUserLevelThread) {
      if (previous == NULL) {
         _stack = (void *) NEW char[_stackSize];
         verbose0("   new stack created: " << _stackSize << " bytes");
      } else {
         verbose0("   reusing stacks");
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

void SMPDD::execute ( WD &wd ) //throw ( )
{
#ifdef NANOS_RESILIENCY_ENABLED
   bool retry = false;
   int num_tries = 0;
   if (wd.isInvalid() || (wd.getParent() != NULL && wd.getParent()->isInvalid())) {
      /*
       *  TODO Optimization?
       *  It could be better to skip this work if workdescriptor is flagged as invalid
       *  before allocating a new stack for the task and, perhaps,
       *  skip data copies of dependences.
       */
      wd.setInvalid(true);
      debug ( "Task " << wd.getId() << " is flagged as invalid.");
   }
   else {
       //bool taskFailed = false;
      while (true) {
         try {
            if( wd.getParent() != NULL && wd.getResilienceNode() == NULL )
               fatal( "WD has not RN." );
            if( wd.isCheckpoint() && wd.isSideEffect() )
               fatal( "The same task cannot be checkpoint and side_effect. (2)" );
            if( wd.getResilienceNode() == NULL && ( wd.isCheckpoint() || wd.isSideEffect() ) )
               fatal( "This kind of tasks must have ResilienceNode but this task has no ResilienceNode." );

            if( wd.isCheckpoint() ) {
               if( wd.getResilienceNode()->isComputed() ) {
                  if( wd.getResilienceNode()->getDataIndex() <= 0 ) { 
                     //std::cout << "Rank " << sys._rank << " wd " << wd.getId() << " is checkpoint and computed and has no data --> skip." << std::endl;
                     break;
                  }
                  else {
                     //std::cout << "Rank " << sys._rank << " wd " << wd.getId() << " is checkpoint and computed and has data --> loadInput." << std::endl;
                     NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
                     NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("resilience"); )
                     NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, RESILIENCE_LOAD_INPUT ); )
                     wd.getResilienceNode()->loadInput( wd.getCopies(), wd.getNumCopies(), wd.getId() );
                     sys.getResiliencePersistence()->disableRestore();
                     NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, 0 ); )
                  }
               }
               else if( wd.getNumCopies() > 0 ) { 
                  //std::cout << "Rank " << sys._rank << " wd " << wd.getId() << " is checkpoint and is not computed --> storeInput." << std::endl;
                  NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
                  NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("resilience"); )
                  NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, RESILIENCE_STORE_INPUT ) ; )
                  wd.getResilienceNode()->storeInput( wd.getCopies(), wd.getNumCopies(), wd.getId() );
                  NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, 0 ); )
               }
            }

            if( wd.isSideEffect() ) {
               if( wd.getResilienceNode()->isComputed() ) {
                  //std::cout << "Rank " << sys._rank << " wd " << wd.getId() << " is side_effect and is computed --> loadOutput." << std::endl;
                  NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
                  NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("resilience"); )
                  NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, RESILIENCE_LOAD_OUTPUT ) ; )
                  wd.getResilienceNode()->loadOutput( wd.getCopies(), wd.getNumCopies(), wd.getId() );
                  NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, 0 ); )
                  break;
               }
               else {
                  //std::cout << "Rank " << sys._rank << " wd " << wd.getId() << " is side_effect and is not computed --> storeOutput." << std::endl;
                  sys._resilienceCriticalRegion++;
                  //std::cerr << "RANK " << sys._rank << " will store output of RN "
                  //    << wd.getResilienceNode() - sys.getResiliencePersistence()->getResilienceNode(1) << "." << std::endl;
               }
            }
            else if( wd.getParent() != NULL && sys.getResiliencePersistence()->restore() ) break;

            getWorkFct()( wd.getData() );

            if( wd.isSideEffect() ) {
               sys._resilienceCriticalRegion--;
               //std::cerr << "RANK " << sys._rank << " has stored output of RN "
               //    << wd.getResilienceNode() - sys.getResiliencePersistence()->getResilienceNode(1) << "." << std::endl;
            }

            // Introduce errors only in created tasks, not in implicit tasks.
            if( /*error_injected == false &&*/ ( wd.getParent() != NULL && sys.faultInjectionThreshold() ) ) { 
               int totalTasks = sys.getSchedulerStats().getTotalTasks();
               int createdTasks = sys.getSchedulerStats().getCreatedTasks(); 
               int executedTasks =  createdTasks-totalTasks;
               if( executedTasks >= sys.faultInjectionThreshold() ) {
                  //error_injected = true;
                  while( sys._resilienceCriticalRegion.value() > 0 ) { }
                  throw std::runtime_error( "Injected error." );
               }
            }

         } catch (TaskExecutionException& e) {
            //taskFailed = true;
            /*
             * When a signal handler is executing, the delivery of the same signal
             * is blocked, and it does not become unblocked until the handler returns.
             * In this case, it will not become unblocked since the handler is exited
             * through an exception: it should be explicitly unblocked.
             */
            sigset_t sigs;
            sigemptyset(&sigs);
            sigaddset(&sigs, e.getSignal());
            pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);

            std::stringstream ss;
            ss << e.what() << std::endl << std::endl << std::endl;
            message( ss.str() );
            //message(e.what());
            // Unrecoverable error: terminate execution
            std::terminate();
         } catch (std::exception& e) {
            //taskFailed = true;
            std::stringstream ss;
            ss << "RANK " << sys._rank << ". "
               << "Uncaught exception "
               << typeid(e).name()
               << ". Thrown in task "
               << wd.getId()
               << ". \n"
               << e.what();
            message(ss.str());
            // Unexpected error: terminate execution
            std::terminate();
         } catch (...) {
            //taskFailed = true;
            message("Uncaught exception (unknown type). Thrown in task " << wd.getId() << ". ");
            // Unexpected error: terminate execution
            std::terminate();
         }
         // Only retry when ...
         retry = wd.isInvalid()// ... the execution failed,
         && wd.isRecoverable()// and the task is able to recover (pragma),
         && (wd.getParent() == NULL || !wd.getParent()->isInvalid())// and there is not an invalid parent.
         && num_tries < sys.getTaskMaxRetries();// This last condition avoids unbounded re-execution.

         if (!retry) 
             break;

         // This is executed only on re-execution
         num_tries++;
         recover(wd);
      }
   }
#else
   // Workdescriptor execution
   getWorkFct()(wd.getData());
#endif
}

#ifdef NANOS_RESILIENCY_ENABLED
void SMPDD::recover( WD & wd ) {
   std::cerr << "Recovering " << &wd << std::endl;
   // Wait for successors to finish.
   wd.waitCompletion();

   // Do anything ...

   // Reset invalid state
   wd.setInvalid(false);
}
#endif
