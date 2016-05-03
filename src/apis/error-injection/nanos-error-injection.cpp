
#include "nanos-error-injection.h"
#include "error-injection/errorinjectioninterface.hpp"

using namespace nanos::error;

void nanos_inject_error( void *handle )
{
	ErrorInjectionInterface::injectError( handle );
}

void nanos_declare_resource( void *handle, size_t size )
{
	ErrorInjectionInterface::declareResource( handle, size );
}

void nanos_injection_start()
{
	ErrorInjectionInterface::resumeInjection();
}

void nanos_injection_stop()
{
	ErrorInjectionInterface::stopInjection();
}

void nanos_injection_finalize()
{
	ErrorInjectionInterface::terminateInjection();
}
