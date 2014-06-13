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

BackupManager nanos::ext::Backup( "BackupMgr", 20000000 );

BackupManager::BackupManager ( const char *n, size_t size ): Device ( n ), _memsize(size), _pool_addr(malloc(size)), _managed_pool(boost::interprocess::create_only, _pool_addr, size) {}

/*BackupManager::BackupManager ( const BackupManager &arch ) : Device ( arch ), _memsize(arch._memsize){
   _managed_pool = arch._managed_pool;
}*/

BackupManager::~BackupManager(){
   free(_pool_addr);
}

//debe ser privado
BackupManager & BackupManager::operator= ( BackupManager &arch ){
   if(this!=&arch){
      Device::operator=(arch);
      _memsize = arch._memsize;
      _managed_pool.swap(arch._managed_pool);
   }
   return *this;
}

/*const bool BackupManager::operator== ( const BackupManager &arch ){
   return Device::operator==(arch);
}*/

void * BackupManager::memAllocate ( size_t size, SeparateMemoryAddressSpace &mem,
                    uint64_t targetHostAddr) {
   return _managed_pool.allocate(size);
}

void BackupManager::memFree ( uint64_t addr, SeparateMemoryAddressSpace &mem ) {
   _managed_pool.deallocate((void*)addr);
}

void BackupManager::_canAllocate ( SeparateMemoryAddressSpace const &mem, size_t *sizes,
                    uint numChunks, size_t *remainingSizes ) const {

}

std::size_t BackupManager::getMemCapacity ( SeparateMemoryAddressSpace const &mem ) const {
   return _managed_pool.get_size();
}

void BackupManager::_copyIn ( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
               SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f,
               WorkDescriptor const &wd, void *hostObject,
               reg_t hostRegionId ) const {
   ops->addOp();
   //devAddr es un offset?
   //hostAddr es un offset?
   // Creo que hay que utilizar el separate memory address space para calcular la direccion absoluta
   memcpy((void*)devAddr,(void*)hostAddr,len);
   ops->completeOp();
}

void BackupManager::_copyOut ( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
                SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f,
                WorkDescriptor const &wd, void *hostObject,
                reg_t hostRegionId ) const {
   // Atm this does nothing, as we only have to care when functions is called, not when function exits.
   // This could be the place to invalidate/free backups that are no longer useful (i.e. backup data that will not be needed any more).
   std::cerr << "wrong copyOut" << std::endl;
}

bool BackupManager::_copyDevToDev ( uint64_t devDestAddr, uint64_t devOrigAddr,
                     std::size_t len, SeparateMemoryAddressSpace &memDest,
                     SeparateMemoryAddressSpace &memorig, DeviceOps *ops,
                     Functor *f, WorkDescriptor const &wd, void *hostObject,
                     reg_t hostRegionId ) const {
   //ops->addOp();
   //ops->completeOp();
   std::cerr << "wrong copyDevToDev" << std::endl;
   return true;
}

void BackupManager::_copyInStrided1D ( uint64_t devAddr, uint64_t hostAddr, std::size_t len,
                        std::size_t numChunks, std::size_t ld,
                        SeparateMemoryAddressSpace const &mem, DeviceOps *ops,
                        Functor *f, WorkDescriptor const &wd, void *hostObject,
                        reg_t hostRegionId )
{
   /*
   ops->addOp();
   for(int i = 0; i < numChunks; i++){

   }
   ops->completeOp();
   */
   std::cerr << "wrong copyIn" << std::endl;

}

void BackupManager::_copyOutStrided1D ( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
                         std::size_t numChunks, std::size_t ld,
                         SeparateMemoryAddressSpace const &mem, DeviceOps *ops,
                         Functor *f, WorkDescriptor const &wd, void *hostObject,
                         reg_t hostRegionId )
{
   /*
   ops->addOp();

  ops->completeOp();*/
   std::cerr << "wrong copyOut" << std::endl;
}

bool BackupManager::_copyDevToDevStrided1D ( uint64_t devDestAddr, uint64_t devOrigAddr,
                              std::size_t len, std::size_t numChunks,
                              std::size_t ld,
                              SeparateMemoryAddressSpace const &memDest,
                              SeparateMemoryAddressSpace const &memOrig,
                              DeviceOps *ops, Functor *f,
                              WorkDescriptor const &wd, void *hostObject,
                              reg_t hostRegionId )
{
   /*
   ops->addOp();

   ops->completeOp();*/
   std::cerr << "wrong copyDevToDev" << std::endl;
   //return false;
   return true;
}

void BackupManager::_getFreeMemoryChunksList ( SeparateMemoryAddressSpace const &mem,
                                SimpleAllocator::ChunkList &list ) const
{
   std::cerr << "wrong _getFreeMemoryChunksList()" << std::endl;
}
