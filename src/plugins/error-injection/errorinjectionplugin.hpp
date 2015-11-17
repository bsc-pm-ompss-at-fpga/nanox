
#ifndef ERROR_INJECTION_PLUGIN_HPP
#define ERROR_INJECTION_PLUGIN_HPP

#include "errorinjectionpolicy.hpp"
#include "plugin.hpp"

namespace nanos {
namespace error {

class ErrorInjectionPlugin : public Plugin
{
   public:
      ErrorInjectionPlugin() : 
				Plugin( "ErrorInjectionPlugin", 1 )
		{
		}

		virtual ErrorInjectionPolicy &getInjectionPolicy() = 0;
};

}// namespace error
}// namespace nanos

#endif // ERROR_INJECTION_PLUGIN_HPP

