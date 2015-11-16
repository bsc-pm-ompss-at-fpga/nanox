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

#ifndef OPERATION_FAILURE_HPP
#define OPERATION_FAILURE_HPP

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
		using super=SegmentationFaultException;
	public:
		OperationFailure( siginfo_t* signalInfo, ucontext_t* executionContext ) :
				super( signalInfo, executionContext )
		{
		}
};

// Singleton object that installs the signal handler for OperationFailureException
SignalTranslator<OperationFailure> g_objOperationFailureTranslator;

}//namespace error
}//namespace nanos

#endif // OPERATION_FAILURE_HPP

