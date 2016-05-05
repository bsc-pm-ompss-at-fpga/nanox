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


#ifndef MEMCONTROLLER_HPP
#define MEMCONTROLLER_HPP

#include "memcontroller_decl.hpp"
#include "workdescriptor.hpp"

namespace nanos {

inline std::ostream& operator<<( std::ostream& os, const MemController& mcontrol )
{
   const WorkDescriptor* wd = mcontrol.getWorkDescriptor();
   os << "Number of copies " << wd->getNumCopies() << std::endl;
   for( unsigned index = 0; index < wd->getNumCopies(); index++ ) {
      const CopyData& copy = wd->getCopies()[index];
      const MemCacheCopy& mCacheCopy = mcontrol._memCacheCopies[index];

      NewNewDirectoryEntryData *entry = NewNewRegionDirectory::getDirectoryEntry(
                                             *(mCacheCopy._reg.key), mCacheCopy._reg.id );
      if( copy.isInput() )
         os << "in ";
      if( copy.isOutput() )
         os << "out ";

      if( entry )
         os << *entry << std::endl;
      else
         os << "dir entry n/a" << std::endl;

      mCacheCopy._reg.key->printRegion( os, mCacheCopy._reg.id ) ;
   }
   return os;
}

} // namespace nanos

#endif

