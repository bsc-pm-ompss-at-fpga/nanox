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

#include "taskexecutionexception.hpp"
#include "workdescriptor.hpp"
#include "system_decl.hpp"

namespace nanos {

TaskExecutionException::TaskExecutionException (
      WD *t, siginfo_t const &info,
      ucontext_t const &context ) throw () :
      task(t), signal_info(info), task_context(context)
{
   std::stringstream ss;
   ss << "Signal raised during the execution of task "
      << t->getId() 
      << std::endl;

   const char* sig_desc;
   if (signal_info.si_signo >= 0 && signal_info.si_signo < NSIG && (sig_desc =
         _sys_siglist[signal_info.si_signo]) != NULL) {

      ss << sig_desc;
      switch (signal_info.si_signo) {
         // Check {glibc_include_path}/bits/{siginfo.h, signum.h}
         case SIGILL:
            switch (signal_info.si_code) {
               case ILL_ILLOPC:
                  ss << " Illegal opcode.";
                  break;
               case ILL_ILLOPN:
                  ss << " Illegal operand.";
                  break;
               case ILL_ILLADR:
                  ss << " Illegal addressing mode.";
                  break;
               case ILL_ILLTRP:
                  ss << " Illegal trap.";
                  break;
               case ILL_PRVOPC:
                  ss << " Privileged opcode.";
                  break;
               case ILL_PRVREG:
                  ss << " Privileged register.";
                  break;
               case ILL_COPROC:
                  ss << " Coprocessor error.";
                  break;
               case ILL_BADSTK:
                  ss << " Internal stack error.";
                  break;
            }

            break;
         case SIGFPE:
            switch (signal_info.si_code) {

               case FPE_INTDIV:
                  ss << " Integer divide by zero.";
                  break;
               case FPE_INTOVF:
                  ss << " Integer overflow.";
                  break;
               case FPE_FLTDIV:
                  ss << " Floating-point divide by zero.";
                  break;
               case FPE_FLTOVF:
                  ss << " Floating-point overflow.";
                  break;
               case FPE_FLTUND:
                  ss << " Floating-point underflow.";
                  break;
               case FPE_FLTRES:
                  ss << " Floating-poing inexact result.";
                  break;
               case FPE_FLTINV:
                  ss << " Invalid floating-point operation.";
                  break;
               case FPE_FLTSUB:
                  ss << " Subscript out of range.";
                  break;
            }
            break;
         case SIGSEGV:
            switch (signal_info.si_code) {

               case SEGV_MAPERR:
                  ss << " Address not mapped to object.";
                  break;
               case SEGV_ACCERR:
                  ss << " Invalid permissions for mapped object.";
                  break;
            }
            break;
         case SIGBUS:
            switch (signal_info.si_code) {

               case BUS_ADRALN:
                  ss << " Invalid address alignment.";
                  break;
               case BUS_ADRERR:
                  ss << " Nonexisting physical address.";
                  break;
               case BUS_OBJERR:
                  ss << " Object-specific hardware error.";
                  break;
#ifdef BUS_MCEERR_AR
                  case BUS_MCEERR_AR: //(since Linux 2.6.32)
                  ss << " Hardware memory error consumed on a machine check; action required.";
                  break;
#endif
#ifdef BUS_MCEERR_AO
                  case BUS_MCEERR_AO: //(since Linux 2.6.32)
                  ss << " Hardware memory error detected in process but not consumed; action optional.";
                  break;
#endif
            }
            break;
         case SIGTRAP:
            switch (signal_info.si_code) {

               case TRAP_BRKPT:
                  ss << " Process breakpoint.";
                  break;
               case TRAP_TRACE:
                  ss << " Process trace trap.";
                  break;
            }
            break;

            //default:
            /*
             * note #1: since this exception is going to be thrown by the signal handler
             * only synchronous signals information will be printed, as the remaining
             * are unsupported by -fnon-call-exceptions
             */
      }
   } else {
      /*
       * See note #1
       */
      ss << " Unsupported signal (" << signal_info.si_signo << " )";
   }
   error_msg = ss.str();
}

TaskExecutionException::TaskExecutionException (
      TaskExecutionException const &tee ) throw () :
      error_msg(tee.error_msg), task(tee.task), signal_info(tee.signal_info), task_context(
            tee.task_context)
{

}

TaskExecutionException::~TaskExecutionException ( ) throw ()
{
   /*
    * Note that this destructor does not delete the WorkDescriptor object pointed by 'task'.
    * This is because that object's life does not finish at this point and,
    * thus, it may be necessary to access it later.
    */
}

const WD* TaskExecutionException::getFailedTask ( ) const
{
   return task;
}

int TaskExecutionException::getSignal ( ) const
{
   return signal_info.si_signo;
}

const siginfo_t TaskExecutionException::getSignalInfo ( ) const
{
   return signal_info;
}

const ucontext_t TaskExecutionException::getExceptionContext ( ) const
{
   return task_context;
}

void TaskExecutionException::handle ( ) const
{
   /*
    * When a signal handler is executing, the delivery of the same signal
    * is blocked, and it does not become unblocked until the handler returns.
    * In this case, it will not become unblocked since the handler is exited
    * through an exception: it should be explicitly unblocked.
    */
   sigset_t sigs;
   sigemptyset(&sigs);
   sigaddset(&sigs, signal_info.si_signo);
   pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);

   bool recoverable_error = false;
   if( task->started() ) {
      // error detected in task execution: invalidate task
      recoverable_error = task->setInvalid(true);
      sys.getExceptionStats().incrExecutionErrors();
      debug("Resiliency: error detected during task " << task->getId() << " execution.");
   } else {
      // error detected in task initialization: invalidate task AND its parent
      // if it has no recoverable parent, then the execution cannot continue
      recoverable_error = task->setInvalid(true) &&
                          task->getParent() &&
                          task->getParent()->setInvalid(true);
      sys.getExceptionStats().incrInitializationErrors();
      debug("Resiliency: error detected during task " << task->getId() << " initialization.");
   }

   if( !recoverable_error )  
   {
       message("An error was found, but there isn't any recoverable ancestor.");
       message( what() );
       // Unrecoverable error: terminate execution
       std::terminate();
   } else {
      debug( what() );
   }

   // Try to recover the system from the failure (so we can continue with the execution)
   if(!task->getActiveDevice().recover( *this )) {
      // If we couldn't recover the system, we can't go on with the execution
      message("Resiliency: Unrecoverable error found. ");
      message( what() );
      std::terminate();
   }
}

}//namespace nanos

