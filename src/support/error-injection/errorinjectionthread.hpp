
#ifndef ERROR_INJECTION_THREAD_DECL_HPP
#define ERROR_INJECTION_THREAD_DECL_HPP

#include "frequency.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <thread>

namespace nanos {
namespace error {

template <class RandomEngine, template<class> class InjectionPolicy>
class ErrorInjectionThread {
	private:
		using milliseconds_t = std::chrono::duration<double, std::milli>;
		using Injector = InjectionPolicy<RandomEngine>;

		Injector                             &_injectionPolicy;
		std::exponential_distribution<double> _waitTimeDistribution;
		bool                                  _finish;
		bool                                  _wait;
		std::mutex                            _mutex;
		std::condition_variable               _suspendCondition;
		std::thread                           _injectionThread;

	public:
		ErrorInjectionThread( Injector& manager, frequency<double,std::kilo> injectionRate ) noexcept :
			_injectionPolicy( manager ),
			_waitTimeDistribution( injectionRate.count() ),
			_finish(false),
			_wait(true),
			_mutex(),
			_suspendCondition(),
			_injectionThread( &ErrorInjectionThread::injectionLoop, this )
		{
			debug( "Injection thread: Starting thread" );
		}


		virtual ~ErrorInjectionThread() noexcept
		{
			terminate();
			debug( "Injection thread: thread finished" );
		}

		Injector &getInjectionPolicy() { return _injectionPolicy; }

		void terminate() noexcept
		{
			{
				std::unique_lock<std::mutex> lock( _mutex );
				_finish = true;
				_wait = false;
				_suspendCondition.notify_all();
			}
			_injectionThread.join();
		}

		void stop() noexcept
		{
			std::unique_lock<std::mutex> lock( _mutex );
			_wait = true;
		}

		void resume() noexcept
		{
			std::unique_lock<std::mutex> lock( _mutex );
			if( _wait ) {
				_wait = false;
				_suspendCondition.notify_one();
			}
		}

	private:
		void wait()
		{
			std::unique_lock<std::mutex> lock( _mutex );
			if( _wait ) {
				_suspendCondition.wait( lock );
			} else {
				_suspendCondition.wait_for( lock, getWaitTime() );
			}
		}

		milliseconds_t getWaitTime() noexcept
		{
			return milliseconds_t( 
							_waitTimeDistribution( _injectionPolicy.getRandomGenerator() )
						);
		}

		void injectionLoop ()
		{
			while( !_finish ) {
				wait();
				debug("Injection thread: injecting an error");
				getInjectionPolicy().injectError();
			}
		}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_THREAD_DECL_HPP
