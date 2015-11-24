
#ifndef PERIODIC_INJECTION_POLICY_DECL_HPP
#define PERIODIC_INJECTION_POLICY_DECL_HPP

#include "error-injection/errorinjectionpolicy.hpp"
#include "error-injection/errorinjectionthread_decl.hpp"
#include "error-injection/periodicinjectionpolicy_fwd.hpp"

#include <random>

namespace nanos {
namespace error {

template < typename RandomEngine = std::minstd_rand >
class PeriodicInjectionPolicy : public ErrorInjectionPolicy
{
	private:
		RandomEngine _generator;
		ErrorInjectionThread<RandomEngine> _thread;

	public:
		PeriodicInjectionPolicy( ErrorInjectionConfig const& properties );

		RandomEngine &getRandomGenerator() { return _generator; }

		virtual void resume();

		virtual void stop();
};

} // namespace error
} // namespace nanos

#endif // PERIODIC_INJECTION_POLICY_DECL_HPP
