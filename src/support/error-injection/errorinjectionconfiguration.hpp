
#ifndef ERROR_INJECTION_CONFIG_HPP
#define ERROR_INJECTION_CONFIG_HPP

#include "config.hpp"
#include "frequency.hpp"
#include "system.hpp"

#include <chrono>
#include <string>

namespace nanos {
namespace error {

// TODO: review how Config is used with plugins (src/support/plugin.cpp:98)
class ErrorInjectionConfig /*: public Config*/ {
	private:
		std::string selected_injector;    //!< Name of the error injector selected by the user
		frequency<float> injection_rate;  //!< Injection rate in Hz
		unsigned injection_limit;         //!< Maximum number of errors injected (0: unlimited)
		unsigned injection_seed;          //!< Error injection random number generator seed.
		
	public:
		ErrorInjectionConfig () : 
				//Config(),
				selected_injector("none"),
				injection_rate(0),
				injection_limit(0),
				injection_seed(0)
		{
		}

		void config( Config &properties )
		{
			properties.setOptionsSection("Error injection plugin for resiliency evaluation", "Injection plugin specific options" );

			properties.registerConfigOption("error_injection_seed",
			            NEW Config::UintVar(injection_seed),
			            "Error injector randon number generator seed.");
			properties.registerArgOption("error_injection_seed", "error-injection-seed");
			properties.registerEnvOption("error_injection_seed", "NX_ERROR_INJECTION_SEED");

			properties.registerConfigOption("error_injection_rate",
			            NEW Config::FloatVar(static_cast<float&>(injection_rate)),
			            "Error injection rate (Hz).");
			properties.registerArgOption("error_injection_rate", "error-injection-rate");
			properties.registerEnvOption("error_injection_rate", "NX_ERROR_INJECTION_RATE");

			properties.registerConfigOption("error_injection_limit",
			            NEW Config::UintVar(injection_limit),
			            "Maximum number of injected errors (0: unlimited)");
			properties.registerArgOption("error_injection_limit", "error-injection-limit");
			properties.registerEnvOption("error_injection_limit", "NX_ERROR_INJECTION_LIMIT");
		}

		std::string const& getSelectedInjectorName() const { return selected_injector; }

		frequency<float> getInjectionRate() const { return injection_rate; }

		unsigned getInjectionLimit() const { return injection_limit; }

		unsigned getInjectionSeed() const { return injection_seed; }

};

} // namespace error
} // namespace nanos

#endif // ERROR_INJECTION_CONFIG
