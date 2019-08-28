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

#ifndef _FPGA_DEVICE_DECL
#define _FPGA_DEVICE_DECL

#include "fpgadevice_fwd.hpp"
#include "workdescriptor_decl.hpp"
#include "processingelement_fwd.hpp"
#include "copydescriptor_decl.hpp"
#include "basethread.hpp" //for getMyThreadSafe() in warning/verbose, etc.

namespace nanos {
namespace ext {
   /* \breif Auxiliar class that contains the string with the FPGADevice architecture name.
    *        Cannot be a member of FPGADevice because the string constructor must be called before
    *        the Device constructor
    */
   struct FPGADeviceName {
      std::string _fpgaArchName;
      FPGADeviceName ( FPGADeviceType const t ) : _fpgaArchName( "FPGA " + toString(t) ) {}
   };

   /* \brief Device specialization for FPGA architecture
    * provides functions to allocate and copy data in the device
    */
   class FPGADevice : private FPGADeviceName, public Device
   {
      private:
         FPGADeviceType const    _fpgaType; ///< Type information

      public:

         FPGADevice ( FPGADeviceType const t );

         virtual ~FPGADevice () {}

         FPGADeviceType getFPGAType() const { return _fpgaType; }

         virtual void *memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem,
                 WD const *wd, unsigned int copyIdx);
         virtual void memFree( uint64_t addr, SeparateMemoryAddressSpace &mem );

         virtual void _canAllocate( SeparateMemoryAddressSpace &mem, std::size_t *sizes,
                 unsigned int numChunks, std::size_t *remainingSizes );

         virtual std::size_t getMemCapacity( SeparateMemoryAddressSpace &mem );

         virtual void _copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
               SeparateMemoryAddressSpace &mem, DeviceOps *ops,
               WD const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
               SeparateMemoryAddressSpace &mem, DeviceOps *ops,
               WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual bool _copyDevToDev( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len,
               SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memorig,
               DeviceOps *ops, WorkDescriptor const *wd, void *hostObject,
               reg_t hostRegionId );

         virtual void _getFreeMemoryChunksList( SeparateMemoryAddressSpace &mem,
               SimpleAllocator::ChunkList &list );

         virtual void _copyInStrided1D( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
               std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem,
               DeviceOps *ops, WorkDescriptor const *wd, void *hostObject,
               reg_t hostRegionId );

         virtual void _copyOutStrided1D( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
               std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem,
               DeviceOps *ops, WD const *wd, void *hostObject,
               reg_t hostRegionId );

         virtual bool _copyDevToDevStrided1D( uint64_t devDestAddr, uint64_t devOrigAddr,
               std::size_t len, std::size_t numChunks, std::size_t ld,
               SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig,
               DeviceOps *ops, WorkDescriptor const *wd, void *hostObject,
               reg_t hostRegionId );
   };
} // namespace ext
} // namespace nanos
#endif
