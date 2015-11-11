
#include "signalexception.hpp"

/* FIXME: Actually uncorrected errors are notified to the application
 * using BusErrorException.
 * Usage of SegmentationFaultException is exclusively for user level
 * fault injection purposes.
 */
class OperationFailure : public SegmentationFaultException {
	private:
		WorkDescriptor &runningTaskWhenHandled;
		//WorkDescriptor &plannedTaskWhenHandled;
		using super=SegmentationFaultException;
	public:
		OperationFailure( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				super( signalInfo, executionContext ),
				runningTaskWhenHandled( *(getMyThreadSafe()->getCurrentWD()) )
				//plannedTaskWhenHandled( *(getMyThreadSafe()->getPlannedWD()) )
		{}
};

// Singleton object that installs the signal handler for OperationFailureException
SignalTranslator<OperationFailure> g_objOperationFailureTranslator;
