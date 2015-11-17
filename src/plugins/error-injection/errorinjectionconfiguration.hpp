
#ifndef ERROR_INJECTION_CONFIG_HPP
#define ERROR_INJECTION_CONFIG_HPP

#include "config.hpp"
#include "frequency.hpp"
#include <chrono>
#include <string>

namespace nanos {
namespace error {

class ErrorInjectionConfig : public Config {
	private:
		std::string selected_injector;    //!< Name of the error injector selected by the user
		frequency<float> injection_rate;  //!< Injection rate in Hz
		unsigned injection_limit;         //!< Maximum number of errors injected (0: unlimited)
		unsigned injection_seed;          //!< Error injection random number generator seed.
		
	public:
		ErrorInjectionConfig () : 
				Config(),
				selected_injector("none"),
				injection_rate(0),
				injection_limit(0),
				injection_seed(0)
		{
			//registerConfigOption("error_injection",
			//	NEW Config::StringVar(selected_injector, "none"),
			//	"Selects error injection policy. Used for resiliency evaluation.");
			//registerArgOption("error_injection", "error-injection");
			//registerEnvOption("error_injection", "NX_ERROR_INJECTION");

			//registerConfigOption("error_injection_seed",
			//	NEW Config::IntegerVar(injection_seed,0),
			//	"Error injector randon number generator seed.");
			//registerArgOption("error_injection_seed", "error-injection-seed");
			//registerEnvOption("error_injection_seed", "NX_ERROR_INJECTION_SEED");

			//registerConfigOption("error_injection_rate",
			//	NEW Config::FloatVar(static_cast<float&>(injection_rate),0.0f),
			//	"Error injection rate (Hz).");
			//registerArgOption("error_injection_rate", "error-injection-rate");
			//registerEnvOption("error_injection_rate", "NX_ERROR_INJECTION_RATE");

			//registerConfigOption("error_injection_limit",
			//	NEW Config::IntegerVar(injection_limit,0),
			//	"Maximum number of injected errors (0: unlimited)");
			//registerArgOption("error_injection_limit", "error-injection-limit");
			//registerEnvOption("error_injection_limit", "NX_ERROR_INJECTION_LIMIT");

			init();
		}

		std::string const& getSelectedInjectorName() const { return selected_injector; }

		frequency<float> getInjectionRate() const { return injection_rate; }

		unsigned getInjectionLimit() const { return injection_limit; }

		unsigned getInjectionSeed() const { return injection_seed; }

};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_CONFIG
