
#ifndef ERROR_INJECTION_POLICY_HPP
#define ERROR_INJECTION_POLICY_HPP

#include "errorinjectionconfiguration.hpp"

#include <chrono>
#include <ratio>

namespace nanos {
namespace error {

class ErrorInjectionPolicy {
	public:
		ErrorInjectionPolicy() noexcept
		{
		}

		virtual ~ErrorInjectionPolicy() noexcept
		{
		}

		virtual void config( ErrorInjectionConfig const& properties )
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

		// Returns the time to wait until the injection thread
		// performs the next injection
		virtual std::chrono::duration<float> getWaitTime() noexcept = 0;
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_POLICY_HPP
