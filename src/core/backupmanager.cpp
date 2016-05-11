/*************************************************************************************/
/*      Copyright 2014 Barcelona Supercomputing Center                               */
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

#include "backupmanager.hpp"
#include "deviceops.hpp"
#include "exception/checkpointfailure.hpp"
#include "exception/restorefailure.hpp"

#include <sys/mman.h>
#include <iostream>

using namespace nanos;

BackupManager::BackupManager ( ) :
      Device("BackupMgr"), _memsize(0), _pool_addr(), _managed_pool() {}

BackupManager::BackupManager ( const char *n, size_t size ) :
      Device(n), _memsize(size),
      _pool_addr(mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)),
      _managed_pool(boost::interprocess::create_only, _pool_addr, size)
{
}

BackupManager::~BackupManager ( )
{
   munmap(_pool_addr, _memsize);
}

BackupManager & BackupManager::operator= ( BackupManager & arch )
{
   if (this != &arch) {
      Device::operator=(arch);
      _memsize = arch._memsize;
      _managed_pool.swap(arch._managed_pool);
   }
   return *this;
}

memory::Address BackupManager::memAllocate ( size_t size,
                                    SeparateMemoryAddressSpace &mem,
                                    WorkDescriptor const* wd,
                                    uint copyIdx )
{
   return _managed_pool.allocate(size);
}

void BackupManager::memFree ( memory::Address addr, SeparateMemoryAddressSpace &mem )
{
   _managed_pool.deallocate(addr);
}

void BackupManager::_canAllocate ( SeparateMemoryAddressSpace& mem,
                                   const std::vector<size_t>& sizes,
                                   std::vector<size_t>& remainingSizes )
{
}

std::size_t BackupManager::getMemCapacity (
      SeparateMemoryAddressSpace& mem )
{
   return _managed_pool.get_size();
}

bool BackupManager::checkpointCopy ( memory::Address devAddr, memory::Address hostAddr,
                              std::size_t len, SeparateMemoryAddressSpace &mem,
                              WorkDescriptor const* wd ) noexcept
{
   /* This is called on backup operations. Data is copied from host to device.
    * The operation is defined outside _copyIn because, for inout args we need
    * to create and manage private checkpoints, so passing through the dictionary and
    * region cache is necessary.
    */
   bool success;
   try {
      char* begin = static_cast<char*>(hostAddr);
      char* end = static_cast<char*>(hostAddr)+len;
      char* dest = static_cast<char*>(devAddr);
      /* We use another function call to perform the copy in order to
       * be able to compile std::copy call in a separate file.
       * This is needed to avoid the GCC bug related to 
       * non-call-exceptions plus inline and ipa-pure-const
       * optimizations.
       */
      rawCopy(begin, end, dest);

      success = true;
   } catch ( error::OperationFailure &e ) {
      error::CheckpointFailure error(e);

      success = false;
   }
   return success;
}

bool BackupManager::restoreCopy ( memory::Address hostAddr, memory::Address devAddr,
                               std::size_t len, SeparateMemoryAddressSpace &mem,
                               WorkDescriptor const* wd ) noexcept
{

   /* This is called on restore operations. Data is copied from device to host.
    * The operation is defined outside _copyOut because, for inout args, we need
    * to create and manage private checkpoints, so passing through the dictionary and
    * region cache is necessary.
    */
   bool success = false;
   do {
      try {
         char* begin = static_cast<char*>(devAddr);
         char* end = static_cast<char*>(devAddr)+len;
         char* dest = static_cast<char*>(hostAddr);
         /* We use another function call to perform the copy in order to
          * be able to compile std::copy call in a separate file.
          * This is needed to avoid the GCC bug related to
          * non-call-exceptions plus inline and ipa-pure-const
          * optimizations.
          */
         rawCopy(begin, end, dest);

         success = true;
      } catch ( error::OperationFailure &e ) {
         error::RestoreFailure error(e);
      }
   } while ( !success );

   return success;
}

