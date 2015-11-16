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

#include "memorychunk.hpp"

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
		/*! \brief Creates a new blocked memory area from two addresses.
		 * \details Shrinks a memory area delimited by two addresses to meet
		 * memory page alignment constraints. Later, it blocks its access
		 * rights using mprotect.
		 * @param[in] beginAddress lower hard limit of the blocked area. Blocked
		 * 	rights may apply to the next page-aligned address if this isn't.
		 * @param[in] endAddress upper hard limit of the blocked area. Blocked
		 * 	rights may apply up to the previous page-aligned address if this isn't.
		 */
		BlockedMemoryPage( Address const& beginAddress, Address const& endAddress ) :
			MemoryPage( beginAddress, endAddress )
		{
			// Blocks this area of memory from reading
			mprotect( getBaseAddress(), getSize(), PROT_NONE );
		}

		/*! \brief Creates a new blocked memory area.
		 * \details Shrinks a memory area delimited by an address and a size to meet
		 * memory page alignment constraints. Later, it blocks its access
		 * rights using mprotect.
		 * @param[in] baseAddress lower hard limit of the blocked area. Blocked
		 * 	rights may apply to the next page-aligned address if this isn't.
		 * @param[in] length maximum size of the blocked area. Blocked rights may
		 * 	apply up to the previous page-aligned address if this isn't.
		 */
		BlockedMemoryPage( Address const& baseAddress, size_t length ) :
			MemoryPage( baseAddress, length )
		{
			// Blocks this area of memory from reading
			mprotect( getBaseAddress(), getSize(), PROT_NONE );
		}

		/*! \brief Releases the resources allocated by this object.
		 * \details Releases the resources used by this object. It also
		 * 	restores access rights to its previous value.
		 * 	Regarding the previous value, we assume that original
		 * 	access rights were read/write.
		 */
		virtual ~BlockedMemoryPage
		{
			// Restores access rights for this area of memory
			mprotect( getBaseAddress(), getSize(), PROT_READ | PROT_WRITE );
		}
};

#endif // BLOCKED_PAGE_HPP
