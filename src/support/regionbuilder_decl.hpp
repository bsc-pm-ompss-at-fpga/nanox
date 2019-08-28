/*************************************************************************************/
/*      Copyright 2009-2018 Barcelona Supercomputing Center                          */
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
/*      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            */
/*************************************************************************************/


#ifndef _NANOS_REGION_BUILDER_DECL
#define _NANOS_REGION_BUILDER_DECL


//#include <cstddef>
#include <stddef.h>

#include "region_fwd.hpp"


namespace nanos
{
   /*! \class RegionBuilder
    *  \brief A class that can generate a single dimension \a Region from an address and the dimension
    */
   class RegionBuilder {
   public:
      //! \brief Generate a single dimension \a Region from an address and the dimension
      //! \param address base address of the region
      //! \param base base of the current dimension
      //! \param length length in \a base elements of the dimension
      //! \param[in,out] additionalContribution carried over additional contribution generated by lesser significant dimensions on input, and with this dimension's contributiou on ouput
      //! \return the region
      static Region build(size_t address, size_t base, size_t length, size_t /* INOUT */ &additionalContribution);
   };
   
   
} // namespace nanos


#endif // _NANOS_REGION_BUILDER_DECL
