
#ifndef ERROR_INJECTION_THREAD_DECL_HPP
#define ERROR_INJECTION_THREAD_DECL_HPP

#include "frequency.hpp"
#include "error-injection/periodicinjectionpolicy_fwd.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <thread>

namespace nanos {
namespace error {

template <class RandomEngine>
class ErrorInjectionThread {
	private:
		using InjectionPolicy = PeriodicInjectionPolicy<RandomEngine>;

		InjectionPolicy                      &_injectionPolicy;
		std::exponential_distribution<double> _waitTimeDistribution;
		bool                                  _finish;
		bool                                  _wait;
		std::mutex                            _mutex;
		std::condition_variable               _suspendCondition;
		std::thread                           _injectionThread;

	public:
		ErrorInjectionThread( InjectionPolicy& manager, frequency<double,std::kilo> injectionRate ) noexcept;

		virtual ~ErrorInjectionThread() noexcept;

		InjectionPolicy &getInjectionPolicy();

		void terminate() noexcept;

		void stop() noexcept;

		void resume() noexcept;

	private:
		void wait();

		std::chrono::duration<double,std::milli> getWaitTime() noexcept;

		void injectionLoop ();
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_THREAD_DECL_HPP
