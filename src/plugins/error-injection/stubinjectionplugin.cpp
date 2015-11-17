
#include "stubinjector.hpp"
#include "errorinjectionplugin.hpp"
#include "system.hpp"

namespace nanos {
namespace error {

class StubInjectionPlugin : public ErrorInjectionPlugin
{
	private:
		StubInjector policy;

	public:
		StubInjectionPlugin() :
			ErrorInjectionPlugin(),
			policy()
		{
		}

		virtual ErrorInjectionPolicy &getInjectionPolicy() { return policy; }

		static const char* pluginName() { return "none"; }
};

}// namespace error
}// namespace nanos

DECLARE_PLUGIN( nanos::error::StubInjectionPlugin::pluginName(),
                nanos::error::StubInjectionPlugin
              );
