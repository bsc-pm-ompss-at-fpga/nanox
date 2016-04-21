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

#include "signalexception.hpp"

#include <signal.h>
#include <sstream>

using namespace nanos::error;

// Probably we could add a class specialization
// for each SIGBUS/SIGSEGV signal codes
std::string BusErrorException::getErrorMessage() const
{
	std::stringstream ss;
	ss << "BusErrorException:";
	switch ( getSignalInfo().getSignalCode() ) {
		case BUS_ADRALN:
			ss << " Invalid address alignment.";
			break;
		case BUS_ADRERR:
			ss << " Nonexisting physical address.";
			break;
		case BUS_OBJERR:
			ss << " Object-specific hardware error.";
			break;
#ifdef BUS_MCEERR_AR
		case BUS_MCEERR_AR: //(since Linux 2.6.32)
			ss << " Hardware memory error consumed on a machine check; action required.";
			break;
#endif
#ifdef BUS_MCEERR_AO
		case BUS_MCEERR_AO: //(since Linux 2.6.32)
			ss << " Hardware memory error detected in process but not consumed; action optional.";
			break;
#endif
	}
	return ss.str();
}

std::string SegmentationFaultException::getErrorMessage() const
{
	std::stringstream ss;
	ss << "SegmentationFaultException:";
	switch ( getSignalInfo().getSignalCode() ) {
		case SEGV_MAPERR:
			ss << " Address not mapped to object.";
			break;
		case SEGV_ACCERR:
			ss << " Invalid permissions for mapped object.";
			break;
	}
	return ss.str();
}

