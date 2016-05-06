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

#ifndef INVALIDATED_REGION_FOUND_HPP
#define  INVALIDATED_REGION_FOUND_HPP

#include <stdexcept>

namespace nanos {
namespace error {

class InvalidatedRegionFound : public std::runtime_error {
   public:
      InvalidatedRegionFound() :
         std::runtime_error( "Error: trying to restore from a corrupted backup." )
      {
      }
};

} // namespace error
} // namespace nanos

#endif // INVALIDATED_REGION_FOUND_HPP
