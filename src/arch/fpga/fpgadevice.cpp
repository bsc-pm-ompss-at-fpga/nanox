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

#include "libxdma.h"

#include "fpgaprocessor.hpp"
#include "fpgaprocessorinfo.hpp"
#include "fpgamemorytransfer.hpp"
#include "fpgadevice.hpp"
#include "fpgaconfig.hpp"
#include "deviceops.hpp"
#include "fpgapinnedallocator.hpp"
#include "instrumentation_decl.hpp"

using namespace nanos;
using namespace nanos::ext;

#define DIRTY_SYNC

FPGADevice::FPGADevice ( const char *n ): Device( n ) {}

void FPGADevice::_copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
   SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject,
   reg_t hostRegionId )
{
   //NOTE: Copies are synchronous so we don't need to register them in the DeviceOps
   copyData( (void *)devAddr, (void *)hostAddr, len );
}

void FPGADevice::_copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
      SeparateMemoryAddressSpace &mem, DeviceOps *ops,
      WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId ) {
   //NOTE: Copies are synchronous so we don't need to register them in the DeviceOps
   copyData( (void *)hostAddr, (void *)devAddr, len );
}

void FPGADevice::copyData( void* dst, void* src, size_t len )
{
   verbose( "FPGADevice copy data (" << len << " bytes) from " << std::hex << src <<
            " to " << std::hex << dst );
   std::memcpy( dst, src, len );
}

void *FPGADevice::memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem,
        WorkDescriptor const *wd, unsigned int copyIdx){
   void * ptr = FPGAProcessor::getPinnedAllocator().allocate( size );
   return ptr;
}

void FPGADevice::memFree( uint64_t addr, SeparateMemoryAddressSpace &mem ){
   void * ptr = ( void * )( addr );
   FPGAProcessor::getPinnedAllocator().free( ptr );
}

//this is used to priorize transfers (because someone needs the data)
//In our case this causes this actually means "finish the transfer"
void FPGADevice::syncTransfer( uint64_t hostAddress, ProcessingElement *pe)
{
    //TODO: At this point we only are going to sync output transfers
    // as input transfers do not need to be synchronized
    ((FPGAProcessor *)pe)->getOutTransferList()->syncTransfer(hostAddress);
    //((FPGAProcessor *)pe)->getInTransferList()->syncTransfer(hostAddress);
}
