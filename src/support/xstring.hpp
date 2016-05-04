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

#ifndef _NANOS_XSTRING
#define _NANOS_XSTRING

#include <sstream>
#include <string>
#include <utility>

namespace nanos {

template <class T>
inline std::string toString ( const T& t )
{
   std::stringstream ss;
   ss << t;
   return ss.str();
}

template <typename OStreamType>
static inline OStreamType &join( OStreamType &&os )
{
   os << std::endl;
   return os;
}

template <typename OStreamType, typename T, typename...Ts>
static inline OStreamType &join( OStreamType &&os, const T &first, const Ts&... rest)
{
   os << first;
   return join( os, rest... );
}

} // namespace nanos

#endif // _NANOS_XSTRING

