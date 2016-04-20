#include "instrumentation.hpp"

class FPGAInstrumentation : public DeviceInstrumentation {
    private:
        int deviceid;       //interna instrumentation ID
    public:
        virtual void init() {}
        virtual unsigned long long int getDeviceTime();
        virtual unsigned long long int translateDeviceTime( unsigned long long int );
        virtual void startDeviceTrace() {}
        virtual void pauseDeviceTrace( bool pause ) {}
        virtual void stopDeviceTrace() {}
        virtual const char* getDeviceType() { return "FPGA Accelerator"; }
};
