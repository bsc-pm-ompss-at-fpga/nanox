
#ifndef ERROR_INJECTION_INTERFACE_HPP
#define ERROR_INJECTION_INTERFACE_HPP

#include "errorinjectionconfiguration.hpp"
#include "errorinjectionplugin.hpp"
#include "errorinjectionpolicy.hpp"
#include "errorinjectionthread.hpp"
#include "frequency.hpp"
#include "system.hpp"

#include <memory>

namespace nanos {
namespace error {

class ErrorInjectionInterface {
private:
	class InjectionInterfaceSingleton {
		private:
			ErrorInjectionConfig properties;
			//std::unique_ptr<ErrorInjectionPlugin> plugin;
			ErrorInjectionPlugin* plugin;
			ErrorInjectionPolicy &policy;
			ErrorInjectionThread thread;

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
					properties(),
					plugin( reinterpret_cast<ErrorInjectionPlugin*>(
									sys.loadAndGetPlugin("error-injection-"+properties.getSelectedInjectorName() )
								) ),
					policy( plugin->getInjectionPolicy() ),
					thread( policy )
			{
				policy.config( properties );
			}

			virtual ~InjectionInterfaceSingleton()
			{
				delete plugin;
			}
	};

	static InjectionInterfaceSingleton interfaceObject;

public:
	//! Deterministically injects an error
	static void injectError( void* handle ) { interfaceObject.policy.injectError( handle ); }

	/*! Restore an injected error providing some hint
	 * of where to find it
	 */
	static void recoverError( void* handle ) noexcept { interfaceObject.policy.recoverError( handle ); }

	/*! Declares some resource that will be
	 * candidate for corruption using error injection
	 */
	static void declareResource(void* handle, size_t size ) { interfaceObject.policy.declareResource( handle, size ); }

	static void resumeInjection() { interfaceObject.thread.resume(); }

	static void stopInjection() { interfaceObject.thread.stop(); }

	static void terminateInjection() { interfaceObject.thread.terminate(); }
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_INTERFACE_HPP

