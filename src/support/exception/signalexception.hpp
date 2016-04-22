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

#ifndef SIGNAL_EXCEPTION_HPP
#define SIGNAL_EXCEPTION_HPP

#include "genericexception.hpp"
#include "signaltranslator.hpp"
#include "signalinfo.hpp"

#include <signal.h>
#include <pthread.h>

#include <exception>

namespace nanos {
namespace error {

class SignalException : public GenericException {
	private:
		SignalInfo       _handledSignalInfo;
		ExecutionContext _executionContextWhenHandled;

	public:
		SignalException( siginfo_t* signalInfo, ucontext_t* executionContext, std::string const& message ) :
				GenericException( message ),
				_handledSignalInfo( *signalInfo ),
				_executionContextWhenHandled( *executionContext )
		{}

		/* \brief Deallocates resources and restores 
		 * \details In addition to freeing the memory used for saving signal structs, 
		 * it also unblocks the signal when the exception is deleted.
		 * Deletion is usually performed at the end of the catch block, unless
		 * the exception is thrown through the heap (catch by pointer), where the user 
		 * is responsible for calling the delete operation explicitly.
		 */
		virtual ~SignalException() noexcept {
			sigset_t thisSignalMask;
			sigemptyset(&thisSignalMask);
			sigaddset(&thisSignalMask, _handledSignalInfo.getSignalNumber());
			pthread_sigmask(SIG_UNBLOCK, &thisSignalMask, NULL);
		}

		SignalInfo const& getSignalInfo() const { return _handledSignalInfo; }

		ExecutionContext const& getExecutionContext() { return _executionContextWhenHandled; }

};

class SegmentationFaultException : public SignalException {
	private:
		std::string getErrorMessage() const;
	public:
		static int getSignalNumber() { return SIGSEGV; }

		SegmentationFaultException( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				SignalException( signalInfo, executionContext, getErrorMessage() )
		{
		}

		virtual ~SegmentationFaultException() noexcept
		{
		}
};

class BusErrorException : public SignalException {
	private:
		std::string getErrorMessage() const;
	public:
		static int getSignalNumber() { return SIGBUS; }

		BusErrorException( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				SignalException( signalInfo, executionContext, getErrorMessage()  )
		{
		}

		virtual ~BusErrorException() noexcept
		{
		}
};

} // namespace error
} // namespace nanos

#endif // SIGNAL_EXCEPTION_HPP

