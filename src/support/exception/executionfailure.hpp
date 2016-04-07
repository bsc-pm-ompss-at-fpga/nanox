
#include "operationfailure.hpp"

namespace nanos {
namespace error {

class ExecutionFailure {
	private:
		OperationFailure& _failedOperation;
	public:
		ExecutionFailure( OperationFailure& operation ) :
				_failedOperation( operation )
		{
         sys.getExceptionStats().incrExecutionErrors();

			WorkDescriptor* recoverableAncestor = _failedOperation.getTask().propagateInvalidationAndGetRecoverableAncestor();
			if( !recoverableAncestor ) {
				fatal( "Could not find a recoverable task when recovering from ", _failedOperation.what() );
			}
		}
};

} // namespace error
} // namespace nanos

