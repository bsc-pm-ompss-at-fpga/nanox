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

#ifndef TASK_RECOVERY_FAILED_HPP
#define TASK_RECOVERY_FAILED_HPP

#include "genericexception.hpp"
#include "workdescriptor.hpp"

namespace nanos {
namespace error {

class TaskRecoveryFailed : public GenericException {
   public:
      TaskRecoveryFailed() :
         GenericException( "Resiliency: task recovery was unsuccessful" )
      {
      }
};

} // namespace error
} // namespace nanos

#endif // TASK_RECOVERY_FAILED_HPP

