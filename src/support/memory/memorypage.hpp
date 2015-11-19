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

#ifndef MEMORY_PAGE_HPP
#define MEMORY_PAGE_HPP

#include "memorychunk.hpp"

#include <deque>
#include <sys/user.h>

/*!
 * \brief Represents a virtual memory page.
 *
 * \author Jorge Bellon
 */
class MemoryPage : public AlignedMemoryChunk<PAGE_SIZE> {
   public:
		constexpr
		static size_t size() { return PAGE_SIZE; }

		/*! Creates a region that represents the virtual memory
		 * page that contains a given address.
		 * \param[in] address the address that belongs to the page.
		 */
      constexpr
      MemoryPage( Address const& address ) :
            AlignedMemoryChunk( address.align<PAGE_SIZE>(), MemoryPage::size() )
      {
      }

      template<class ChunkType>
      static std::deque<MemoryPage>&& retrievePagesFromChunk( ChunkType const& chunk )
      {
			// Create a page-aligned chunk that wraps the given one
			AlignedMemoryChunk<PAGE_SIZE> wrap( chunk );
			// Construct a list of as many memory pages as needed to
			// represent this newly created chunk.
			std::deque<MemoryPage> pageScatteredChunk( 
					wrap.size() / MemoryPage::size(), // number of elements
					MemoryPage( wrap.begin() )        // default value
				);

			Address addr = wrap.begin();
			std::for_each( pageScatteredChunk.begin(), 
								pageScatteredChunk.end(),
								[&addr](MemoryPage& page) { page.setBaseAddress( addr+=MemoryPage::size() ); }
							);

			return pageScatteredChunk;
      }
};

template<>
struct is_contiguous_memory_region<MemoryPage> : public std::true_type
{
};

#endif // MEMORY_PAGE_HPP
