
#ifndef OPERATIONFAILURE_HPP
#define OPERATIONFAILURE_HPP

#include "signalexception.hpp"

namespace nanos {
namespace error {

/* FIXME: Actually uncorrected errors are notified to the application
 * using BusErrorException.
 * Usage of SegmentationFaultException is exclusively for user level
 * fault injection purposes.
 */
class OperationFailure : public SegmentationFaultException {
	private:
		WorkDescriptor &_runningTaskWhenHandled;
		//WorkDescriptor &plannedTaskWhenHandled;
		using super=SegmentationFaultException;
	public:
		OperationFailure( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				super( signalInfo, executionContext ),
				_runningTaskWhenHandled( *(getMyThreadSafe()->getCurrentWD()) )
				//plannedTaskWhenHandled( *(getMyThreadSafe()->getPlannedWD()) )
		{}

		WorkDescriptor& getTask() { return _runningTaskWhenHandled; }
};

// Singleton object that installs the signal handler for OperationFailureException
SignalTranslator<OperationFailure> g_objOperationFailureTranslator;

} // namespace error
} // namespace nanos

#endif // OPERATIONFAILURE_HPP

