#include "fpgainstrumentation.hpp"
#include "fpgaconfig.hpp"
#include "libxdma.h"

using namespace nanos;
using namespace nanos::ext;

unsigned long long int FPGAInstrumentation::getDeviceTime() const {
   uint64_t time;
   //TODO error checking
   xdmaGetDeviceTime(&time);
   debug0( "Init device time "  << time );
   return (unsigned long long)time;
}

unsigned long long FPGAInstrumentation::translateDeviceTime( unsigned long long devTime ) const {
   //devTime is raw device time in cycles
   //_deviceInfo->getFreq() returns Mhz (10^6 cycles/sec == 1 cycle/us)
   return devTime * 1000 / _deviceInfo->getFreq();
}
