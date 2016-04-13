
#ifndef CHECKPOINTFAILURE_HPP
#define CHECKPOINTFAILURE_HPP

#include "operationfailure.hpp"

namespace nanos {
namespace error {

class CheckpointFailure {
	private:
		OperationFailure& _failedOperation;
	public:
		CheckpointFailure( OperationFailure& operation ) :
				_failedOperation( operation )
		{
			WorkDescriptor* recoverableAncestor = _failedOperation.getTask().propagateInvalidationAndGetRecoverableAncestor();
			if( !recoverableAncestor ) {
				fatal( "Could not find a recoverable task when recovering from ", _failedOperation.what() );
			}
		}
};

} // namespace error
} // namespace nanos

#endif
