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

#ifndef SIGNAL_INFO_HPP
#define SIGNAL_INFO_HPP

#include <ucontext.h>

namespace nanos {
namespace error {

class ExecutionContext {
	private:
		ucontext_t savedContext;
	public:
		ExecutionContext( ucontext_t * context ) :
			savedContext( *context)
		{}

		/* TODO: finish ucontext encapsulation */
		ucontext_t &get() { return savedContext; }
};

class SignalInfo {
	private:
		siginfo_t savedInfo;
	public:
		SignalInfo( siginfo_t * info ) :
			savedInfo( *info )
		{}

};

} // namespace error
} // namespace nanos

#endif // SIGNAL_INFO_HPP
