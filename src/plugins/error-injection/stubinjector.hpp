
#ifndef ERROR_INJECTION_STUB_HPP
#define ERROR_INJECTION_STUB_HPP

#include "errorinjectionpolicy.hpp"

namespace nanos {
namespace error {

class StubInjector : public ErrorInjectionPolicy
{
	public:
		StubInjector() noexcept :
			ErrorInjectionPolicy()
		{
		}

		void config( ErrorInjectionConfig const& properties )
		{
		}

		virtual void injectError()
		{
		}

		virtual void injectError( void* handle )
		{
		}

		virtual void recoverError( void *handle ) noexcept
		{
		}

		virtual void declareResource( void* handle, size_t size )
		{
		}

		virtual std::chrono::duration<float> getWaitTime() noexcept
		{
			return std::chrono::duration<float>(0);
		}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_STUB_HPP
