
#ifndef ERROR_INJECTION_INTERFACE_HPP
#define ERROR_INJECTION_INTERFACE_HPP

#include "errorinjectionconfiguration.hpp"
#include "errorinjectionpolicy.hpp"

#include <memory>

namespace nanos {
namespace error {

class ErrorInjectionInterface {
private:
	class InjectionInterfaceSingleton {
		private:
			std::unique_ptr<ErrorInjectionPolicy> _policy;

			friend class ErrorInjectionInterface;
		public:
			/*! Default constructor
			 * \details
			 * 	1) Create a properties object (config) that reads the environment 
			 * 	searching for user defined configurations.
			 * 	2) Loads a user-defined error injection plugin (or a stub, if nothing is defined).
			 * 	3) Read the injection policy from the error injection plugin, that will be used by the thread.
			 * 	4) Instantiate the thread that will perform the injection.
			 */
			InjectionInterfaceSingleton() :
					_policy( nullptr )
			{
			}

			virtual ~InjectionInterfaceSingleton()
			{
			}
	};

	static InjectionInterfaceSingleton interfaceObject;

public:
	//! Deterministically injects an error
	static void injectError( void* handle )
	{
		interfaceObject._policy->injectError( handle );
	}

	/*! Restore an injected error providing some hint
	 * of where to find it
	 */
	static void recoverError( void* handle ) noexcept
	{
		interfaceObject._policy->recoverError( handle );
	}

	/*! Declares some resource that will be
	 * candidate for corruption using error injection
	 */
	static void declareResource(void* handle, size_t size )
	{
		interfaceObject._policy->declareResource( handle, size );
	}

	static void resumeInjection()
	{
		interfaceObject._policy->resume();
	}

	static void stopInjection()
	{
		interfaceObject._policy->stop();
	}

	static void terminateInjection()
	{
		interfaceObject._policy.reset(nullptr);
	}

	static void setInjectionPolicy( ErrorInjectionPolicy *selectedPolicy ) 
	{
		interfaceObject._policy.reset( selectedPolicy );
	}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_INTERFACE_HPP

