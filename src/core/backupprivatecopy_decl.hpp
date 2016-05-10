/*************************************************************************************/
/*      Copyright 2009-2016 Barcelona Supercomputing Center                          */
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

#ifndef BACKUP_PRIVATE_COPY_DECL
#define BACKUP_PRIVATE_COPY_DECL

#include "backupmanager_fwd.hpp"

#include "memcachecopy_decl.hpp"
#include "regioncache_decl.hpp"

namespace nanos {

class BackupPrivateCopy : public RemoteChunk {
   private:
      BackupManager &_device;
      bool           _aborted;

   public:
      BackupPrivateCopy( const CopyData& copy, const WorkDescriptor* wd, unsigned index );

      BackupPrivateCopy( const BackupPrivateCopy& ) = delete;

      BackupPrivateCopy( BackupPrivateCopy&& other );

      virtual ~BackupPrivateCopy();

      void checkpoint( const WorkDescriptor* wd );

      void restore( const WorkDescriptor* wd );

      bool isAborted() const { return _aborted; }
};

} // namespace nanos

#endif // PRIVATE_BACKUP_COPY_DECL

