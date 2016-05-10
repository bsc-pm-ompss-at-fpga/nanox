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

#ifndef BACKUP_PRIVATE_COPY_HPP
#define BACKUP_PRIVATE_COPY_HPP

#include "backupmanager.hpp"
#include "memcachecopy_decl.hpp"
#include "system_decl.hpp"
#include "regioncache.hpp"

#include "debug.hpp"

namespace nanos {

BackupPrivateCopy::BackupPrivateCopy( const CopyData& copy, const WorkDescriptor* wd, unsigned index ) :
   RemoteChunk( copy.getFitAddress(), nullptr, copy.getSize() ),
   _device( reinterpret_cast<BackupManager&>(sys.getBackupMemory().getCache().getDevice()) ),
   _aborted( false )
{
   setDeviceAddress( _device.memAllocate( getSize(), sys.getBackupMemory(), wd, index ) );
}

BackupPrivateCopy::BackupPrivateCopy( BackupPrivateCopy&& other ) :
   RemoteChunk( std::move(other) ),
   _device( other._device ),
   _aborted( other._aborted )
{
   // Do not free the device address once the other
   // object is destroyed
   other.setDeviceAddress(nullptr);
}

BackupPrivateCopy::~BackupPrivateCopy()
{
   // In case this object has been moved away
   if( getDeviceAddress() != nullptr ) {
      _device.memFree( getDeviceAddress(), sys.getBackupMemory() );
   }
}

void BackupPrivateCopy::checkpoint( const WorkDescriptor* wd )
{
   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_INOUT );
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

   _aborted = !_device.checkpointCopy( getDeviceAddress(), getHostAddress(), getSize(),
                                     sys.getBackupMemory(), wd );

   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
}

void BackupPrivateCopy::restore( const WorkDescriptor* wd )
{
   if( !_aborted ) {
      NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
      NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_RT_INOUT );
      NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

      _aborted = !_device.restoreCopy( getHostAddress(), getDeviceAddress(), getSize(),
                                  sys.getBackupMemory(), wd );

      NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
   }
}

} // namespace nanos 

#endif // BACKUP_PRIVATE_COPY_HPP

