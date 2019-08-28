/*************************************************************************************/
/*      Copyright 2009-2018 Barcelona Supercomputing Center                          */
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
/*      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            */
/*************************************************************************************/
#include "fpgaprocessor.hpp"
#include "fpgaprocessorinfo.hpp"
#include "fpgadevice.hpp"
#include "deviceops.hpp"
#include "simpleallocator.hpp"
#include "fpgapinnedallocator.hpp"
#include "instrumentation_decl.hpp"

using namespace nanos;
using namespace nanos::ext;

FPGADevice::FPGADevice ( FPGADeviceType const t ) : FPGADeviceName( t ),
   Device( _fpgaArchName.c_str() ), _fpgaType( t )
{}

void FPGADevice::_copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
   SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject,
   reg_t hostRegionId )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "cache-copy-data-in", wd->getId() ) );
   FPGAPinnedAllocator *allocator = ( FPGAPinnedAllocator * )( mem.getSpecificData() );
   //NOTE: Copies are synchronous so we don't need to register them in the DeviceOps
   size_t offset = devAddr - allocator->getBaseAddress();
   fpgaCopyDataToFPGA( allocator->getBufferHandle(), offset, len, (void *)hostAddr );
}

void FPGADevice::_copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
   SeparateMemoryAddressSpace &mem, DeviceOps *ops,
   WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "cache-copy-data-out", wd->getId() ) );
   FPGAPinnedAllocator *allocator = ( FPGAPinnedAllocator * )( mem.getSpecificData() );
   //NOTE: Copies are synchronous so we don't need to register them in the DeviceOps
   size_t offset = devAddr - allocator->getBaseAddress();
   fpgaCopyDataFromFPGA( allocator->getBufferHandle(), offset, len, (void *)hostAddr );
}

bool FPGADevice::_copyDevToDev( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len,
   SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memorig, DeviceOps *ops,
   WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId )
{
   fatal( "FPGADevice::_copyDevToDev not implemented yet" );
   return true;
}

void FPGADevice::_copyInStrided1D( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
   std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem,
   DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "cache-copy-data-in", wd->getId() ) );
   fatal( "FPGADevice::_copyInStrided1D not implemented yet" );
   for ( std::size_t count = 0; count < numChunks; count += 1) {
      //copyData( ((char *) devAddr) + count * ld, ((char *) hostAddr) + count * ld, len );
   }
}

void FPGADevice::_copyOutStrided1D( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
   std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem,
   DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId )
{
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "cache-copy-data-out", wd->getId() ) );
   fatal( "FPGADevice::_copyOutStrided1D not implemented yet" );
   for ( std::size_t count = 0; count < numChunks; count += 1) {
      //copyData( ((char *) hostAddr) + count * ld, ((char *) devAddr) + count * ld, len );
   }
}

bool FPGADevice::_copyDevToDevStrided1D( uint64_t devDestAddr, uint64_t devOrigAddr,
   std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &memDest,
   SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject,
   reg_t hostRegionId )
{
   fatal( "FPGADevice::_copyDevToDevStrided1D not implemented yet" );
   for ( std::size_t count = 0; count < numChunks; count += 1) {
      //copyData( ((char *) devDestAddr) + count * ld, ((char *) devOrigAddr) + count * ld, len );
   }
   return true;
}

void *FPGADevice::memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem,
   WorkDescriptor const *wd, unsigned int copyIdx )
{
   FPGAPinnedAllocator *allocator = ( FPGAPinnedAllocator * )( mem.getSpecificData() );
   //verbose( "FPGADevice allocate memory:\t " << size << " bytes in allocator " << allocator );
   return allocator->allocate( size );
}

void FPGADevice::memFree( uint64_t addr, SeparateMemoryAddressSpace &mem )
{
   void * ptr = ( void * )( addr );
   FPGAPinnedAllocator *allocator = ( FPGAPinnedAllocator * )( mem.getSpecificData() );
   //verbose( "FPGADevice free memory:\t " << ptr << " in allocator " << allocator );
   allocator->free( ptr );
}

void FPGADevice::_canAllocate( SeparateMemoryAddressSpace &mem, std::size_t *sizes,
   unsigned int numChunks, std::size_t *remainingSizes )
{
   SimpleAllocator *allocator = ( SimpleAllocator * )( mem.getSpecificData() );
   allocator->canAllocate( sizes, numChunks, remainingSizes );
}

void FPGADevice::_getFreeMemoryChunksList( SeparateMemoryAddressSpace &mem,
   SimpleAllocator::ChunkList &list )
{
   SimpleAllocator *allocator = ( SimpleAllocator * )( mem.getSpecificData() );
   allocator->getFreeChunksList( list );
}

std::size_t FPGADevice::getMemCapacity( SeparateMemoryAddressSpace &mem )
{
   SimpleAllocator *allocator = ( SimpleAllocator * )( mem.getSpecificData() );
   return allocator->getCapacity();
}
