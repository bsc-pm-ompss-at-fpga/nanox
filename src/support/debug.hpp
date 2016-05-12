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

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "xstring.hpp"

#include <stdexcept>
#include <iostream>
#include <sstream>

#if VERBOSE_CACHE
 #define _VERBOSE_CACHE 1
#else
 #define _VERBOSE_CACHE 0
#endif

namespace nanos {

#define _nanos_ostream std::cerr

bool verboseEnabled();

std::string getMyThreadId();

int getNodeNumber();

#ifdef NANOS_DEBUG_ENABLED

template <typename...Ts>
inline void verbose( const Ts&... msg )
{
   if( verboseEnabled() ) {
      join( _nanos_ostream, "[", std::dec, getMyThreadId(), "] ", msg... );
   }
}

template <typename...Ts>
inline void verbose_cache( const Ts&... msg )
{
   if( _VERBOSE_CACHE ) {
      verbose( msg... );
   }
}

template <typename...Ts>
inline void debug( const Ts&... msg )
{
   if( verboseEnabled() ) {
      join( _nanos_ostream, "DBG [", std::dec, getMyThreadId(), "] ", msg... );
   }
}

#else

template <typename...Ts>
inline void verbose( const Ts&... msg ) {}

template <typename...Ts>
inline void verbose_cache( const Ts&... msg ) {}

template <typename...Ts>
inline void debug( const Ts&... msg ) {}

#endif // NANOS_DEBUG_ENABLED

template <typename...Ts>
inline void warning( const Ts&... msg )
{
    join( _nanos_ostream, "WARNING: [", getMyThreadId(), "] ", msg... );
}

template <typename...Ts>
inline void message( const Ts&... msg )
{
    join( _nanos_ostream, "MSG: [", getMyThreadId(), "] ", msg... );
}

template <typename T>
inline void messageMaster( T msg )
{
   do {
      if ( getNodeNumber() == 0 ) {
         join( _nanos_ostream, "MSG: m:[", getMyThreadId(), "] ", msg );
      }
   } while (0);
}

} // namespace nanos

#endif // DEBUG_HPP

