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

#include "backupmanager_fwd.hpp"

#include "workdescriptor_decl.hpp"

#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/indexes/null_index.hpp>

#include <cstddef>

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
         BackupManager ( );

         BackupManager ( const char *n, size_t memsize );

         virtual ~BackupManager();

         // Warning: cannot reuse the source object because its _managed_pool object is invalidated
         BackupManager & operator= ( BackupManager& arch );

         virtual memory::Address memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem, WorkDescriptor const* wd, unsigned int copyIdx);

         virtual void memFree (memory::Address addr, SeparateMemoryAddressSpace &mem);

         virtual void _canAllocate( SeparateMemoryAddressSpace &mem, const std::vector<size_t>& sizes, std::vector<size_t>& remainingSizes );

         virtual std::size_t getMemCapacity( SeparateMemoryAddressSpace& mem );

         //! \brief Intermediate function used to bypass a bug with GCC ipa-pure-const and inline optimizations.
         void rawCopy ( char *begin, char *end, char *dest );

         //! \brief Makes a copy of the given chunk into device memory. This should be called on checkpoint operations only.
         virtual bool checkpointCopy ( memory::Address devAddr, memory::Address hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, WorkDescriptor const* wd ) noexcept;

         //! \brief Makes a copy of the given chunk back into host memory. This should be called on restore operations only.
         virtual bool restoreCopy ( memory::Address hostAddr, memory::Address devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, WorkDescriptor const* wd ) noexcept;

         virtual void _copyIn( memory::Address devAddr, memory::Address hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyOut( memory::Address hostAddr, memory::Address devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual bool _copyDevToDev( memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memorig, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyInStrided1D( memory::Address devAddr, memory::Address hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _copyOutStrided1D( memory::Address hostAddr, memory::Address devAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual bool _copyDevToDevStrided1D( memory::Address devDestAddr, memory::Address devOrigAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId );

         virtual void _getFreeMemoryChunksList( SeparateMemoryAddressSpace &mem, SimpleAllocator::ChunkList &list );
   };
} // namespace nanos

#endif /* BACKUPMANAGER_HPP_ */
