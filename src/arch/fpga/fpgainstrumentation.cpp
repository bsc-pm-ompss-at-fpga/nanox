#include "fpgainstrumentation.hpp"
#include "libxdma.h"

unsigned long long int FPGAInstrumentation::getDeviceTime() {
   unsigned long long time;
   //TODO error checking
   xdmaGetDeviceTime(&time);
   return time;
}

unsigned long long FPGAInstrumentation::translateDeviceTime( unsigned long long devTime ) {
   //TODO get this value from clock status registers
   static const unsigned long long cycleTime = 10; //ns
   //devTime is raw device time in cycles
   return devTime * cycleTime;
}
