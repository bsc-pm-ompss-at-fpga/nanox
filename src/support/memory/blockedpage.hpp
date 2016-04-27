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

#ifndef BLOCKED_PAGE_HPP
#define BLOCKED_PAGE_HPP

#include "memorypage.hpp"

#include <sys/mman.h>

namespace nanos {
namespace memory {

/*! \brief Object representation of a memory portion without any access rights.
 *  \details BlockedMemoryPage represents an area of memory whose access rights
 *  have been taken away.
 *  Is strictly necessary that this area is aligned to a memory page (both beginning
 *  and ending) to avoid undesired behavior.
 *
 *  \inmodule ErrorInjection
 *  \author Jorge Bellon
 */
class BlockedMemoryPage : public MemoryPage {
	public:
		/*! \brief Creates a new blocked memory area that covers a virtual memory page
		 * \details It blocks its access rights using mprotect. The underlying memory
		 * page object is created by copy.
		 * @param[in] page the page that is going to be blocked.
		 */
		BlockedMemoryPage( MemoryPage const& page ) :
			MemoryPage( page )
		{
			// Blocks this area of memory from reading
			mprotect( begin(), size(), PROT_NONE );
		}

		/*! \brief Creates a new blocked memory area that covers a virtual memory page
		 * \details It blocks its access rights using mprotect. The underlying memory
		 * page object is created using one of its constructors.
		 * @param[in] page the page that is going to be blocked.
		 */
		template< typename... Args >
		BlockedMemoryPage( Args const&... arguments ) :
			MemoryPage( arguments... )
		{
			// Blocks this area of memory from reading
			mprotect( begin(), size(), PROT_NONE );
		}

		/*! \brief Creates a new blocked memory area that covers a virtual memory page
		 * \details It blocks its access rights using mprotect. The underlying memory
		 * page object is created using one of its constructors.
		 * @param[in] page the page that is going to be blocked.
		 */
		template< typename... Args >
		BlockedMemoryPage( Args&&... arguments ) :
			MemoryPage( std::forward<Args>(arguments)... )
		{
			// Blocks this area of memory from reading
			mprotect( begin(), size(), PROT_NONE );
		}

		/*! \brief Releases the resources allocated by this object.
		 * \details Releases the resources used by this object. It also
		 * 	restores access rights to its previous value.
		 * 	Regarding the previous value, we assume that original
		 * 	access rights were read/write.
		 */
		virtual ~BlockedMemoryPage()
		{
			// Restores access rights for this area of memory
			mprotect( begin(), size(), PROT_READ | PROT_WRITE );
		}
};

} // namespace memory
} // namespace nanos

#endif // BLOCKED_PAGE_HPP

