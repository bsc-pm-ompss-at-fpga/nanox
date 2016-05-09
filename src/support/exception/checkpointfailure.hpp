/*************************************************************************************/
/*      Copyright 2009-2015 Barcelona Supercomputing Center                          */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#ifndef CHECKPOINTFAILURE_HPP
#define CHECKPOINTFAILURE_HPP

#include "operationfailure.hpp"
#include "failurestats.hpp"
#include "workdescriptor.hpp"

namespace nanos {
namespace error {

class CheckpointFailure {
	private:
		OperationFailure& _failedOperation;
	public:
		CheckpointFailure( OperationFailure& operation ) :
				_failedOperation( operation )
		{
			debug("Resiliency: checkpoint error detected ", operation.what() );
			FailureStats<CheckpointFailure>::increase();

			WorkDescriptor* recoverableAncestor = operation.getTask().propagateInvalidationAndGetRecoverableAncestor();
			if( !recoverableAncestor ) {
				fatal( "Could not find a recoverable task when recovering from ", operation.what() );
			}
		}
};

} // namespace error
} // namespace nanos

#endif
