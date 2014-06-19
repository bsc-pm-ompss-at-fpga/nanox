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

#ifndef _NANOS_TASKEXECUTIONEXCEPTION
#define _NANOS_TASKEXECUTIONEXCEPTION

#include "taskexecutionexception_decl.hpp"
#include "xstring.hpp"

namespace nanos {
TaskExecutionException::TaskExecutionException (
      WD const *task_wd, siginfo_t const &info,
      ucontext_t const &context ) throw () :
      task(task_wd), signal_info(info), task_context(context)
{
   std::stringstream ss("Signal raised during the execution of task ");
   ss << task_wd->getId() << std::endl;

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
    * thus, it will be accessed later.
    */
}

inline int TaskExecutionException::getSignal ( )
{
   return signal_info.si_signo;
}

inline const siginfo_t TaskExecutionException::getSignalInfo ( ) const
{
   return signal_info;
}

inline const ucontext_t TaskExecutionException::getExceptionContext ( ) const
{
   return task_context;
}
}

#endif /* _NANOS_TASKEXECUTIONEXCEPTION */
