
#ifndef PERIODIC_INJECTION_POLICY_DECL_HPP
#define PERIODIC_INJECTION_POLICY_DECL_HPP

#include "error-injection/errorinjectionpolicy.hpp"
#include "error-injection/errorinjectionthread.hpp"

#include <random>

namespace nanos {
namespace error {

template < template<class> class InjectionPolicy, typename RandomEngine = std::minstd_rand >
class PeriodicInjectionPolicy : public InjectionPolicy<RandomEngine>
{
	private:
		using Injector = InjectionPolicy<RandomEngine>;

		ErrorInjectionThread<RandomEngine,InjectionPolicy> _thread;

	public:
		PeriodicInjectionPolicy( ErrorInjectionConfig const& properties ) :
			Injector( properties ),
			_thread( *this, properties.getInjectionRate() )
		{
		}

		virtual ~PeriodicInjectionPolicy() {}

		virtual void stop() { _thread.stop(); }

		virtual void resume() { _thread.resume(); }
};

} // namespace error
} // namespace nanos

#endif // PERIODIC_INJECTION_POLICY_DECL_HPP
