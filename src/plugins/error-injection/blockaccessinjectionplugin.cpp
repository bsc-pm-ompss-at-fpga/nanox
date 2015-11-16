
#include "blockaccessinjector.hpp"
#include "errorinjectionplugin.hpp"


namespace nanos {
namespace resiliency {

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
};

}// namespace resiliency
}// namespace nanos

