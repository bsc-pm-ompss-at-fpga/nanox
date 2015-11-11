
#include "exceptiontracer.hpp"
#include "signaltranslator.hpp"
#include "signalinfo.hpp"

#include <signal.h>
#include <pthread.h>

#include <exception>

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
