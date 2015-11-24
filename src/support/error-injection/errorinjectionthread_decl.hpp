
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

		InjectionPolicy& injectionPolicy;

		std::exponential_distribution<float> waitTimeDistribution;

		std::atomic<bool> mustFinish;
		std::atomic<bool> allowInjection;
		std::mutex suspendMutex;
		std::condition_variable suspendCondition;
		std::thread injectionThread;

		void wait();

		std::chrono::duration<float, std::ratio<1> > getWaitTime() noexcept;

		bool shouldTerminate() const noexcept { return mustFinish; }

	public:
		ErrorInjectionThread( InjectionPolicy& manager, frequency<float,std::ratio<1> > injectionRate ) noexcept;

		virtual ~ErrorInjectionThread() noexcept;

		InjectionPolicy &getInjectionPolicy() { return injectionPolicy; }

		void terminate() noexcept;

		void stop() noexcept;

		void resume() noexcept;

	private:
		static void injectionLoop ( ErrorInjectionThread *thisThread );
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_THREAD_DECL_HPP
