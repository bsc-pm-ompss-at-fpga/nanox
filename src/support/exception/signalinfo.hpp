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

#include "memory/memoryaddress.hpp"
#include "memory/memorychunk.hpp"

#include <signal.h>
#include <ucontext.h>

namespace nanos {
namespace error {

// siginfo_t {
//     int      si_signo;     /* Signal number */
//     int      si_errno;     /* An errno value */
//     int      si_code;      /* Signal code */
//     int      si_trapno;    /* Trap number that caused
//                               hardware-generated signal
//                               (unused on most architectures) */
//     pid_t    si_pid;       /* Sending process ID */
//     uid_t    si_uid;       /* Real user ID of sending process */
//     int      si_status;    /* Exit value or signal */
//     clock_t  si_utime;     /* User time consumed */
//     clock_t  si_stime;     /* System time consumed */
//     sigval_t si_value;     /* Signal value */
//     int      si_int;       /* POSIX.1b signal */
//     void    *si_ptr;       /* POSIX.1b signal */
//     int      si_overrun;   /* Timer overrun count;
//                               POSIX.1b timers */
//     int      si_timerid;   /* Timer ID; POSIX.1b timers */
//     void    *si_addr;      /* Memory location which caused fault */
//     long     si_band;      /* Band event (was int in
//                               glibc 2.3.2 and earlier) */
//     int      si_fd;        /* File descriptor */
//     short    si_addr_lsb;  /* Least significant bit of address
//                               (since Linux 2.6.32) */
//     void    *si_call_addr; /* Address of system call instruction
//                               (since Linux 3.5) */
//     int      si_syscall;   /* Number of attempted system call
//                               (since Linux 3.5) */
//     unsigned int si_arch;  /* Architecture of attempted system call
//                               (since Linux 3.5) */
// }


class ExecutionContext {
	private:
		ucontext_t _savedContext;
	public:
		ExecutionContext( ucontext_t const& context ) :
			_savedContext( context)
		{}

		/* TODO: finish ucontext encapsulation */
		ucontext_t &get() { return _savedContext; }
};

/* \brief C++ wrapper for siginfo_t struct.
 * \details The purpose of this class is to provide a safe
 * way to access siginfo_t elements.
 * Since it is a struct mainly composed by union of fields,
 * not all accesses are valid, depending on the type of signal
 * that is referenced.
 *
 * Currently, there is no mechanism to check that a right
 * use of the siginfo_t is being done.
 *
 * If signals other than SIGBUS and SIGSEGV have to be supported,
 * it could be a better idea to create a siginfo specialized for each
 * of those signals, since siginfo_t struct is actually composed by
 * several unions whose elements' values are not valid depending
 * on the signal number and/or code.
 */
class SignalInfo {
	private:
		siginfo_t _info;
	public:
		SignalInfo( siginfo_t const& info ) :
			_info( info )
		{}

		int getSignalNumber() const { return _info.si_signo; }

		int getErrorNumber() const { return _info.si_errno; }

		int getSignalCode() const { return _info.si_code; }

		memory::Address getAddress() const { return memory::Address( _info.si_addr ); }

		/*! \brief Gets the portion of memory affected by an error
		 * \details This does only applies for SIGBUS signals with 
		 * BUS_MCEERR_AO or BUS_MCEERR_AR codes (ECC check error).
		 *
		 * Uses the value of the less significant bit to create
		 * a memory region with that size.
		 * For example: if the affected portion is a whole page,
		 * the value of lsb would be log2(sysconf(SC_PAGESIZE)).
		 * \see sigaction(2).
		 */
		memory::MemoryChunk getAffectedMemoryLocation() const
		{
			return memory::MemoryChunk( getAddress(),
#ifdef si_addr_lsb
						1 << (_info.si_addr_lsb)
#else
						1 << 12 // 4K, a memory page
#endif
					);
		}
};

} // namespace error
} // namespace nanos

#endif // SIGNAL_INFO_HPP
