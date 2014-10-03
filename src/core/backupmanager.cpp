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
#include "taskexecutionexception.hpp"
#include <signal.h>
#include <sys/mman.h>

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

void * BackupManager::memAllocate ( size_t size,
                                    SeparateMemoryAddressSpace &mem,
                                    WD const& wd,
                                    uint copyIdx )
{
   return _managed_pool.allocate(size);
}

void BackupManager::memFree ( uint64_t addr, SeparateMemoryAddressSpace &mem )
{
   _managed_pool.deallocate((void*) addr);
}

void BackupManager::_canAllocate ( SeparateMemoryAddressSpace const& mem,
                                   size_t *sizes, uint numChunks,
                                   size_t *remainingSizes ) const
{

}

std::size_t BackupManager::getMemCapacity (
      SeparateMemoryAddressSpace const& mem ) const
{
   return _managed_pool.get_size();
}

void BackupManager::_copyIn ( uint64_t devAddr, uint64_t hostAddr,
                              std::size_t len, SeparateMemoryAddressSpace &mem,
                              DeviceOps *ops, Functor *f,
                              WorkDescriptor const& wd, void *hostObject,
                              reg_t hostRegionId ) const
{
   // Called on backup operations
   ops->addOp();
   try {
      memcpy((void*) devAddr, (void*) hostAddr, len);
   } catch ( TaskExecutionException &e ) {
      e.handle( );
      sys.getExceptionStats().incrInitializationErrors();
      debug("Resiliency: error detected during task " << wd.getId() << " input data backup.");
   }
   ops->completeOp();
}

void BackupManager::_copyOut ( uint64_t hostAddr, uint64_t devAddr,
                               std::size_t len, SeparateMemoryAddressSpace &mem,
                               DeviceOps *ops, Functor *f,
                               WorkDescriptor const& wd, void *hostObject,
                               reg_t hostRegionId ) const
{
   // This is called on restore operations
   ops->addOp();
   try {
      memcpy((void*) hostAddr, (void*) devAddr, len);
   } catch ( TaskExecutionException &e ) {
      e.handle( );
      sys.getExceptionStats().incrInitializationErrors();
      debug("Resiliency: error detected during task " << wd.getId() << " input data restoration.");
   }

   ops->completeOp();
}

bool BackupManager::_copyDevToDev ( uint64_t devDestAddr, uint64_t devOrigAddr,
                                    std::size_t len,
                                    SeparateMemoryAddressSpace &memDest,
                                    SeparateMemoryAddressSpace &memorig,
                                    DeviceOps *ops, Functor *f,
                                    WorkDescriptor const& wd, void *hostObject,
                                    reg_t hostRegionId ) const
{
   /* Device to device copies are not supported for BackupManager as only one instance
    * is expected for the whole process.
    */
   return false;
}

void BackupManager::_copyInStrided1D ( uint64_t devAddr, uint64_t hostAddr,
                                       std::size_t len, std::size_t numChunks,
                                       std::size_t ld,
                                       SeparateMemoryAddressSpace const& mem,
                                       DeviceOps *ops, Functor *f,
                                       WorkDescriptor const& wd,
                                       void *hostObject, reg_t hostRegionId )
{
   char* hostAddresses = (char*) hostAddr;
   char* deviceAddresses = (char*) devAddr;

   ops->addOp();
   try {
      for (unsigned int i = 0; i < numChunks; i += 1) {
         memcpy(&deviceAddresses[i * ld], &hostAddresses[i * ld], len);
      }
   } catch ( TaskExecutionException &e ) {
      e.handle( );
      sys.getExceptionStats().incrInitializationErrors();
      debug("Resiliency: error detected during task " << wd.getId() << " input data backup.");
   }
   ops->completeOp();
}

void BackupManager::_copyOutStrided1D ( uint64_t hostAddr, uint64_t devAddr,
                                        std::size_t len, std::size_t numChunks,
                                        std::size_t ld,
                                        SeparateMemoryAddressSpace & mem,
                                        DeviceOps *ops, Functor *f,
                                        WorkDescriptor const& wd,
                                        void *hostObject, reg_t hostRegionId )
{
   char* hostAddresses = (char*) hostAddr;
   char* deviceAddresses = (char*) devAddr;

   ops->addOp();
   try {
      for (unsigned int i = 0; i < numChunks; i += 1) {
         memcpy(&hostAddresses[i * ld], &deviceAddresses[i * ld], len);
      }
   } catch ( TaskExecutionException &e ) {
      e.handle( );
      sys.getExceptionStats().incrInitializationErrors();
      debug("Resiliency: error detected during task " << wd.getId() << " input data restoration.");
   }
   ops->completeOp();
}

bool BackupManager::_copyDevToDevStrided1D (
      uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len,
      std::size_t numChunks, std::size_t ld,
      SeparateMemoryAddressSpace const& memDest,
      SeparateMemoryAddressSpace const& memOrig, DeviceOps *ops, Functor *f,
      WorkDescriptor const& wd, void *hostObject, reg_t hostRegionId ) const
{
   return false;
}

void BackupManager::_getFreeMemoryChunksList (
      SeparateMemoryAddressSpace const& mem,
      SimpleAllocator::ChunkList &list ) const
{
   fatal(__PRETTY_FUNCTION__ << "is not implemented.");
}
