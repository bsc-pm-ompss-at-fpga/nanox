// Source code based in the following article:
// http://www.ibm.com/developerworks/library/l-cppexcep/

template < class SignalException >
class SignalTranslator {
	private:
		class SingletonTranslator {
			public:
				SingletonTranslator() {
	   			// Set up the structure to specify task-recovery.
	   			struct sigaction recovery_action;
	   			recovery_action.sa_sigaction = &signalHandler;
	   			sigemptyset(&recovery_action.sa_mask);
					// SA_SIGINFO: Provides context information to the handler.
					// SA_RESTART: Resume system calls interrupted by the signal.
	   			recovery_action.sa_flags = SA_SIGINFO | SA_RESTART;

	   			// Program signal to use the default recovery handler.
	   			int err =
						sigaction(
							SignalException::getSignalNumber(),
							&recovery_action,
							NULL);
	   			fatal_cond( err != 0, "Signal handling setup failed");
				}

				static void signalHandler( int signalNumber, siginfo_t* signalInfo, void* executionContext ) {
					throw SignalException( signalInfo, (ucontext_t*) executionContext );
				}
		};

	public:
		SignalTranslator()
		{
			static SingletonTranslator s_objTranslator;
		}
};

