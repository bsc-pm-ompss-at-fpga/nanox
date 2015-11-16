
#include "nanos-error-injection.h"
#include "errorinjectioninterface.hpp"

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
	ErrorInjectionInterface::suspendInjection();
}

void nanos_injection_finalize()
{
	ErrorInjectionInterface::terminateInjection();
}
