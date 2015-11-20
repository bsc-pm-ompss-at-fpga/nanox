
#ifndef ERROR_INJECTION_POLICY_HPP
#define ERROR_INJECTION_POLICY_HPP

#include "error-injection/errorinjectionconfiguration.hpp"

#include <chrono>
#include <random>
#include <ratio>

namespace nanos {
namespace error {

class ErrorInjectionPolicy {
	public:
		ErrorInjectionPolicy( ErrorInjectionConfig const& properties ) noexcept
		{
		}

		virtual ~ErrorInjectionPolicy() noexcept
		{
		}

		// Randomly injects an error
		// Automatically called by the injection thread
		virtual void injectError() = 0;

		// Deterministically injects an error
		// Might be called by the user through the API
		virtual void injectError( void* handle ) = 0;

		// Restore an injected error providing some hint
		// of where to find it
		virtual void recoverError( void* handle ) noexcept = 0;

		// Declares some resource that will be
		// candidate for corruption using error injection
		virtual void declareResource(void* handle, size_t size ) = 0;

		virtual void stop() {}

		virtual void resume() {}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_POLICY_HPP
