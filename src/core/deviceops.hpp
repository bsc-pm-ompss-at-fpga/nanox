/*************************************************************************************/
/*      Copyright 2015 Barcelona Supercomputing Center                               */
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

#ifndef DEVICEOPS_HPP
#define DEVICEOPS_HPP

#include "basethread_decl.hpp"
#include "deviceops_decl.hpp"
#include "workdescriptor_decl.hpp"

#include "debug.hpp"
#include "error.hpp"
#include "lock.hpp"
#include "os.hpp"

#include <iostream>

#define VERBOSE_CACHE_OPS 0

namespace nanos {

inline DeviceOps::DeviceOps() : _pendingDeviceOps ( 0 ), _aborted( false ), _owner( -1 ), _wd( NULL ), _loc( 0 ) {
}

inline DeviceOps::~DeviceOps() {
}

inline void DeviceOps::addOp() {
   _pendingDeviceOps++;
}

inline bool DeviceOps::aborted() {
   return _aborted.value();
}

inline bool DeviceOps::allCommited() {
   return !aborted() && allCompleted();
}

inline bool DeviceOps::allCompleted() {
   bool b = ( _pendingDeviceOps.value() == 0);
   return b;
}

inline bool DeviceOps::addCacheOp( /* debug: */ WorkDescriptor const *wd, int loc ) {
   ensure( wd != NULL, "Invalid WD adding a Cache Op.");

   bool success = false;
   if ( _pendingCacheOp.tryAcquire() ) {
      if ( VERBOSE_CACHE_OPS ) {
         message( this, " Added op ", std::hex, this, " by ", wd->getId(), " at loc ", loc );
      }
      _wd = wd;
      _owner = wd->getId();
      _loc = loc;

      success = true;
   }
   return success;
}

inline bool DeviceOps::allCacheOpsCompleted() const {
   return _pendingCacheOp.getState() == NANOS_LOCK_FREE;
}

inline void DeviceOps::completeOp() {
   _pendingDeviceOps--;
}

inline void DeviceOps::abortOp() {
   _pendingDeviceOps--;
   _aborted = true;
}

inline void DeviceOps::completeCacheOp( /* debug: */ WorkDescriptor const *wd ) {
   ensure( _pendingCacheOp.getState() != NANOS_LOCK_FREE, "Already completed op!" );
        ensure( wd == _wd, "Invalid owner clearing a cache op." );
        if ( VERBOSE_CACHE_OPS ) {
           *(myThread->_file) << "[" << myThread->getId() << "] " << OS::getMonotonicTime() << " " << (void *)this << " cleared an op by " << wd->getId() << std::endl;
        }
        _wd = NULL;
        _owner = -1;
        _loc = 0;
   _pendingCacheOp.release();
}

inline int DeviceOps::getPendingDeviceOpNumber() const {
   return _pendingDeviceOps.value();
}

inline std::ostream & operator<< ( std::ostream &o, nanos::DeviceOps const &ops ) {
   o << "{ pending device ops: " << ops.getPendingDeviceOpNumber();
   if( !ops.allCacheOpsCompleted() ) {
      o << "(ops pending) ";
   }
   o << " owner: " << ops._owner << "}";
   return o;
}

} // namespace nanos

#endif /* DEVICEOPS_HPP */
