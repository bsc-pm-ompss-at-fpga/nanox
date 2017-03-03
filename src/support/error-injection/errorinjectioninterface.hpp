
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
			ErrorInjectionPolicy* _policy;

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

	static InjectionInterfaceSingleton& getInterface () {
        static InjectionInterfaceSingleton interfaceObject;
        return interfaceObject;
    }

public:
	//! Deterministically injects an error
	static void injectError( void* handle )
	{
		getInterface()._policy->injectError( handle );
	}

	/*! Restore an injected error providing some hint
	 * of where to find it
	 */
	static void recoverError( void* handle ) noexcept
	{
		getInterface()._policy->recoverError( handle );
	}

	/*! Declares some resource that will be
	 * candidate for corruption using error injection
	 */
	static void declareResource(void* handle, size_t size )
	{
		getInterface()._policy->declareResource( handle, size );
	}

	static void resumeInjection()
	{
		getInterface()._policy->resume();
	}

	static void stopInjection()
	{
		getInterface()._policy->stop();
	}

	static void terminateInjection()
	{
		delete(getInterface()._policy);
		getInterface()._policy = nullptr;
	}

	static void setInjectionPolicy( ErrorInjectionPolicy *selectedPolicy ) 
	{
		getInterface()._policy = selectedPolicy;
	}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_INTERFACE_HPP

