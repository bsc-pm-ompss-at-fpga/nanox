/*************************************************************************************/
/*      Copyright 2009-2018 Barcelona Supercomputing Center                          */
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
/*      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            */
/*************************************************************************************/

#include "fpgainstrumentation.hpp"
#include "fpgaconfig.hpp"
#include "libxtasks_wrapper.hpp"

using namespace nanos;
using namespace nanos::ext;

nanos_event_time_t FPGAInstrumentation::getDeviceTime() const {
   uint64_t time;
#ifdef NANOS_DEBUG_ENABLED
   ensure0( _deviceInfo != NULL, " Cannot execute FPGAInstrumentation::getDeviceTime when _deviceInfo is NULL" );
   xtasks_stat stat = xtasksGetAccCurrentTime( _deviceInfo->getHandle(), &time );
   ensure0( stat == XTASKS_SUCCESS, " Error executing xtasksGetAccCurrentTime (error code: " << stat << ")" );
   debug0( "Initial FPGA device time: "  << time );
#else
   xtasksGetAccCurrentTime( _deviceInfo->getHandle(), &time );
#endif //NANOS_DEBUG_ENABLED
   return ( nanos_event_time_t )( time );
}

nanos_event_time_t FPGAInstrumentation::translateDeviceTime( nanos_event_time_t devTime ) const {
   //devTime is raw device time in cycles
   //_deviceInfo->getFreq() returns Mhz (10^6 cycles/sec == 1 cycle/us)
   return devTime * 1000 / _deviceInfo->getFreq();
}
