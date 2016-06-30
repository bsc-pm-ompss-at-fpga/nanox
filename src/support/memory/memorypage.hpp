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

#include "memory/memorychunk.hpp"

#include <vector>

#include <string.h>
#include <sys/mman.h>
#include <sys/user.h>

namespace nanos {
namespace memory {

/*!
 * \brief Represents a virtual memory page.
 * Note: PAGE_SIZE is defined at sys/user.h
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
      //constexpr
      MemoryPage( Address const& address ) :
            AlignedMemoryChunk( address.align<PAGE_SIZE>(), MemoryPage::size() )
      {
      }

      void unmap()
      {
         int err = munmap( begin(), size() );
         fatal_cond( err != 0, "Failed to unmap memory page. Address:", begin(), ". Error: ", strerror(errno) );
      }

      void map()
      {
         memory::Address newAddress( mmap( begin(), size(), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED, -1, 0 ) );
         fatal_cond( newAddress == memory::Address(MAP_FAILED), "Failed to mmap memory page ", begin(), ". Error: ", strerror(errno) );
         fatal_cond( newAddress != begin(), "Failed to map memory page. Its virtual address has changed." );
      }

      void remap()
      {
         // If we mmap again with MAP_FIXED it discards the previously mapped page.
         //unmap();
         map();
      }

      void lock()
      {
         int err = mlock( begin(), size() );
         fatal_cond( err != 0, "Failed to lock memory page. Address:", begin(), ". Error: ", strerror(errno) );
      }

      void unlock()
      {
         int err = munlock( begin(), size() );
         fatal_cond( err != 0, "Failed to unlock memory page. Address:", begin(), ". Error: ", strerror(errno) );
      }

      void protect( int rights = PROT_NONE )
      {
          verbose( "Changing access rights of page ", begin(), " to ", std::hex, rights );
          int err = mprotect( begin(), size(), rights );
          fatal_cond( err != 0, "Failed to protect memory page. Address:", begin(), ". Error: ", strerror(errno) );
      }

      template<class ChunkType>
      static std::vector<MemoryPage> getPagesWrappingChunk( ChunkType const& chunk )
      {
         std::vector<MemoryPage> pages;

         Address begin = chunk.begin().align(MemoryPage::size());
         Address end = chunk.end().align(MemoryPage::size());

         if( end < chunk.end() )
            end += MemoryPage::size();

         // Reserve as many space as necessary
         pages.reserve( (end-begin) / MemoryPage::size() );

         // Fill the container with the appropiate values
         for( Address address = begin; address < end; address += MemoryPage::size() ) {
            pages.emplace_back( address );
         }

         return pages;
      }

      template<class ChunkType>
      static std::vector<MemoryPage> getPagesInsideChunk( ChunkType const& chunk )
      {
         std::vector<MemoryPage> pages;

         Address begin = chunk.begin().align(MemoryPage::size());
         Address end = chunk.end().align(MemoryPage::size());

         if( begin < chunk.begin() )
            begin += MemoryPage::size();
         if( end < begin )
            end = begin;

         // Reserve as many space as necessary
         pages.reserve( (end-begin) / MemoryPage::size() );

         // Fill the container with the appropiate values
         for( Address address = begin; address < end; address += MemoryPage::size() ) {
            pages.emplace_back( address );
         }
         return pages;
      }
};

template<>
struct is_contiguous_memory_region<MemoryPage> : public std::true_type
{
};


} // namespace memory
} // namespace nanos

#endif // MEMORY_PAGE_HPP
