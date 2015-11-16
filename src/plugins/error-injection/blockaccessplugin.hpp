
#include "errorinjectionplugin.hpp"

class BlockPageAccessInjectionPlugin : public ErrorInjectionPlugin
{
	using super = ErrorInjectionPlugin;

   public:
      BlockPageAccessInjectionPlugin() : super()
		{
		}
};
