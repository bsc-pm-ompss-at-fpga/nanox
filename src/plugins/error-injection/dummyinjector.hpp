
#include "errorinjectionpolicy.hpp"

class DummyInjector : public ErrorInjectionPolicy
{
	public:
		DummyInjector() std::noexcept :
			ErrorInjectionPolicy()
		{
			ErrorInjectionPolicy::terminate();
		}

		void config( ErrorInjectionConfig const& properties )
		{
		}

		std::chrono::seconds<float> getWaitTime() std::noexcept {
			return std::chrono:seconds<float>(0);
		}

		// Tells the injection thread to start injecting errors
		virtual void resume()
		{
		}

		// Tells the injection thread to stop injecting threads
		virtual void suspend()
		{
		}

		// Tells the injection thread to finish its execution
		virtual void terminate()
		{
		}
};

