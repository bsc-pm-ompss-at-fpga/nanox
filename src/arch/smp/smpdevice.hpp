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

#ifndef _SMP_DEVICE_DECL
#define _SMP_DEVICE_DECL

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "workdescriptor_decl.hpp"
#include "processingelement_fwd.hpp"
#include "copydescriptor.hpp"
#include "system_decl.hpp"
#include "smptransferqueue.hpp"
#include "globalregt.hpp"

namespace nanos {

SMPDevice::SMPDevice ( const char *n ) : Device ( n ), _transferQueue() {}
SMPDevice::SMPDevice ( const SMPDevice &arch ) : Device ( arch ), _transferQueue() {}

/*! \brief SMPDevice destructor
 */
SMPDevice::~SMPDevice()
{
};

memory::Address SMPDevice::memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem, WD const *wd, unsigned int copyIdx ) {
   void *retAddr = NULL;

   SimpleAllocator *sallocator = (SimpleAllocator *) mem.getSpecificData();
   sallocator->lock();
   retAddr = sallocator->allocate( size );
   sallocator->unlock();
   return retAddr;
}

void SMPDevice::memFree( memory::Address addr, SeparateMemoryAddressSpace &mem ) {
   SimpleAllocator *sallocator = (SimpleAllocator *) mem.getSpecificData();
   sallocator->lock();
   sallocator->free( (void *) addr );
   sallocator->unlock();
}

void SMPDevice::_canAllocate( SeparateMemoryAddressSpace &mem, const std::vector<size_t>& sizes, std::vector<size_t>& remainingSizes ) {
   static_cast<SimpleAllocator*>(mem.getSpecificData())->canAllocate( sizes, remainingSizes );
}

std::size_t SMPDevice::getMemCapacity( SeparateMemoryAddressSpace &mem ) {
   return static_cast<SimpleAllocator*>(mem.getSpecificData())->getCapacity();
}

void SMPDevice::_copyIn( memory::Address devAddr, memory::Address hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {
   if ( sys.getSMPPlugin()->asyncTransfersEnabled() ) {
      _transferQueue.addTransfer( ops, ((char *) devAddr), ((char *) hostAddr), len, 1, 0, true );
   } else {
      ops->addOp();
      NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
      NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("cache-copy-in"); )
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, (nanos_event_value_t) len ); )
      if (sys._watchAddr != NULL ) {
         if (sys._watchAddr >= hostAddr && sys._watchAddr < hostAddr + len) {
            *myThread->_file << "WATCH update dev: value " << *((double *) sys._watchAddr )<< std::endl;
         }
      }
      ::memcpy( devAddr, hostAddr, len );
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, (nanos_event_value_t) 0 ); )
      ops->completeOp();
   }
}

void SMPDevice::_copyOut( memory::Address hostAddr, memory::Address devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {
   if ( sys.getSMPPlugin()->asyncTransfersEnabled() ) {
      _transferQueue.addTransfer( ops, ((char *) hostAddr), ((char *) devAddr), len, 1, 0, true );
   } else {
      ops->addOp();
      NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
      NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("cache-copy-out"); )
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, (nanos_event_value_t) len ); )
      if (sys._watchAddr != NULL ) {
         if (sys._watchAddr >= hostAddr && sys._watchAddr < hostAddr + len) {
            *myThread->_file << "WATCH update host: old value " << *((double *) sys._watchAddr )<< std::endl;
         }
      }
      ::memcpy( hostAddr, devAddr, len );
      if (sys._watchAddr != NULL ) {
         if (sys._watchAddr >= hostAddr && sys._watchAddr < hostAddr + len) {
            *myThread->_file << "WATCH update host: new value " << *((double *) sys._watchAddr )<< std::endl;
         }
      }
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, (nanos_event_value_t) 0 ); )
      ops->completeOp();
   }
}

