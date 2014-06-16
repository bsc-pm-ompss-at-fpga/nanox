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

#ifndef BACKUPMANAGER_HPP_
#define BACKUPMANAGER_HPP_

#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/indexes/null_index.hpp>

#include "workdescriptor_decl.hpp"

namespace boost {
   namespace interprocess {

      typedef boost::interprocess::basic_managed_external_buffer
            <char
            ,rbtree_best_fit<mutex_family,offset_ptr<void> >
            ,null_index
            > managed_buffer;
   }
}

namespace nanos {

   class BackupManager : public Device {
      private:
         size_t                              _memsize;
         void                               *_pool_addr;
         boost::interprocess::managed_buffer _managed_pool;

      public:
         BackupManager ( const char *n, size_t memsize );

         //BackupManager ( const BackupManager &arch );

         virtual ~BackupManager();

         //debe ser privado
         BackupManager & operator= ( BackupManager &arch );

         //const bool operator== ( const BackupManager &arch );

         virtual void *memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem, uint64_t targetHostAddr);

         virtual void memFree (uint64_t addr, SeparateMemoryAddressSpace &mem);

         virtual void _canAllocate( SeparateMemoryAddressSpace const &mem, size_t *sizes, uint numChunks, size_t *remainingSizes ) const;

         virtual std::size_t getMemCapacity( SeparateMemoryAddressSpace const &mem ) const;

         virtual void _copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WorkDescriptor const &wd, void *hostObject, reg_t hostRegionId ) const;

         virtual void _copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WorkDescriptor const &wd, void *hostObject, reg_t hostRegionId ) const;

         virtual bool _copyDevToDev( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memorig, DeviceOps *ops, Functor *f, WorkDescriptor const &wd, void *hostObject, reg_t hostRegionId ) const;

         virtual void _copyInStrided1D( uint64_t devAddr, uint64_t hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace const &mem, DeviceOps *ops, Functor *f, WorkDescriptor const &wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyOutStrided1D( uint64_t hostAddr, uint64_t devAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace const &mem, DeviceOps *ops, Functor *f, WorkDescriptor const &wd, void *hostObject, reg_t hostRegionId );

         virtual bool _copyDevToDevStrided1D( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace const &memDest, SeparateMemoryAddressSpace const &memOrig, DeviceOps *ops, Functor *f, WorkDescriptor const &wd, void *hostObject, reg_t hostRegionId );

         virtual void _getFreeMemoryChunksList( SeparateMemoryAddressSpace const &mem, SimpleAllocator::ChunkList &list ) const;
   };

   namespace ext {
      extern BackupManager Backup;
   }
} // namespace nanos

#endif /* BACKUPMANAGER_HPP_ */