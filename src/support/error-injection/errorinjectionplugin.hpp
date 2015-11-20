
#ifndef ERROR_INJECTION_PLUGIN_HPP
#define ERROR_INJECTION_PLUGIN_HPP

#include "errorinjectioninterface.hpp"
#include "errorinjectionpolicy.hpp"
#include "plugin.hpp"

#include <memory>

namespace nanos {
namespace error {

template < typename InjectionPolicy >
class ErrorInjectionPlugin : public Plugin
{
	private:
		ErrorInjectionConfig _injectionProperties;

   public:
      ErrorInjectionPlugin() : 
				Plugin( "ErrorInjectionPlugin", 1 ),
				_injectionProperties()
		{
		}

		void config( Config &properties ) {
			_injectionProperties.config( properties );
		}

		void init()
		{
			ErrorInjectionInterface::setInjectionPolicy( 
					new InjectionPolicy( _injectionProperties )
				);
		}
};

}// namespace error
}// namespace nanos

#endif // ERROR_INJECTION_PLUGIN_HPP

