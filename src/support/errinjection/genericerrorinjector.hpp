
#include "errorinjectionthread.hpp"

#include <chrono>

class ErrorInjectionPolicy {
	private:
		ErrorInjectionThread thread;

	public:
		ErrorInjector() :
			thread( *this )
		{
		}

		virtual void injectError() = 0;
		
		virtual void recoverError( void* handle ) = 0;

		virtual std::chrono::milliseconds const& getWaitTime() = 0;
}
