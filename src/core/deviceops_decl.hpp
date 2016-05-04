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

#ifndef DEVICEOPS_DECL_HPP
#define DEVICEOPS_DECL_HPP

#include <set>
#include "atomic_decl.hpp"
#include "lock_decl.hpp"
#include "deviceops_fwd.hpp"
#include "workdescriptor_fwd.hpp"

namespace nanos {

   class DeviceOps {
      private:
         Atomic<unsigned int> _pendingDeviceOps;
         Atomic<bool> _aborted;
         Lock _pendingCacheOp;

      public:
         /*debug:*/ int _owner;
         /*debug:*/ WorkDescriptor const *_wd;
         /*debug:*/ int _loc;

         DeviceOps();
         ~DeviceOps();

         void abortOp();
         void completeOp();
         void addOp();

         bool aborted();
         bool allCommited();
         bool allCompleted();

         bool addCacheOp( /* debug: */ WorkDescriptor const *wd, int loc = -1 );
         void completeCacheOp( /* debug: */WorkDescriptor const *wd );
         bool allCacheOpsCompleted() const;
         int getPendingDeviceOpNumber() const;
   };

} // namespace nanos

#endif /* DEVICEOPS_DECL_HPP */
