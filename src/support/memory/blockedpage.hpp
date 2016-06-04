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

struct block_page_at_creation_t {};
struct block_page_manually_t {};

block_page_at_creation_t block_page_at_creation;
block_page_manually_t block_page_manually;

/*! \brief Object representation of a memory portion without any access rights.
 *  \details BlockableAccessMemoryPage represents an area of memory whose access rights
 *  have been taken away.
 *  Is strictly necessary that this area is aligned to a memory page (both beginning
 *  and ending) to avoid undesired behavior.
 *
 *  \inmodule ErrorInjection
 *  \author Jorge Bellon
 */
class BlockableAccessMemoryPage : public MemoryPage {
	private:
		bool _blocked;
	public:
		/*! \brief Creates a new blocked memory area that covers a virtual memory page
		 * \details It blocks its access rights using mprotect. The underlying memory
		 * page object is created by copy.
		 * @param[in] page the page that is going to be blocked.
		 */
		BlockableAccessMemoryPage( const MemoryPage& page, block_page_at_creation_t ) :
			MemoryPage( page ),
			_blocked(false)
		{
			blockAccess();
		}

		BlockableAccessMemoryPage( const MemoryPage& page, block_page_manually_t ) :
			MemoryPage( page ),
			_blocked(false)
		{
		}

		BlockableAccessMemoryPage( const MemoryPage& page ) :
			BlockableAccessMemoryPage( page, block_page_at_creation )
		{
		}

		/*! \brief Creates a new blocked memory area that covers a virtual memory page
		 * \details It blocks its access rights using mprotect. The underlying memory
		 * page object is created using one of its constructors.
		 * @param[in] page the page that is going to be blocked.
		 */
		BlockableAccessMemoryPage( const Address& address, block_page_at_creation_t ) :
			MemoryPage( address ),
			_blocked(false)
		{
			blockAccess();
		}

		BlockableAccessMemoryPage( const Address& address, block_page_manually_t ) :
			MemoryPage( address ),
			_blocked(false)
		{
		}

		BlockableAccessMemoryPage( const Address& address ) :
			BlockableAccessMemoryPage( address, block_page_at_creation )
		{
		}

		/*! \brief Copies a blocked virtual memory page (deleted).
		 */
		BlockableAccessMemoryPage( const BlockableAccessMemoryPage& page ) = delete;

		BlockableAccessMemoryPage( BlockableAccessMemoryPage&& page ) :
			MemoryPage( page ),
			_blocked(page._blocked)
		{
			page._blocked = false;
		}

		/*! \brief Releases the resources allocated by this object.
		 * \details Releases the resources used by this object. It also
		 * 	restores access rights to its previous value.
		 * 	Regarding the previous value, we assume that original
		 * 	access rights were read/write.
		 */
		virtual ~BlockableAccessMemoryPage()
		{
			// Restores access rights for this area of memory
			if( _blocked )
				unblockAccess();
		}

		void blockAccess()
		{
			_blocked = true;
			protect( PROT_NONE);
		}

		void unblockAccess()
		{
			protect( PROT_READ|PROT_WRITE );
			_blocked = false;
		}

		bool isBlocked() const
		{
			return _blocked;
		}
};

} // namespace memory
} // namespace nanos

#endif // BLOCKED_PAGE_HPP

