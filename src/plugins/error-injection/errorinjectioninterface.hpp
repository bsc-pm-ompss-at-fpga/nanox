
#ifndef ERROR_INJECTION_INTERFACE_HPP
#define ERROR_INJECTION_INTERFACE_HPP

#include "frequency.hpp"
#include "errorinjectionconfiguration.hpp"
#include "system.hpp"

class ErrorInjectionInterface {
private:
	class InjectionInterfaceSingleton {
		private:
			ErrorInjectionConfig properties;
			ErrorInjectionPolicy &policy;

		public:
			ErrorInjectionInterface() :
					properties(),
					policy( 
						sys.loadAndGetPlugin("error-injection-"+properties.getSelectedInjectorName())->getInjectionPolicy()
					 )
			{
				policy.config( properties );
			}

	};

	static InjectionInterfaceSingleton interfaceObject;

public:

	// Deterministically injects an error
	void injectError( void* handle ) { interfaceObject.policy.injectError( handle ); }

	// Restore an injected error providing some hint
	// of where to find it
	void recoverError( void* handle ) std::noexcept { interfaceObject.policy.recoverError( handle ); }

	// Declares some resource that will be
	// candidate for corruption using error injection
	void declareResource(void* handle, size_t size ) { interfaceObject.policy.declareResource( handle, size ); }

	void resumeInjection() { interfaceObject.policy.resume(); }

	void suspendInjection() { interfaceObject.policy.suspend(); }

	void terminateInjection() { interfaceObject.policy.terminate(); }

};
#endif // ERROR_INJECTION_INTERFACE_HPP

