
#include "operationfailure.hpp"
#include "signaltranslator.hpp"

namespace nanos {
namespace error {

// Singleton object that installs the signal handler for OperationFailureException
SignalTranslator<OperationFailure> g_objOperationFailureTranslator;

}
}

