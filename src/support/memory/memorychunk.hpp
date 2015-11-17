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

/*!
 * \brief Represents a contiguous area of memory.
 * \author Jorge Bellon
 */
class MemoryChunk {
   private:
      Address _baseAddress; //!< Beginning address of the chunk
      size_t  _length;       //!< Size of the chunk
   public:
		/*! \brief Creates a new representation of an area of memory.
		 * @param[in] base beginning address of the region.
		 * @param[in] chunkLength size of the region.
		 */
      constexpr
      MemoryChunk( Address const& base, size_t chunkLength ) :
            _baseAddress( base ), _length( chunkLength )
      {
      }

		/*! \brief Creates a new representation of an area of memory.
		 * @param[in] begin lower limit of the region.
		 * @param[in] end upper limit of the region.
		 */
      constexpr
      MemoryChunk( Address const& begin, Address const& end ) :
            _baseAddress( begin ), _length( end - begin )
      {
      }

		//! \returns the lower limit address of the region.
      constexpr
      Address getBaseAddress() const { return _baseAddress; }

		//! \returns the size of the region.
      constexpr
      size_t size() const { return _length; }

		//! \returns the lower limit address of the region.
      constexpr
      Address begin() const { return _baseAddress; }

		//! \returns the upper limit address of the region.
      constexpr
      Address end() const { return _baseAddress+_length; }
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

      template<class ChunkType>
      constexpr
      AlignedMemoryChunk( ChunkType const& chunk ) :
            MemoryChunk(
                     chunk.begin().align( alignment_restriction ) + alignment_restriction,
                     chunk.end().align( alignment_restriction )
                  )
      {
      }
};

// FIXME: change literal page size by macro computed by autoconf
using MemoryPage = AlignedMemoryChunk<4096>;

#endif // MEMORY_CHUNK
