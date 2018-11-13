#include "fpgainstrumentation.hpp"
#include "fpgaconfig.hpp"
#include "libxtasks_wrapper.hpp"

using namespace nanos;
using namespace nanos::ext;

nanos_event_time_t FPGAInstrumentation::getDeviceTime() const {
   uint64_t time;
#ifdef NANOS_DEBUG_ENABLED
   ensure0( _deviceInfo != NULL, "Cannot execute FPGAInstrumentation::getDeviceTime when _deviceInfo is NULL" );
   xtasks_stat stat = xtasksGetAccCurrentTime( _deviceInfo->getHandle(), &time );
   ensure0( stat == XTASKS_SUCCESS, "Error executing xtasksGetAccCurrentTime (error code: " << stat << ")" );
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
