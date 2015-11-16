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

#ifndef NANOS_CHECKPOINT_FAILURE_HPP
#define NANOS_CHECKPOINT_FAILURE_HPP

#include "operationfailure.hpp"

namespace nanos {
namespace error {

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

}// namespace error
}// namespace nanos
