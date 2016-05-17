
#include "error-injection/blockaccessinjector.hpp"
#include "error-injection/errorinjectionplugin.hpp"
#include "error-injection/periodicinjectionpolicy.hpp"
#include "system.hpp"

using namespace nanos::error;

using BlockPageAccessInjectionPlugin = ErrorInjectionPlugin< PeriodicInjectionPolicy<BlockMemoryPageAccessInjector> >;

DECLARE_PLUGIN( "injection-block-access",
                BlockPageAccessInjectionPlugin
              );

