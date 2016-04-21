
#include "blockaccessinjector.hpp"
#include "error-injection/errorinjectionplugin.hpp"
#include "system.hpp"

using namespace nanos::error;

class BlockPageAccessInjectionPlugin : public ErrorInjectionPlugin<BlockMemoryPageAccessInjector>
{
	public:
		static const char* pluginName() { return "injection-block-access"; }
};

DECLARE_PLUGIN( BlockPageAccessInjectionPlugin::pluginName(),
                BlockPageAccessInjectionPlugin
              );

