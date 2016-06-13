#include "instrumentation.hpp"

namespace nanos {
namespace ext {
   class FPGAInstrumentation : public DeviceInstrumentation {
      private:
         int deviceid;       //interna instrumentation ID
         std::string _deviceType;
      public:
         FPGAInstrumentation() : DeviceInstrumentation() { }
         FPGAInstrumentation( std::string deviceType ) : DeviceInstrumentation(),
         _deviceType( deviceType ) { }

         virtual void init() {}
         virtual unsigned long long int getDeviceTime();
         virtual unsigned long long int translateDeviceTime( unsigned long long int );
         virtual void startDeviceTrace() {}
         virtual void pauseDeviceTrace( bool pause ) {}
         virtual void stopDeviceTrace() {}
         virtual const char* getDeviceType() { return _deviceType.c_str(); }
   };
} //namespace ext
} //namespace nanos
