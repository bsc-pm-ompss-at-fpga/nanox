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

#ifndef MEMORY_CHUNK_HPP
#define MEMORY_CHUNK_HPP

#include "memoryaddress.hpp"

namespace nanos {
namespace memory {

template < typename ChunkType >
struct is_contiguous_memory_region : public std::false_type
{
};

/*!
 * \brief Represents a contiguous area of memory.
 * \author Jorge Bellon
 */
class MemoryChunk {
   private:
      Address _begin; //!< Beginning address of the chunk
      Address _end;   //!< Size of the chunk
   public:
		MemoryChunk() = delete;

		constexpr
		MemoryChunk( MemoryChunk const& other ) = default;

		/*! \brief Creates a new representation of an area of memory.
		 * @param[in] base beginning address of the region.
		 * @param[in] length size of the region.
		 */
      constexpr
      MemoryChunk( Address const& base, size_t length ) :
            _begin( base ), _end( base+length )
      {
      }

		/*! \brief Creates a new representation of an area of memory.
		 * @param[in] begin lower limit of the region. Inclusive.
		 * @param[in] end upper limit of the region. Exclusive.
		 */
      constexpr
      MemoryChunk( Address const& beginAddress, Address const& endAddress ) :
            _begin( beginAddress ), _end( endAddress )
      {
      }

		//! \returns the size of the region.
      constexpr
      size_t size()
		{
			return _end - _begin;
		}

		//! \returns the lower limit address of the region.
      constexpr
      Address begin()
		{
			return _begin;
		}

		//! \returns the upper limit address of the region.
      constexpr
      Address end()
		{
			return _end;
		}

		//! \returns whether an address belongs to the region or not.
		constexpr
		bool contains( Address address )
		{
			return begin() <= address && address < end();
		}

		constexpr
		bool operator< ( MemoryChunk const& chunk )
		{
			return end() < chunk.begin();
		}
};

/*!
 * \brief Represents a contiguous area of aligned memory.
 * \tparam alignment_restriction alignment of the region that must be satisfied.
 *
 * \author Jorge Bellon
 */
template <size_t alignment_restriction>
class AlignedMemoryChunk : public MemoryChunk {
   public:
		AlignedMemoryChunk() = delete;

		constexpr
		AlignedMemoryChunk( AlignedMemoryChunk const& other ) = default;

      constexpr
      AlignedMemoryChunk( Address const& baseAddress, size_t chunkSize ) :
            MemoryChunk( baseAddress, chunkSize )
      {
      }

      constexpr
      AlignedMemoryChunk( Address const& baseAddress, Address const& endAddress ) :
            MemoryChunk( baseAddress, endAddress )
      {
      }

		/*!
		 * \brief Constructs an AlignedMemoryChunk that wraps
		 * any other kind of contiguous memory chunk.
		 */
      template<class ChunkType>
      constexpr
      AlignedMemoryChunk( ChunkType const& chunk ) :
            MemoryChunk(
                     chunk.begin().template align<alignment_restriction>(),
                     chunk.end().template align<alignment_restriction>()
								+ (chunk.end().template isAligned<alignment_restriction>()?
									0 : 
									alignment_restriction)
                  )
      {
			static_assert( is_contiguous_memory_region<ChunkType>::value,
					"Provided chunk does not represent contiguous memory." );
      }
};

template <>
struct is_contiguous_memory_region< MemoryChunk > : public std::true_type
{
};

template <size_t alignment>
struct is_contiguous_memory_region< AlignedMemoryChunk<alignment> > : public std::true_type
{
};

} // namespace memory
} // namespace nanos

#endif // MEMORY_CHUNK

