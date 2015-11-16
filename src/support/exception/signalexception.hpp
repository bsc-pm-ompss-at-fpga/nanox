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

#include "exceptiontracer.hpp"
#include "signaltranslator.hpp"
#include "signalinfo.hpp"

#include <signal.h>
#include <pthread.h>

#include <exception>

namespace nanos {
namespace error {

class SignalException : public GenericException {
	private:
		SignalInfo handledSignalInfo;
		ExecutionContext executionContextWhenHandled;

	protected:
		void setErrorMessage( std::string const& message ) { errorMessage = message; }

		SignalInfo const& getHandledSignalInfo() const { return handledSignalInfo; }

		ExecutionContext const& getExecutionContextWhenHandled() { return executionContextWhenHandled; }

	public:
		SignalException( siginfo_t* signalInfo, ucontext_t* executionContext, std::string const& errorMessage ) :
				GenericException( errorMessage ),
				handledSignalInfo( signalInfo ), 
				executionContextWhenHandled( executionContext ) 
		{}

		virtual ~SignalException() {
			// Unblock the signal when the exception is deleted
			// Deletion should be performed at the end of the catch block.
			sigset_t thisSignalMask;
			sigemptyset(&thisSignalMask);
			sigaddset(&thisSignalMask, handledSignalInfo.getSignalNumber());
			pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);
		}
};

class SegmentationFaultException : public SignalException {
	private:
		std::string getErrorMessage() const;
	public:
		static int getSignalNumber() { return SIGSEGV; }

		SegmentationFaultException( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				SignalException( signalInfo, executionContext, getErrorMessage() )
		{}
};

class BusErrorException : public SignalException {
	private:
		std::string getErrorMessage() const;
	public:
		static int getSignalNumber() { return SIGBUS; }

		BusErrorException( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				SignalException( signalInfo, executionContext, getErrorMessage()  )
		{}
};

} // namespace error
} // namespace nanos

#endif // SIGNAL_EXCEPTION_HPP

