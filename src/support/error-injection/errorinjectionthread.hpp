
#ifndef ERROR_INJECTION_THREAD_HPP
#define ERROR_INJECTION_THREAD_HPP

#include "errorinjectionthread_decl.hpp"
#include "periodicinjectionpolicy_decl.hpp"
#include "frequency.hpp"

#include <chrono>
#include <ratio>

namespace nanos {
namespace error {

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::injectionLoop ( ErrorInjectionThread<RandomEngine> *thisThread ) {
	while( !thisThread->shouldTerminate() ) {
		thisThread->wait();
		thisThread->getInjectionPolicy().injectError();
	}
}

template < class RandomEngine >
ErrorInjectionThread<RandomEngine>::ErrorInjectionThread( InjectionPolicy& manager, frequency<float, std::ratio<1> > injectionRate ) noexcept :
		injectionPolicy( manager ),
		waitTimeDistribution( injectionRate.count() ),
		mustFinish(false),
		allowInjection(false),
		suspendMutex(),
		suspendCondition(),
		injectionThread( injectionLoop, this )
{
	//std::cout << "Starting injection thread" << std::endl;
}

template < class RandomEngine >
ErrorInjectionThread<RandomEngine>::~ErrorInjectionThread() noexcept
{
	terminate();
	injectionThread.join();
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::wait() {
	std::unique_lock<std::mutex> lockGuard( suspendMutex );             // Lock to safely access conditino variable
	suspendCondition.wait_for( lockGuard,                               // Mutex used for condition variable
										getWaitTime(),                           // Timeout
										[this]()->bool{ return allowInjection; } // Predicate: should thread be resumed after timeout?
									);
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::terminate() noexcept {
	std::unique_lock<std::mutex> lockGuard( suspendMutex );
	mustFinish = true;
	allowInjection = true;
	suspendCondition.notify_all();
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::stop() noexcept {
	std::unique_lock<std::mutex> lockGuard( suspendMutex );
	allowInjection = false;
}

template < class RandomEngine >
void ErrorInjectionThread<RandomEngine>::resume() noexcept {
	std::unique_lock<std::mutex> lockGuard( suspendMutex );
	if( !allowInjection ) {
		allowInjection = true;
		suspendCondition.notify_one();
	}
}

template < class RandomEngine >
std::chrono::duration<float, std::ratio<1> > ErrorInjectionThread<RandomEngine>::getWaitTime() noexcept {
	using duration_type = std::chrono::duration<float, std::ratio<1> >;
	duration_type value = duration_type(waitTimeDistribution( injectionPolicy.getRandomGenerator() ));
	return value;
}

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_THREAD_HPP

