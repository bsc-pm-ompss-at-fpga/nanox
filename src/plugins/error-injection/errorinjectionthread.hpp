
#ifndef ERROR_INJECTION_THREAD_HPP
#define ERROR_INJECTION_THREAD_HPP

#include "errorinjectionpolicy.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace nanos {
namespace error {

class ErrorInjectionThread {
	private:
		ErrorInjectionPolicy& injectionPolicy;
		std::atomic<bool> mustFinish;
		std::atomic<bool> allowInjection;
		std::mutex suspendMutex;
		std::condition_variable suspendCondition;
		std::thread injectionThread;

		void wait() {
			std::unique_lock<std::mutex> lockGuard( suspendMutex ); // Lock to safely access conditino variable
			suspendCondition.wait_for( lockGuard, // Mutex used for condition variable
												injectionPolicy.getWaitTime(),// Timeout
												[this]()->bool{ return allowInjection; } // Predicate: should thread be resumed after notify?
											);
		}

		ErrorInjectionPolicy &getInjectionPolicy() { return injectionPolicy; }

	public:
		ErrorInjectionThread( ErrorInjectionPolicy& manager, bool suspend = true ) noexcept :
				injectionPolicy( manager ),
				mustFinish(false),
				allowInjection(!suspend),
				suspendMutex(),
				suspendCondition(),
				injectionThread( injectionLoop, this )
		{
			//std::cout << "Starting injection thread" << std::endl;
		}

		virtual ~ErrorInjectionThread() noexcept
		{
			terminate();
			injectionThread.join();
		}

		bool shouldTerminate() const noexcept {
			return mustFinish;
		}

		void terminate() noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			mustFinish = true;
			allowInjection = true;
			suspendCondition.notify_all();
		}

		void stop() noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			allowInjection = false;
		}

		void resume() noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			if( !allowInjection ) {
				allowInjection = true;
				suspendCondition.notify_one();
			}
		}

	private:
		static void injectionLoop ( ErrorInjectionThread *thisThread ) {
			while( !thisThread->shouldTerminate() ) {
				thisThread->wait();
				thisThread->getInjectionPolicy().injectError();
			}
		}
};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_THREAD
