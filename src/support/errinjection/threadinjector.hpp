
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

class ErrorInjectionThread {
	private:
		ErrorInjectionPolicy const& injectionPolicy;
		std::thread injectionThread;
		std::atomic<bool> terminate;
		std::atomic<bool> dont_suspend;
		std::mutex suspendMutex;
		std::condition_variable suspendCondition;

		static void injectionLoop ( InjectionThread &thisThread ) {
			while( !thisThread.shouldTermninate() ) {
				this_thread.wait();
				this_thread.getInjectionPolicy().injectError();
			}
		}

		void wait() {
			std::unique_lock<std::mutex> lockGuard( suspendMutex ); // Lock to safely access conditino variable
			suspendCondition.wait_for( suspendMutex, // Mutex used for condition variable
												injectionPolicy.getWaitTime(),// Timeout
												[](){return dont_suspend;} // Predicate: should thread be resumed after notify?
											);
		}

		ErrorInjectionPolicy &getInjectionPolicy() { return injectionPolicy; }

	public:
		InjectionThread( ErrorInjectionPolicy const& manager ) std::noexcept :
				injectionPolicy( manager ),
				injectionThread( injectionFunction, *this ),
				terminate(false),
				dont_suspend(true)
		{
			std::cout << "Starting injection thread" << std::endl;
		}

		virtual ~InjectionThread() std::noexcept
		{
			terminate();

			injectionThread.join();
		}

		bool shouldTerminate() const std::noexcept {
			return terminate;
		}

		void terminate() std::noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			terminate = true;
			dont_suspend = true;
			suspendCondition.notify();
		}

		void stop() std::noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			dont_suspend = false;
		}

		void resume() std::noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			if( !dont_suspend ) {
				dont_suspend = true;
				suspendCondition.nofity();
			}
		}
};
