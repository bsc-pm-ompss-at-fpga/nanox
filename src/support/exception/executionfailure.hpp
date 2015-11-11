
#include "operationfailure.hpp"

class ExecutionFailure {
	private:
		OperationFailure const& failedOperation;
	public:
		ExecutionFailure( OperationFailure const& operation ) :
				failedOperation( operation )
		{
			bool isRecoverable = task->setInvalid( true );
			if( !isRecoverable ) {
				fatal( "Could not find a recoverable task when recovering from ", failedOperation.what() );
			}
		}
};

