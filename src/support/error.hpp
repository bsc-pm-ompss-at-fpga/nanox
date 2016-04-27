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

#ifndef ERROR_DECL_HPP
#define ERROR_DECL_HPP

#include "xstring.hpp"

#include "debug.hpp"
#include "exception/exceptiontracer.hpp"

#include <stdexcept>
#include <sstream>

namespace nanos {
namespace error {

struct FatalError : public ExceptionTracer, public std::runtime_error {
   FatalError ( const std::string &value ) :
      runtime_error( join( std::stringstream(), "FATAL ERROR: [", getMyThreadId(), "] ", value).str() )
   {
   }
};

struct FailedAssertion : public ExceptionTracer, public std::runtime_error {
   FailedAssertion ( const std::string &value, const std::string msg ) :
      runtime_error( join( std::stringstream(), "ASSERT failed: [", getMyThreadId(), "] ", value, ": ", msg ).str() )
   {
   }
};

} // namespace error

template <typename...Ts>
void fatal( const Ts&... message ) __attribute__ ((noreturn));

template <typename...Ts>
inline void fatal( const Ts&... message )
{
   std::stringstream sts;
   join( sts, message... );
   throw error::FatalError( sts.str() );
}

template <typename...Ts>
inline void fatal_cond( bool cond, const Ts&... msg )
{
   if( cond )
      fatal( msg... );
}

#ifdef NANOS_DEBUG_ENABLED
#define ensure(condition,message) nanos::_ensure( condition, #condition, message, "@", __FILE__, ":", __LINE__ )

template <typename...Ts>
inline void _ensure( bool condition, const char* conditionDefinition, const Ts&... msg )
{
   if( !condition ) {
      std::stringstream sts;
      join( sts, msg... );
      throw error::FailedAssertion( conditionDefinition, sts.str() );
   }
}
#else
#define ensure(condition,message)
#endif

} // namespace nanos

#endif // ERROR_DECL_HPP

