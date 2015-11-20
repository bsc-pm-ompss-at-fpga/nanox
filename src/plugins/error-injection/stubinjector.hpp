
#ifndef ERROR_INJECTION_STUB_HPP
#define ERROR_INJECTION_STUB_HPP

#include "error-injection/errorinjectionpolicy.hpp"

namespace nanos {
namespace error {

class StubInjector : public ErrorInjectionPolicy
{
	public:
		StubInjector( ErrorInjectionConfig const& properties ) noexcept :
			ErrorInjectionPolicy( properties )
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
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_STUB_HPP
