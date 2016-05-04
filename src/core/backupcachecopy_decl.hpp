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

#ifndef BACKUP_CACHE_COPY_DECL
#define BACKUP_CACHE_COPY_DECL

#include "memcachecopy_decl.hpp"

namespace nanos {

class BackupCacheCopy : public MemCacheCopy {
   public:
      BackupCacheCopy( const CopyData& copy, const MemCacheCopy& memCacheCopy, const WorkDescriptor& wd, unsigned index ) :
         MemCacheCopy( wd, index )
      {
         setVersion( memCacheCopy.getVersion() );

         if( copy.isInput() ) {
            _locations.insert( _locations.end(), memCacheCopy._locations.begin(), memCacheCopy._locations.end() );
         }
      }
};

} // namespace nanos

#endif // BACKUP_CACHE_COPY_DECL
