
#include "operationfailure.hpp"

class CheckpointFailure {
	private:
		OperationFailure const& failedOperation;
	public:
		CheckpointFailure( OperationFailure const& operation ) :
				failedOperation( operation )
		{
			WorkDescriptor* recoverableAncestor = _failedOperation.getTask().propagateInvalidationAndGetRecoverableAncestor();
			if( !recoverableAncestor ) {
				fatal( "Could not find a recoverable task when recovering from ", failedOperation.what() );
			}
		}
};