void BackupManager::_copyIn ( memory::Address devAddr, memory::Address hostAddr,
                              std::size_t len, SeparateMemoryAddressSpace &mem,
                              DeviceOps *ops, WorkDescriptor const* wd, void *hostObject,
                              reg_t hostRegionId )
{
   ops->addOp();

   bool completed = checkpointCopy( devAddr, hostAddr, len, mem, wd );
   if ( completed ) {
      ops->completeOp();
   } else
      ops->abortOp();
}

void BackupManager::_copyOut ( memory::Address hostAddr, memory::Address devAddr,
                               std::size_t len, SeparateMemoryAddressSpace &mem,
                               DeviceOps *ops, WorkDescriptor const* wd, void *hostObject,
                               reg_t hostRegionId )
{
   ops->addOp();

   bool completed = restoreCopy( hostAddr, devAddr, len, mem, wd );
   if ( completed )
      ops->completeOp();
   else
      ops->abortOp();
}

bool BackupManager::_copyDevToDev ( memory::Address devDestAddr, memory::Address devOrigAddr,
                                    std::size_t len,
                                    SeparateMemoryAddressSpace &memDest,
                                    SeparateMemoryAddressSpace &memorig,
                                    DeviceOps *ops, WorkDescriptor const* wd,
                                    void *hostObject, reg_t hostRegionId )
{
   /* Device to device copies are not supported for BackupManager as only one instance
    * is expected for the whole process.
    */
   return false;
}

void BackupManager::_copyInStrided1D ( memory::Address devAddr, memory::Address hostAddr,
                                       std::size_t len, std::size_t numChunks,
                                       std::size_t ld,
                                       SeparateMemoryAddressSpace& mem,
                                       DeviceOps *ops, WorkDescriptor const* wd,
                                       void *hostObject, reg_t hostRegionId )
{
   ops->addOp();
   try {
      char* hostAddresses = (char*) hostAddr;
      char* deviceAddresses = (char*) devAddr;

      for (unsigned int i = 0; i < numChunks; i += 1) {
         //memcpy(&deviceAddresses[i * ld], &hostAddresses[i * ld], len);
         rawCopy((char*) &hostAddresses[i * ld], (char*) &hostAddresses[i * ld]+len, (char*) &deviceAddresses[i * ld]);
      }
      ops->completeOp();

   } catch ( error::OperationFailure &error ) {
      error::CheckpointFailure handler(error);

      ops->abortOp();
   }
}

void BackupManager::_copyOutStrided1D ( memory::Address hostAddr, memory::Address devAddr,
                                        std::size_t len, std::size_t numChunks,
                                        std::size_t ld,
                                        SeparateMemoryAddressSpace & mem,
                                        DeviceOps *ops, WorkDescriptor const* wd,
                                        void *hostObject, reg_t hostRegionId )
{
   ops->addOp();
   try {
      char* hostAddresses = (char*) hostAddr;
      char* deviceAddresses = (char*) devAddr;

      for (unsigned int i = 0; i < numChunks; i += 1) {
         //memcpy(&hostAddresses[i * ld], &deviceAddresses[i * ld], len);
         rawCopy((char*) &deviceAddresses[i * ld], (char*) &deviceAddresses[i * ld]+len, (char*) &hostAddresses[i * ld]);
      }
      ops->completeOp();
   } catch ( error::OperationFailure &error ) {
      error::CheckpointFailure handler(error);

      ops->abortOp();
   }
}

bool BackupManager::_copyDevToDevStrided1D (
      memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len,
      std::size_t numChunks, std::size_t ld,
      SeparateMemoryAddressSpace& memDest,
      SeparateMemoryAddressSpace& memOrig, DeviceOps *ops,
      WorkDescriptor const* wd, void *hostObject, reg_t hostRegionId )
{
   return false;
}

void BackupManager::_getFreeMemoryChunksList (
      SeparateMemoryAddressSpace& mem,
      SimpleAllocator::ChunkList &list )
{
   fatal(__PRETTY_FUNCTION__, "is not implemented.");
}

