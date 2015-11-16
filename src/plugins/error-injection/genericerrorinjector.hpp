
#include "errorinjectionthread.hpp"

#include <chrono>

class ErrorInjectionPolicy {
	private:
		ErrorInjectionThread thread;

	public:
		ErrorInjector() std::noexcept :
			thread( *this )
		{
		}

		virtual ~ErrorInjectionPolicy std::noexcept
		{
		}

		// Tells the injection thread to start injecting errors
		virtual void resume() { thread.resume(); }

		// Tells the injection thread to stop injecting threads
		virtual void suspend() { thread.stop(); }

		// Tells the injection thread to finish its execution
		virtual void terminate() { thread.terminate(); }

		// Randomly injects an error
		// Automatically called by the injection thread
		virtual void injectError() = 0;

		// Deterministically injects an error
		// Might be called by the user through the API
		virtual void injectError( void* handle ) = 0;

		// Restore an injected error providing some hint
		// of where to find it
		virtual void recoverError( void* handle ) std::noexcept = 0;

		// Declares some resource that will be
		// candidate for corruption using error injection
		virtual void declareResource(void* handle, size_t size ) = 0;

		// Returns the time to wait until the injection thread
		// performs the next injection
		virtual std::chrono::seconds<float> const& getWaitTime() std::noexcept = 0;
};

