
#include "dummyinjector.hpp"
#include "errorinjectionplugin.hpp"


namespace nanos {
namespace resiliency {

class NoInjectionPlugin : public ErrorInjectionPlugin
{
	private:
		DummyInjector policy;

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

