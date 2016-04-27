/*************************************************************************************/
/*      Copyright 2009-2015 Barcelona Supercomputing Center                          */
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

#ifndef EXCEPTION_TRACER_HPP
#define EXCEPTION_TRACER_HPP

#include "debug.hpp"

#include <execinfo.h>
#include <stdlib.h>

namespace nanos {
namespace error {

/*!
 * \ingroup NanosExceptions
 * \brief Prints a backtrace
 * \details ExceptionTracer prints a small backtrace when an exception is created.
 * Based on [http://www.ibm.com/developerworks/library/l-cppexcep]
 */
class ExceptionTracer {
	public:
		ExceptionTracer()
		{
#ifdef NANOS_DEBUG_ENABLED
			void *array[25];
			int length = backtrace(array,25);
			char** symbols = backtrace_symbols(array,length);
			for( int i = 0; i < length; i++) {
				debug(symbols[i]);
			}
			free(symbols);
#endif
		}
};

}// namespace error
}// namespace nanos

#endif // EXCEPTION_TRACER_HPP
