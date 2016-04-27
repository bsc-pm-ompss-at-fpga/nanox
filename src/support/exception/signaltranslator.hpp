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

#ifndef SIGNAL_TRANSLATOR_HPP
#define SIGNAL_TRANSLATOR_HPP

#include "error.hpp"

#include <signal.h>

// Source code based in the following article:
// http://www.ibm.com/developerworks/library/l-cppexcep/

namespace nanos {
namespace error {

template < class SignalException >
class SignalTranslator {
	private:
		class SingletonTranslator {
			private:
				struct sigaction _recoveryAction;
			public:
				SingletonTranslator() {
					// Set up the structure to specify task-recovery.
					_recoveryAction.sa_sigaction = &signalHandler;
					sigemptyset(&_recoveryAction.sa_mask);
					// SA_SIGINFO: Provides context information to the handler.
					// SA_RESTART: Resume system calls interrupted by the signal.
					_recoveryAction.sa_flags = SA_SIGINFO | SA_RESTART;
				}

				void registerHandler() {
					int err =
						sigaction(
							SignalException::getSignalNumber(),
							&_recoveryAction,
							NULL);
					fatal_cond( err != 0, "Signal handling setup failed");
				}

				static void signalHandler( int signalNumber, siginfo_t* signalInfo, void* executionContext ) {
					throw SignalException( signalInfo, (ucontext_t*) executionContext );
				}
		};

	public:
		SignalTranslator()
		{
			static SingletonTranslator s_objTranslator;
			s_objTranslator.registerHandler();
		}
};

} // namespace error
} // namespace nanos

#endif // SIGNAL_TRANSLATOR_HPP
