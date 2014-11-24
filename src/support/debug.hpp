/*************************************************************************************/
/*      Copyright 2014 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_LIB_DEBUG
#define _NANOS_LIB_DEBUG

#include <stdexcept>
#include <iostream>
#include <sstream>
//Having system.hpp here generate too many circular dependences
//but it's not really needed so we can delay it most times until the actual usage
#include "system_decl.hpp"

#define _nanos_ostream ( /* myThread ? *(myThread->_file) : */ std::cerr )

template <typename OStreamType>
static inline OStreamType &join( OStreamType &os )
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

namespace nanos
{

   class FatalError : public  std::runtime_error
   {

      public:
         FatalError ( const std::string &value, int peId=-1 ) :
            runtime_error( join( std::stringstream("FATAL ERROR: ["), peId, "] ", value).str() )
         {
         }

   };

   class FailedAssertion : public  std::runtime_error
   {

      public:
         FailedAssertion ( const char *file, const int line, const std::string &value,
                           const std::string msg, int peId=-1 ) :
            runtime_error( join( std::stringstream("ASSERT failed: ["), peId, "] ", value, ": ", msg, " (", file, ":", line, " )" ).str() )
         {
         }

   };

template <typename...Ts>
inline void fatal( const Ts&... msg )
{
   std::stringstream sts;
   join( sts, msg... );
   throw nanos::FatalError( sts.str(), getMyThreadSafe()->getId() );
}

template <typename...Ts>
inline void fatal0( const Ts&... msg )
{
   std::stringstream sts;
   join( sts, msg... );
   throw nanos::FatalError( sts.str() );
}

template <typename...Ts>
inline void fatal_cond( bool cond, const Ts&... msg )
{
   if( cond )
      fatal( msg... );
}

template <typename...Ts>
inline void fatal_cond0( bool cond, const Ts&... msg )
{
   if( cond )
      fatal0( msg... );
}

template <typename...Ts>
inline void warning( const Ts&... msg )
{
    join( _nanos_ostream, "WARNING: [", std::dec, getMyThreadSafe()->getId(), "] ", msg... );
}

template <typename...Ts>
inline void warning0( const Ts&... msg )
{
   join( _nanos_ostream, "WARNING: [?] ", msg... );
}


template <typename...Ts>
inline void message( const Ts&... msg )
{
    join( _nanos_ostream, "MSG: [", std::dec, getMyThreadSafe()->getId(), "] ", msg... );
}

template <typename...Ts>
inline void message0( const Ts&... msg)
{
    join( _nanos_ostream, "MSG: [?] ", msg... );
}

template <typename T>
inline void messageMaster( T msg )
{
   do {
      if (sys.getNetwork()->getNodeNum() == 0) {
         join( _nanos_ostream, "MSG: m:[", std::dec, getMyThreadSafe()->getId(), "] ", msg );
      }
   } while (0);
}

template <typename T>
inline void message0Master( T msg )
{
   join( _nanos_ostream, "MSG: m:[?] ", msg );
}

#ifdef NANOS_DEBUG_ENABLED

template <typename...Ts>
inline void ensure( bool cond, const Ts&... msg )
{
   if( !cond ) {
      std::stringstream sts;
      join( sts, msg... );
      throw nanos::FatalError( sts.str(), getMyThreadSafe()->getId() );
   }
}

template <typename...Ts>
inline void ensure0( bool cond, const Ts&... msg )
{
   if( !cond ) {
      std::stringstream sts;
      join( sts, msg... );
      throw nanos::FatalError( sts.str() );
   }
}

template <typename...Ts>
inline void verbose( const Ts&... msg )
{
   if( sys.getVerbose() ) {
      join( _nanos_ostream, "[", std::dec, getMyThreadSafe()->getId(), "] ", msg... );
   }
}

template <typename...Ts>
inline void verbose0( const Ts&... msg)
{
   if( sys.getVerbose() ) {
      join( _nanos_ostream, "[?] ", msg... );
   }
}

template <typename...Ts>
inline void debug( const Ts&... msg )
{
   if( sys.getVerbose() ) {
      join( _nanos_ostream, "DBG [", std::dec, getMyThreadSafe()->getId(), "] ", msg... );
   }
}

template <typename...Ts>
inline void debug0( const Ts&... msg)
{
   if( sys.getVerbose() ) {
      join( _nanos_ostream, "DBG [?] ", msg... );
   }
}

#else

template <typename...Ts>
inline void ensure( bool cond, const Ts&... msg ) {}

template <typename...Ts>
inline void ensure0( bool cond, const Ts&... msg ) {}

template <typename...Ts>
inline void verbose( const Ts&... msg ) {}

template <typename...Ts>
inline void verbose0( const Ts&... msg) {}

template <typename...Ts>
inline void debug( const Ts&... msg ) {}

template <typename...Ts>
inline void debug0( const Ts&... msg) {}

#endif // NANOS_DEBUG_ENABLED

};

#endif // _NANOS_LIB_DEBUG
