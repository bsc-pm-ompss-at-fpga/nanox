
#ifndef SIGNALEXCEPTION_HPP
#define SIGNALEXCEPTION_HPP

#include "exceptiontracer.hpp"
#include "signaltranslator.hpp"
#include "signalinfo.hpp"
#include "exception/genericexception.hpp"

#include <signal.h>
#include <pthread.h>

#include <exception>

namespace nanos {
namespace error {

class SignalException : public GenericException {
	private:
		SignalInfo       _handledSignalInfo;
		ExecutionContext _executionContextWhenHandled;

	protected:
		SignalInfo const& getHandledSignalInfo() const { return _handledSignalInfo; }

		ExecutionContext const& getExecutionContextWhenHandled() { return _executionContextWhenHandled; }
	public:
		SignalException( siginfo_t* signalInfo, ucontext_t* executionContext, std::string const& message ) :
				GenericException( message ),
				_handledSignalInfo( signalInfo ), 
				_executionContextWhenHandled( executionContext ) 
		{}

		virtual ~SignalException() noexcept {
			// Unblock the signal when the exception is deleted
			// Deletion should be performed at the end of the catch block.
			sigset_t thisSignalMask;
			sigemptyset(&thisSignalMask);
			sigaddset(&thisSignalMask, getHandledSignalInfo().getSignalNumber());
			pthread_sigmask(SIG_UNBLOCK, &thisSignalMask, NULL);
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

#endif

