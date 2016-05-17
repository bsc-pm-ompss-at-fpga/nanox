
#ifndef ERROR_INJECTION_STUB_HPP
#define ERROR_INJECTION_STUB_HPP

#include "error-injection/errorinjectionconfiguration.hpp"
#include "error-injection/errorinjectionpolicy.hpp"

namespace nanos {
namespace error {

// This injetor does only support standalone mode
// It does not make any sense to use a more ellaborated policy
// if in the end it does not inject anything anyway
class StubInjector : public ErrorInjectionPolicy
{
	public:
		StubInjector( ErrorInjectionConfig const& properties ) noexcept :
			ErrorInjectionPolicy( properties )
		{
		}

		void injectError()
		{
		}

		void injectError( void* handle )
		{
		}

		void recoverError( void *handle ) noexcept
		{
		}

		void declareResource( void* handle, size_t size )
		{
		}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_STUB_HPP
