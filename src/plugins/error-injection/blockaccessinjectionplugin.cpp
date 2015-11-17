
#include "blockaccessinjector.hpp"
#include "errorinjectionplugin.hpp"
#include "system.hpp"

namespace nanos {
namespace error {

class BlockPageAccessInjectionPlugin : public ErrorInjectionPlugin
{
	private:
		BlockMemoryPageAccessInjector policy;

	public:
		BlockPageAccessInjectionPlugin() :
			ErrorInjectionPlugin(),
			policy()
		{
		}

		virtual ErrorInjectionPolicy &getInjectionPolicy() { return policy; }

		static const char* pluginName() { return "block-access"; }
};

}// namespace resiliency
}// namespace nanos

DECLARE_PLUGIN( nanos::error::BlockPageAccessInjectionPlugin::pluginName(),
                nanos::error::BlockPageAccessInjectionPlugin
              );
