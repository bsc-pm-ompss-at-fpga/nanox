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

#ifndef _SMP_DEVICE
#define _SMP_DEVICE

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "workdescriptor_decl.hpp"
#include "processingelement_fwd.hpp"
#include "copydescriptor.hpp"
#include "smptransferqueue_decl.hpp"

namespace nanos {

  /* \brief Device specialization for SMP architecture
   * provides functions to allocate and copy data in the device
   */
   class SMPDevice : public Device
   {
      SMPTransferQueue _transferQueue;
      public:
         /*! \brief SMPDevice constructor
          */
         SMPDevice ( const char *n );

         /*! \brief SMPDevice copy constructor
          */
         SMPDevice ( const SMPDevice &arch );

         /*! \brief SMPDevice destructor
          */
         ~SMPDevice();

         virtual memory::Address memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem, WD const *wd, unsigned int copyIdx );

         virtual void memFree( memory::Address addr, SeparateMemoryAddressSpace &mem );

         virtual void _canAllocate( SeparateMemoryAddressSpace &mem, const std::vector<size_t>& sizes, std::vector<size_t>& remainingSizes );

         virtual std::size_t getMemCapacity( SeparateMemoryAddressSpace &mem );

         virtual void _copyIn( memory::Address devAddr, memory::Address hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyOut( memory::Address hostAddr, memory::Address devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual bool _copyDevToDev( memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memorig, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyInStrided1D( memory::Address devAddr, memory::Address hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyOutStrided1D( memory::Address hostAddr, memory::Address devAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual bool _copyDevToDevStrided1D( memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _getFreeMemoryChunksList( SeparateMemoryAddressSpace &mem, SimpleAllocator::ChunkList &list );

         void tryExecuteTransfer();

   };
} // namespace nanos

#endif
