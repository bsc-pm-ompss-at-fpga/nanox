
#include "error-injection/stubinjector.hpp"
#include "error-injection/errorinjectionplugin.hpp"
#include "system.hpp"

using namespace nanos::error;

using StubInjectionPlugin = ErrorInjectionPlugin<StubInjector>;

DECLARE_PLUGIN( "injection-none",
                StubInjectionPlugin
              );
