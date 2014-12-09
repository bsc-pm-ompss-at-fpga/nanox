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

#ifndef _NANOS_TASKEXCEPTION_H
#define _NANOS_TASKEXCEPTION_H

#include "taskexception_fwd.hpp"

#include "workdescriptor_fwd.hpp"
#include <exception>
#include <signal.h>
#include <ucontext.h>

#include "atomic_decl.hpp"
#include "xstring.hpp"

namespace nanos {
   class InvalidatedRegionFound : public std::runtime_error {
      public:
         InvalidatedRegionFound () : runtime_error( "Tried to read a corrupted memory region." ) {}
         virtual ~InvalidatedRegionFound() {}
   };

   class TaskExceptionStats
   {
      private:
         Atomic<int> _errors_injected;
         Atomic<int> _errors_in_execution;
         Atomic<int> _errors_in_initialization;
         Atomic<int> _recovered_tasks;
         Atomic<int> _discarded_tasks;

         TaskExceptionStats ( TaskExceptionStats &tes );

         TaskExceptionStats& operator= ( TaskExceptionStats &tes );

      public:
         TaskExceptionStats () : _errors_in_execution(0), _errors_in_initialization(0), _recovered_tasks(0), _discarded_tasks(0) {}

         ~TaskExceptionStats () {}

         int getInjectedErrors() const { return _errors_injected.value(); }
         void incrInjectedErrors() { _errors_injected++; }

         int getExecutionErrors() const { return _errors_in_execution.value(); }
         void incrExecutionErrors() { _errors_in_execution++; }

         int getInitializationErrors() const { return _errors_in_initialization.value(); }
         void incrInitializationErrors() { _errors_in_initialization++; }

         int getRecoveredTasks() const { return _recovered_tasks.value(); }
         void incrRecoveredTasks() { _recovered_tasks++; }

         int getDiscardedTasks() const { return _discarded_tasks.value(); }
         void incrDiscardedTasks() { _discarded_tasks++; }
   };

   /*!
    * \class TaskException
    * \brief Contains usefull information about a runtime error generated in a task execution.
    */
   class TaskException: public std::exception
   {

      private:
         std::string error_msg; /*!< Description of the error that created this exception */
         WD* task;/*!< Pointer to the affected task */
         const siginfo_t signal_info;/*!< Detailed description after the member */
         const ucontext_t task_context;/*!< Detailed description after the member */

      public:

         /*!
          * Constructor for class TaskException
          * \param task a pointer to the task where the error appeared
          * \param info information about the signal raised
          * \param context contains the state of execution when the error appeared
          */
         TaskException ( WD *t, siginfo_t const &info,
                                  ucontext_t const &context ) throw ();

         TaskException ( WD *t, siginfo_t const &info,
                                  ucontext_t const &context,
                                  char** backtrace, size_t bt_size ) throw ();

         /*!
          * Copy constructor for class TaskException
          */
         TaskException ( TaskException const &tee ) throw ();

         /*!
          * Destructor for class TaskException
          */
         virtual ~TaskException ( ) throw ();

         /*!
          * Returns some information about the error in text format.
          */
         virtual const char* what ( ) const throw () { return error_msg.c_str(); };

         /*!
          * \return a pointer to the affected WorkDescriptor.
          */
         const WD* getFailedTask ( ) const;

         /*!
          * \return the raised signal number
          */
         int getSignal ( ) const;

         /*!
          * \return the structure containing the signal information.
          * \see siginfo_t
          */
         const siginfo_t getSignalInfo ( ) const;

         /*!
          * \return the structure conteining the execution status when the error appeared
          * \see ucontext_t
          */
         const ucontext_t getExceptionContext ( ) const;

         /*!
          * Common actions to be taken when an error raises in backup/restart operations.
          * \param initTask Task that was being initialized at the moment of the error. That task is invalidated.
          * \param srcAddr Contains the address where the data was being copied from.
          * \param destAddr Contains the address where the data was being copied to.
          * \return whether the checkpoint can be retried again safely or not.
          */
         bool handleCheckpointError ( WorkDescriptor const &initTask, uint64_t srcAddr, uint64_t destAddr, size_t len ) const;

         /*!
          * Common actions to be taken when an error raises in execution. This task is invalidated.
          */
         void handleExecutionError ( ) const;
   };
}

#endif /* _NANOS_TASKEXCEPTION_H */
