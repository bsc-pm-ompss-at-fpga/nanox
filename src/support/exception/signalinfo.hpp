
#include <ucontext.h>

class ExecutionContext {
	private:
		ucontext_t savedContext;
	public:
		ExecutionContext( ucontext_t * context ) :
			savedContext( *context)
		{}

		/* TODO: finish ucontext encapsulation */
		ucontext_t &get() { return savedContext; }
};

class SignalInfo {
	private:
		siginfo_t _info;
	public:
		SignalInfo( siginfo_t * info ) :
			_info( *info )
		{}

		int getSignalNumber() const { return _info.si_signo; }

		int getSignalCode() const { return _info.si_code; }
};
