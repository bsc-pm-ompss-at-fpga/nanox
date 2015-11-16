
#ifndef ERROR_INJECTION_PLUGIN_HPP
#define ERROR_INJECTION_PLUGIN_HPP

namespace nanos {
namespace resiliency {

class ErrorInjectionPlugin : public Plugin
{
   public:
      ErrorInjectionPlugin() : 
				Plugin( "InjectionPlugin", 1 ),
		{
		}

		virtual ErrorInjectionPolicy &getInjectionPolicy() = 0;
};

}// namespace resiliency
}// namespace nanos

#endif // ERROR_INJECTION_PLUGIN_HPP

