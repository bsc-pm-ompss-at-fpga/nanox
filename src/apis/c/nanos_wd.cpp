/*************************************************************************************/
/*      Copyright 2009 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#include "nanos.h"
#include "basethread.hpp"
#include "debug.hpp"
#include "system.hpp"
#include "workdescriptor.hpp"
#include "smpdd.hpp"

using namespace nanos;

// TODO: move to dependent part
void * nanos_smp_factory( void *prealloc, void *args )
{
   nanos_smp_args_t *smp = ( nanos_smp_args_t * ) args;

   if ( prealloc != NULL )
      return ( void * )new (prealloc) ext::SMPDD( smp->outline );
   else 
      return ( void * )new ext::SMPDD( smp->outline );
}

nanos_wd_t nanos_current_wd()
{
   return myThread->getCurrentWD();
}

int nanos_get_wd_id ( nanos_wd_t wd )
{
   WD *lwd = ( WD * )wd;
   return lwd->getId();
}

// FIX-ME: currently, it works only for SMP
nanos_err_t nanos_create_wd (  nanos_wd_t *uwd, size_t num_devices, nanos_device_t *devices, size_t data_size,
                               void ** data, nanos_wg_t uwg, nanos_wd_props_t *props )
{
   try {
      if ( ( props == NULL  ||
            ( props != NULL  && !props->mandatory_creation ) ) && !sys.throttleTask() ) {
         *uwd = 0;
         return NANOS_OK;
      }

      //std::cout << "creatin because?" << std::endl;

      if ( num_devices > 1 ) warning( "Multiple devices not yet supported. Using first one" );

#if 0
      // there is problem at destruction with this right now

      // FIX-ME: support more than one device, other than SMP
      int dd_size = sizeof(ext::SMPDD);

      int size_to_allocate = ( *uwd == NULL ) ? sizeof( WD ) : 0 +
                             ( *data == NULL ) ? data_size : 0 +
                             dd_size
                             ;

      char *chunk=0;

      if ( size_to_allocate )
         chunk = new char[size_to_allocate];

      if ( *uwd == NULL ) {
         *uwd = ( nanos_wd_t ) chunk;
         chunk += sizeof( WD );
      }

      if ( *data == NULL ) {
         *data = chunk;
         chunk += sizeof(data_size);
      }

      DD * dd = ( DD* ) devices[0].factory( chunk , devices[0].arg );
      WD * wd =  new (*uwd) WD( dd, *data );

#else
      WD *wd;

      if ( *data == NULL )
         *data = new char[data_size];

      if ( *uwd ==  NULL )
         *uwd = wd =  new WD( ( DD* ) devices[0].factory( 0, devices[0].arg ), *data );
      else
         wd = ( WD * )*uwd;

#endif

      // add to workgroup
      if ( uwg != NULL ) {
         WG * wg = ( WG * )uwg;
         wg->addWork( *wd );
      }

      // set properties
      if ( props != NULL ) {
         if ( props->tied ) wd->tied();

         if ( props->tie_to ) wd->tieTo( *( BaseThread * )props->tie_to );
      }

   } catch ( ... ) {
      return NANOS_UNKNOWN_ERR;
   }

   return NANOS_OK;
}

nanos_err_t nanos_submit ( nanos_wd_t uwd, unsigned int num_deps, nanos_dependence_t *deps, nanos_team_t team )
{
   try {
      ensure( uwd,"NULL WD received" );

      WD * wd = ( WD * ) uwd;

      if ( team != NULL ) {
         warning( "Submitting to another team not implemented yet" );
      }

      if ( deps != NULL ) {
         Dependency conv_deps[num_deps];
         for ( unsigned int i = 0; i < num_deps; i++ ) {
            conv_deps[i] = Dependency( deps[i].address, deps[i].flags.input, deps[i].flags.output, deps[i].flags.can_rename );
         }
         sys.submitWithDependencies( *wd, num_deps, conv_deps );
      }

      sys.submit( *wd );
   } catch ( ... ) {
      return NANOS_UNKNOWN_ERR;
   }

   return NANOS_OK;
}

// data must be not null
nanos_err_t nanos_create_wd_and_run ( size_t num_devices, nanos_device_t *devices, void * data,
                                      nanos_dependence_t *deps, nanos_wd_props_t *props )
{
   try {
      if ( num_devices > 1 ) warning( "Multiple devices not yet supported. Using first one" );

      if ( deps != NULL ) warning( "Dependence support not implemented yet" );

      // TODO: pre-allocate devices
      WD wd( ( DD* ) devices[0].factory( 0, devices[0].arg ), data );

      sys.inlineWork( wd );

   } catch ( ... ) {
      return NANOS_UNKNOWN_ERR;
   }

   return NANOS_OK;
}

nanos_err_t nanos_set_internal_wd_data ( nanos_wd_t wd, void *data )
{
   try {
      WD *lwd = ( WD * ) wd;

      lwd->setInternalData( data );
   } catch ( ... ) {
      return NANOS_UNKNOWN_ERR;
   }

   return NANOS_OK;
}

nanos_err_t nanos_get_internal_wd_data ( nanos_wd_t wd, void **data )
{
   try {
      WD *lwd = ( WD * ) wd;
      void *ldata;

      ldata = lwd->getInternalData();

      *data = ldata;
   } catch ( ... ) {
      return NANOS_UNKNOWN_ERR;
   }

   return NANOS_OK;
}
