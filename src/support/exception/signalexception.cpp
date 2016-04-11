
#include "signalexception.hpp"

#include <signal.h>
#include <sstream>

using namespace nanos::error;

std::string BusErrorException::getErrorMessage() const
{
	std::stringstream ss;
	ss << "BusErrorException:";
	switch ( getHandledSignalInfo().getSignalCode() ) {
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
	switch ( getHandledSignalInfo().getSignalCode() ) {
		case SEGV_MAPERR:
			ss << " Address not mapped to object.";
			break;
		case SEGV_ACCERR:
			ss << " Invalid permissions for mapped object.";
			break;
	}
	return ss.str();
}
