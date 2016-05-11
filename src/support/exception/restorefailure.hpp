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

#ifndef RESTOREFAILURE_HPP
#define RESTOREFAILURE_HPP

#include "operationfailure.hpp"
#include "failurestats.hpp"
#include "workdescriptor.hpp"

namespace nanos {
namespace error {

class RestoreFailure {
   private:
      OperationFailure& _failedOperation;
   public:
      RestoreFailure( OperationFailure& operation ) :
            _failedOperation( operation )
      {
         FailureStats<RestoreFailure>::increase();

         // Just assume we can restore again because the backup has not been corrupted.
         // The memory page has been remapped by OperationFailure constructor, so we can restart it.

         //WorkDescriptor* recoverableAncestor = getTask().propagateInvalidationAndGetRecoverableAncestor();
         //if( !recoverableAncestor ) {
         //   fatal( "Could not find a recoverable task when recovering from ", what() );
         //}
         debug("Resiliency: restore error detected. ", operation.what() );
      }
};

} // namespace error
} // namespace nanos

#endif // RESTORE_FAILURE_HPP

