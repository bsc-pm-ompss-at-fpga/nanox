
#include "nanos-int.h"

//! \defgroup errinjectionapi
//! \ingroup errinjectionapi
//! \{

#ifdef __cplusplus
extern "C" {
#endif

void nanos_inject_error( void *handle );

void nanos_inject_error_( void **handle );

void nanos_declare_resource( void *handle, size_t size );

void nanos_declare_resource_( void **handle, size_t *size );

NANOS_API_DECL( void, nanos_injection_start, (void));

NANOS_API_DECL( void, nanos_injection_stop, (void));

NANOS_API_DECL( void, nanos_injection_finalize, (void));

#ifdef __cplusplus
} //extern C
#endif

//! \}
