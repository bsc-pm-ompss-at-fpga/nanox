
#include "operationfailure.hpp"

class CheckpointFailure {
	private:
		OperationFailure const& failedOperation;
	public:
		CheckpointFailure( OperationFailure const& operation ) :
				failedOperation( operation )
		{
			bool isRecoverable = task->setInvalid( true );
			if( !isRecoverable ) {
				fatal( "Could not find a recoverable task when recovering from ", failedOperation.what() );
			}
		}
};
