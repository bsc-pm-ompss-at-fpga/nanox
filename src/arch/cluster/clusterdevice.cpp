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


#include "clusterdevice_decl.hpp"
#include "basethread.hpp"
#include "debug.hpp"
#include "system.hpp"
#include "network_decl.hpp"
#include "clusternode_decl.hpp"
#include "deviceops.hpp"
#include <iostream>

using namespace nanos;
using namespace nanos::ext;

ClusterDevice nanos::ext::Cluster( "SMP" );


ClusterDevice::ClusterDevice ( const char *n ) : Device ( n ) {
}

ClusterDevice::ClusterDevice ( const ClusterDevice &arch ) : Device ( arch ) {
}

ClusterDevice::~ClusterDevice() {
}

void * ClusterDevice::memAllocate( size_t size, SeparateMemoryAddressSpace &mem, WorkDescriptor const &wd, unsigned int copyIdx) const {
   void *retAddr = NULL;

   SimpleAllocator *allocator = (SimpleAllocator *) mem.getSpecificData();
   allocator->lock();
   retAddr = allocator->allocate( size );
   allocator->unlock();
   return retAddr;
}

void ClusterDevice::memFree( uint64_t addr, SeparateMemoryAddressSpace &mem ) const {
   SimpleAllocator *allocator = (SimpleAllocator *) mem.getSpecificData();
   allocator->lock();
   allocator->free( (void *) addr );
   allocator->unlock();
}

void ClusterDevice::_copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const {
   ops->addOp();
   sys.getNetwork()->put( mem.getNodeNumber(),  devAddr, ( void * ) hostAddr, len, wd.getId(), wd, hostObject, hostRegionId );
   ops->completeOp();
}

void ClusterDevice::_copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const {

   char *recvAddr = NULL;
   do { 
      recvAddr = (char *) sys.getNetwork()->allocateReceiveMemory( len );
      if ( !recvAddr ) {
         myThread->idle( true );
      }
   } while ( recvAddr == NULL );

   GetRequest *newreq = NEW GetRequest( (char *) hostAddr, len, recvAddr, ops, f );
   myThread->_pendingRequests.insert( newreq );

   ops->addOp();
   sys.getNetwork()->get( ( void * ) recvAddr, mem.getNodeNumber(), devAddr, len, (volatile int *) newreq, hostObject, hostRegionId );
}

bool ClusterDevice::_copyDevToDev( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const {
   ops->addOp();
   sys.getNetwork()->sendRequestPut( memOrig.getNodeNumber(), devOrigAddr, memDest.getNodeNumber(), devDestAddr, len, wd.getId(), wd, f, hostObject, hostRegionId );
   ops->completeOp();
   return true;
}

void ClusterDevice::_copyInStrided1D( uint64_t devAddr, uint64_t hostAddr, std::size_t len, std::size_t count, std::size_t ld, SeparateMemoryAddressSpace const &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
   char * hostAddrPtr = (char *) hostAddr;
   ops->addOp();
   //NANOS_INSTRUMENT( InstrumentState inst2(NANOS_STRIDED_COPY_PACK); );
   char * packedAddr = (char *) _packer.give_pack( hostAddr, len, count );
   if ( packedAddr != NULL) { 
      for ( unsigned int i = 0; i < count; i += 1 ) {
         ::memcpy( &packedAddr[ i * len ], &hostAddrPtr[ i * ld ], len );
      }
   } else { std::cerr << "copyInStrided ERROR!!! could not get a packet to gather data." << std::endl; }
   //NANOS_INSTRUMENT( inst2.close(); );
   sys.getNetwork()->putStrided1D( mem.getNodeNumber(),  devAddr, ( void * ) hostAddr, packedAddr, len, count, ld, wd.getId(), wd, hostObject, hostRegionId );
   _packer.free_pack( hostAddr, len, count, packedAddr );
   ops->completeOp();
}

void ClusterDevice::_copyOutStrided1D( uint64_t hostAddr, uint64_t devAddr, std::size_t len, std::size_t count, std::size_t ld, SeparateMemoryAddressSpace const &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
   char * hostAddrPtr = (char *) hostAddr;

   std::size_t maxCount = ( ( len * count ) <= sys.getNetwork()->getMaxGetStridedLen() ) ?
      count : ( sys.getNetwork()->getMaxGetStridedLen() / len );

   if ( maxCount != count ) std::cerr <<"WARNING: maxCount("<< maxCount << ") != count(" << count <<") MaxGetStridedLen="<< sys.getNetwork()->getMaxGetStridedLen()<<std::endl;
   for ( unsigned int i = 0; i < count; i += maxCount ) {
      unsigned int thisCount = ( i + maxCount > count ) ? count - i : maxCount; 
      char * packedAddr = NULL;
      do {
         packedAddr = (char *) _packer.give_pack( hostAddr, len, thisCount );
         if (!packedAddr ) {
            myThread->idle( true );
         }
      } while ( packedAddr == NULL );

      if ( packedAddr != NULL) { 
         GetRequestStrided *newreq = NEW GetRequestStrided( &hostAddrPtr[ i * ld ] , len, thisCount, ld, packedAddr, ops, f, &_packer );
         myThread->_pendingRequests.insert( newreq );
         ops->addOp();
         sys.getNetwork()->getStrided1D( packedAddr, mem.getNodeNumber(), devAddr, devAddr + ( i * ld ), len, thisCount, ld, (volatile int *) newreq, hostObject, hostRegionId );
      } else {
         std::cerr << "copyOutStrdided ERROR!!! could not get a packet to gather data." << std::endl;
      }
   }
}

bool ClusterDevice::_copyDevToDevStrided1D( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, std::size_t count, std::size_t ld, SeparateMemoryAddressSpace const &memDest, SeparateMemoryAddressSpace const &memOrig, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const {
   ops->addOp();
   sys.getNetwork()->sendRequestPutStrided1D( memOrig.getNodeNumber(), devOrigAddr, memDest.getNodeNumber(), devDestAddr, len, count, ld, wd.getId(), wd, f, hostObject, hostRegionId );
   ops->completeOp();
   return true;
}

void ClusterDevice::_canAllocate( SeparateMemoryAddressSpace const &mem, std::size_t *sizes, unsigned int numChunks, std::size_t *remainingSizes ) const {
   SimpleAllocator *allocator = (SimpleAllocator *) mem.getSpecificData();
   allocator->canAllocate( sizes, numChunks, remainingSizes );
}
void ClusterDevice::_getFreeMemoryChunksList( SeparateMemoryAddressSpace const &mem, SimpleAllocator::ChunkList &list ) const {
   SimpleAllocator *allocator = (SimpleAllocator *) mem.getSpecificData();
   allocator->getFreeChunksList( list );
}

std::size_t ClusterDevice::getMemCapacity( SeparateMemoryAddressSpace const &mem ) const {
   SimpleAllocator *allocator = (SimpleAllocator *) mem.getSpecificData();
   return allocator->getCapacity();
}
