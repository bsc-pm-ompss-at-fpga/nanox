
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

class ErrorInjectionThread {
	private:
		ErrorInjectionPolicy const& injectionPolicy;
		std::atomic<bool> terminate;
		std::atomic<bool> allowInjection;
		std::mutex suspendMutex;
		std::condition_variable suspendCondition;
		std::thread injectionThread;

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
												[](){return allowInjection;} // Predicate: should thread be resumed after notify?
											);
		}

		ErrorInjectionPolicy &getInjectionPolicy() { return injectionPolicy; }

	public:
		InjectionThread( ErrorInjectionPolicy const& manager, bool suspend = true ) std::noexcept :
				injectionPolicy( manager ),
				terminate(false),
				allowInjection(!suspend),
				suspendMutex(),
				suspendCondition(),
				injectionThread( injectionFunction, *this )				
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
			allowInjection = true;
			suspendCondition.notify();
		}

		void stop() std::noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			allowInjection = false;
		}

		void resume() std::noexcept {
			std::unique_lock<std::mutex> lockGuard( suspendMutex );
			if( !allowInjection ) {
				allowInjection = true;
				suspendCondition.nofity();
			}
		}
};
