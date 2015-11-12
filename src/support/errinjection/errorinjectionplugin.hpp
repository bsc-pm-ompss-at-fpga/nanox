
template < typename ErrorInjector >
class ErrorInjectionPlugin : public Plugin
{
   public:
      ErrorInjectionPlugin() : Plugin( "Error injection plugin", 1 ) {}

      virtual void config ( Config &cfg )
      {
			// TODO: put dependent configurations in each particular injector implementation plugin
			cfg.registerConfigOption("memory_poisoning",
				NEW Config::FlagOption(_memory_poison_enabled, true),
				"Enables random memory page poisoning (resiliency testing)");
			cfg.registerArgOption("memory_poisoning", "memory-poisoning");
			cfg.registerEnvOption("memory_poisoning", "NX_ENABLE_POISONING");

			cfg.registerConfigOption("mp_seed",
				NEW Config::IntegerVar(_memory_poison_seed),
				"Seed used by memory page poisoning RNG (default: 'time(0)')");
			cfg.registerArgOption("mp_seed", "mpoison-seed");
			cfg.registerEnvOption("mp_seed", "NX_MPOISON_SEED");

			cfg.registerConfigOption("mp_rate",
				NEW Config::FloatVar(_memory_poison_rate),
				"Memory poisoning rate (error/s). Default: '0')");
			cfg.registerArgOption("mp_rate", "mpoison-rate");
			cfg.registerEnvOption("mp_rate", "NX_MPOISON_RATE");

			cfg.registerConfigOption("mp_amount",
				NEW Config::IntegerVar(_memory_poison_amount),
				"Maximum number of injected errors (default: infinite )");
			cfg.registerArgOption("mp_amount", "mpoison-amount");
			cfg.registerEnvOption("mp_amount", "NX_MPOISON_AMOUNT");
      }

      virtual void init() {
         static ErrorInjectionPolicy injectionManager;
      }
};
