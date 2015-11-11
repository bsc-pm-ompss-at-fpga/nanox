
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
		siginfo_t savedInfo;
	public:
		SignalInfo( siginfo_t * info ) :
			savedInfo( *info )
		{}

};
