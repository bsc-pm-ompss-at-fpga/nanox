
#ifndef ERROR_INJECTION_THREAD_HPP
#define ERROR_INJECTION_THREAD_HPP

#include "errorinjectionthread_decl.hpp"
#include "periodicinjectionpolicy_decl.hpp"
#include "frequency.hpp"

#include "debug.hpp"

#include <chrono>
#include <ratio>

namespace nanos {
namespace error {

template < class RandomEngine >
ErrorInjectionThread<RandomEngine>::ErrorInjectionThread( InjectionPolicy& manager, frequency<double, std::kilo> injectionRate ) noexcept :
		_injectionPolicy( manager ),
		_waitTimeDistribution( injectionRate.count() ),
		_finish(false),
		_wait(true),
		_mutex(),
		_suspendCondition(),
		_injectionThread( &ErrorInjectionThread::injectionLoop, this )
{
	debug( "Starting injection thread" );
}

template < class RandomEngine >
ErrorInjectionThread<RandomEngine>::~ErrorInjectionThread() noexcept
{
	terminate();
	debug( "Error injection thread finished." );
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::wait() {
	std::unique_lock<std::mutex> lock( _mutex );
	if( _wait ) {
		_suspendCondition.wait( lock );
	} else {
		_suspendCondition.wait_for( lock, getWaitTime() );
	}
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::terminate() noexcept {
	{
		std::unique_lock<std::mutex> lock( _mutex );
		_finish = true;
		_wait = false;
		_suspendCondition.notify_all();
	}
	_injectionThread.join();
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::stop() noexcept {
	std::unique_lock<std::mutex> lock( _mutex );
	_wait = true;
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::resume() noexcept {
	std::unique_lock<std::mutex> lock( _mutex );
	if( _wait ) {
		_wait = false;
		_suspendCondition.notify_one();
	}
}

template < class RandomEngine >
PeriodicInjectionPolicy<RandomEngine> &ErrorInjectionThread<RandomEngine>::getInjectionPolicy()
{
	return _injectionPolicy;
}


using duration_t = std::chrono::duration<double, std::milli>;

template < class RandomEngine >
duration_t ErrorInjectionThread<RandomEngine>::getWaitTime() noexcept {
	return duration_t( 
					_waitTimeDistribution( _injectionPolicy.getRandomGenerator() )
				);
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::injectionLoop() {
	while( !_finish ) {
		wait();
		getInjectionPolicy().injectError();
	}
}

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_THREAD_HPP