bool SMPDevice::_copyDevToDev( memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memorig, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {
   if ( sys.getSMPPlugin()->asyncTransfersEnabled() ) {
      _transferQueue.addTransfer( ops, ((char *) devDestAddr), ((char *) devOrigAddr), len, 1, 0, true );
   } else {
      ops->addOp();
      NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
      NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("cache-copy-in"); )
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, (nanos_event_value_t) len ); )
      if (sys._watchAddr != NULL ) {
         global_reg_t reg( hostRegionId, sys.getHostMemory().getRegionDirectoryKey( (uint64_t) hostObject ) );
         memory::Address target_addr = reg.getKeyFirstAddress();
         if (sys._watchAddr >= target_addr && sys._watchAddr < target_addr + reg.getBreadth() ) {
            ptrdiff_t offset = sys._watchAddr - target_addr;
            *myThread->_file << "WATCH update dev from dev: old value [ " << (void *)(devDestAddr + offset) << " ] " << *((double *) (devDestAddr + offset) ) << " set value [from " << (void *)(devOrigAddr + offset) << " ] " << *((double *) (devOrigAddr + offset) )<< std::endl;
         }
      }
      ::memcpy( devDestAddr, devOrigAddr, len );
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, (nanos_event_value_t) 0 ); )
      ops->completeOp();
   }
   return true;
}

void SMPDevice::_copyInStrided1D( memory::Address devAddr, memory::Address hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {
   if ( sys.getSMPPlugin()->asyncTransfersEnabled() ) {
      _transferQueue.addTransfer( ops, ((char *) devAddr), ((char *) hostAddr), len, numChunks, ld, true );
   } else {
      ops->addOp();
      NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
      NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("cache-copy-in"); )
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, (nanos_event_value_t) 2 ); )
      for ( std::size_t count = 0; count < numChunks; count += 1) {
         ::memcpy( ((char *) devAddr) + count * ld, ((char *) hostAddr) + count * ld, len );
      }
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, (nanos_event_value_t) 0 ); )
      ops->completeOp();
   }
}

void SMPDevice::_copyOutStrided1D( memory::Address hostAddr, memory::Address devAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {
   if ( sys.getSMPPlugin()->asyncTransfersEnabled() ) {
      _transferQueue.addTransfer( ops, ((char *) hostAddr), ((char *) devAddr), len, numChunks, ld, false );
   } else {
      ops->addOp();
      NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
      NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("cache-copy-out"); )
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, (nanos_event_value_t) 2 ); )
      for ( std::size_t count = 0; count < numChunks; count += 1) {
         ::memcpy( ((char *) hostAddr) + count * ld, ((char *) devAddr) + count * ld, len );
      }
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, (nanos_event_value_t) 0 ); )
      ops->completeOp();
   }
}

bool SMPDevice::_copyDevToDevStrided1D( memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {
   if ( sys.getSMPPlugin()->asyncTransfersEnabled() ) {
      _transferQueue.addTransfer( ops, ((char *) devDestAddr), ((char *) devOrigAddr), len, numChunks, ld, true );
   } else {
      ops->addOp();
      NANOS_INSTRUMENT ( static InstrumentationDictionary *ID = sys.getInstrumentation()->getInstrumentationDictionary(); )
      NANOS_INSTRUMENT ( static nanos_event_key_t key = ID->getEventKey("cache-copy-in"); )
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseOpenBurstEvent( key, (nanos_event_value_t) 2 ); )
      for ( std::size_t count = 0; count < numChunks; count += 1) {
         ::memcpy( ((char *) devDestAddr) + count * ld, ((char *) devOrigAddr) + count * ld, len );
      }
      NANOS_INSTRUMENT( sys.getInstrumentation()->raiseCloseBurstEvent( key, (nanos_event_value_t) 0 ); )
      ops->completeOp();
   }
   return true;
}

void SMPDevice::_getFreeMemoryChunksList( SeparateMemoryAddressSpace &mem, SimpleAllocator::ChunkList &list ) {
   SimpleAllocator *sallocator = (SimpleAllocator *) mem.getSpecificData();
   sallocator->getFreeChunksList( list );
}

void SMPDevice::tryExecuteTransfer() {
   _transferQueue.tryExecuteOne();
}

} // namespace nanos

#endif

