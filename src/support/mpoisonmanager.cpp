/*************************************************************************************/
/*      Copyright 2014 Barcelona Supercomputing Center                               */
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

#include "mpoisonmanager.hpp"
#include "debug.hpp"
#include "system.hpp"
#include <sys/mman.h>

namespace nanos {
namespace vm {

uint64_t page_size = sysconf(_SC_PAGESIZE);
uint64_t pn_mask = ~(page_size - 1);


MPoisonManager::MPoisonManager( int seed ):
  mgr_lock(),
  alloc_list(),
  blocked_pages(),
  total_size(0),
  generator(seed)
{
}


MPoisonManager::~MPoisonManager()
{
   LockBlock lock( mgr_lock );
   clearAllocations();
}

void MPoisonManager::addAllocation( uintptr_t addr, size_t size )
{
   alloc_list.push_back( { addr, size } );
   total_size += size;
}

void MPoisonManager::deleteAllocation( uintptr_t addr )
{
   // search for the affected allocation
   std::deque<alloc_t>::iterator it = alloc_list.begin();
   while( it->addr != addr && it != alloc_list.end() ) {
      it++;
   }

   if( it!= alloc_list.end() ){
      size_t size = it->size;
      total_size -= it->size;
      alloc_list.erase(it);

      // Look for any page in this memory portion that was previously blocked and unblock it
      for(unsigned offset=0; offset < size; offset+=page_size)
      {
         uintptr_t page_addr = (addr + offset) & pn_mask;
         if( blocked_pages.erase(page_addr) > 0 ) {// if a page was found, unblock it (it's blocked)
            if( mprotect( (void*) page_addr, page_size, PROT_READ | PROT_WRITE ) < 0 )
               fatal0( "Error while unblocking page ", std::hex, page_addr,
                       strerror(errno)
                     );
            break;// Once we found it, finish.
         }
      }
   }
}


void MPoisonManager::clearAllocations( )
{
   total_size = 0;
   alloc_list.clear();

   std::set<uintptr_t>::iterator it;
   for( it = blocked_pages.begin(); it != blocked_pages.end(); it++ ) {
      uintptr_t addr = *it;
      if( mprotect( (void*)addr, page_size, PROT_READ | PROT_WRITE ) < 0)
         fatal0( "Error while unpoisoning: ", strerror(errno) );
   }
   blocked_pages.clear();
}

uintptr_t MPoisonManager::getRandomPage(){

  std::uniform_int_distribution<size_t> distribution(0, total_size);
  size_t pos = distribution(generator);

  std::deque<alloc_t>::iterator it;
  for( it = alloc_list.begin(); it != alloc_list.end() && pos >= it->size; it++ ) {
     pos -= it->size;
  }

  if( total_size > 0 && it != alloc_list.end() )
    return (it->addr + pos) & pn_mask;
  else {
    debug0( "Mpoison: There isn't any page to block." );
    return 0;
  }
}

int MPoisonManager::blockPage() {
  uintptr_t addr = getRandomPage();
  if( addr ) {
     return blockSpecificPage( addr );
  }
  return -1;
}

int MPoisonManager::blockSpecificPage( uintptr_t page_addr ) {
    LockBlock lock ( mgr_lock );
    blocked_pages.insert( page_addr );
    debug0( "Mpoison: blocking memory page. Addr: 0x", std::hex, page_addr, ". "
            "Total: ", std::dec, blocked_pages.size(), " pages blocked."
          );
    return mprotect( (void*)page_addr, page_size, PROT_NONE );
}

int MPoisonManager::unblockPage( uintptr_t page_addr ) {
  LockBlock lock( mgr_lock );
  if( blocked_pages.erase( page_addr ) > 0 ) {
      return mprotect( (void*)page_addr, page_size, PROT_READ | PROT_WRITE );
  }
  return -1;// page didn't exist
}

}//namespace vm
}//namespace nanos
