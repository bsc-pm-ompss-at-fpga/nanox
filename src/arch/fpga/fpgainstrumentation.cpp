#include "fpgainstrumentation.hpp"
#include "fpgaconfig.hpp"
#include "libxdma.h"

using namespace nanos;
using namespace nanos::ext;

unsigned long long int FPGAInstrumentation::getDeviceTime() {
   uint64_t time;
   //TODO error checking
   xdmaGetDeviceTime(&time);
   debug( "Init device time "  << time );
   return (unsigned long long)time;
}

unsigned long long FPGAInstrumentation::translateDeviceTime( unsigned long long devTime ) {
   //TODO get this value from clock status registers
   unsigned int cycleTime = FPGAConfig::getCycleTime();
   //devTime is raw device time in cycles
   return devTime * cycleTime;
}
