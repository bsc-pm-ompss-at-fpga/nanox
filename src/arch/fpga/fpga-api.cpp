/*************************************************************************************/
/*      Copyright 2010 Barcelona Supercomputing Center                               */
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

#include "nanos-fpga.h"
#include "basethread.hpp"
#include "fpgadd.hpp"
#include "debug.hpp"
#include "fpgaprocessor.hpp"
#include "fpgapinnedallocator.hpp"

#include "libxdma.h"

using namespace nanos;

NANOS_API_DEF( void *, nanos_fpga_factory, ( void *args ) )
{
   nanos_fpga_args_t *fpga = ( nanos_fpga_args_t * ) args;
   // FIXME: acc_num have to be converted into an string
   return ( void * ) NEW ext::FPGADD( fpga->outline, FPGADeviceType( fpga->acc_num < 0 ? 0 : fpga->acc_num ) );
}

NANOS_API_DEF( void *, nanos_fpga_alloc_dma_mem, ( size_t len ) )
{
   warning( "API 'nanos_fpga_alloc_dma_mem( size_t )' is deprecated"
            << " (Implementing using 'malloc')." );
   return malloc( len );
    //return nanos::ext::FPGAProcessor::getPinnedAllocator().allocate( len );
}

NANOS_API_DEF( void, nanos_fpga_free_dma_mem, ( void * buffer ) )
{
   warning( "API 'nanos_fpga_free_dma_mem( size_t )' is deprecated."
            << " (Implementing using 'free')" );
   return free( buffer );
    //nanos::ext::FPGAProcessor::getPinnedAllocator().free( buffer );
}
