
#include "nanos-int.h"
#include "nanos-error-injection.h"
#include "error-injection/errorinjectioninterface.hpp"

using namespace nanos::error;

void nanos_inject_error( void *handle )
{
	ErrorInjectionInterface::injectError( handle );
}

void nanos_inject_error_( void **handle )
{
	ErrorInjectionInterface::injectError( handle );
}

void nanos_declare_resource( void *handle, size_t size )
{
	ErrorInjectionInterface::declareResource( handle, size );
}

void nanos_declare_resource_( void **handle, size_t *size )
{
	ErrorInjectionInterface::declareResource( *handle, *size );
}

NANOS_API_DEF( void, nanos_injection_start, (void) )
{
	ErrorInjectionInterface::resumeInjection();
}

NANOS_API_DEF( void, nanos_injection_stop, (void) )
{
	ErrorInjectionInterface::stopInjection();
}

NANOS_API_DEF( void, nanos_injection_finalize, (void) )
{
	ErrorInjectionInterface::terminateInjection();
}

