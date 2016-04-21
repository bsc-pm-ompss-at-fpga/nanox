
#include "stubinjector.hpp"
#include "error-injection/errorinjectionplugin.hpp"
#include "system.hpp"

using namespace nanos::error;

struct StubInjectionPlugin : public ErrorInjectionPlugin<StubInjector>
{
		static const char* pluginName() { return "injection-none"; }
};

DECLARE_PLUGIN( StubInjectionPlugin::pluginName(),
                StubInjectionPlugin
              );
