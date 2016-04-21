
#ifndef PERIODIC_INJECTION_POLICY_HPP
#define PERIODIC_INJECTION_POLICY_HPP

#include "periodicinjectionpolicy_decl.hpp"
#include "error-injection/errorinjectionthread.hpp"

namespace nanos {
namespace error {

template < typename RandomEngine >
PeriodicInjectionPolicy<RandomEngine>::PeriodicInjectionPolicy( ErrorInjectionConfig const& properties ) :
	ErrorInjectionPolicy( properties ),
	_generator( properties.getInjectionSeed() ),
	_thread( *this, properties.getInjectionRate() )
{
}

template < typename RandomEngine >
void PeriodicInjectionPolicy<RandomEngine>::resume()
{
	_thread.resume();
}

template < typename RandomEngine >
void PeriodicInjectionPolicy<RandomEngine>::stop()
{
	_thread.stop();
}

} // namespace error
} // namespace nanos

#endif // PERIODIC_INJECTION_POLICY_HPP

