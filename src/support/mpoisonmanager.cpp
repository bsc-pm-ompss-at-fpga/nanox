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

const uint64_t page_size = sysconf(_SC_PAGESIZE);
const uint64_t pn_mask = ~(page_size - 1);


MPoisonManager::MPoisonManager( int seed, size_t size, float rate ):
   mgr_lock(),
   alloc_list(),
   blocked_pages(),
   total_size(0),
   generator(seed),
   page_fault_dist(0, size),
   wait_time_dist(rate)
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

void MPoisonManager::resetRndPageDist()
{
   page_fault_dist = std::uniform_int_distribution<size_t>(0, total_size);
   page_fault_dist.reset();
}

uintptr_t MPoisonManager::getRandomPage(){

   size_t pos = page_fault_dist(generator);

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

unsigned MPoisonManager::getWaitTime( )
{
   // Note: wait times are in us
   unsigned t = unsigned( wait_time_dist(generator) * 1000000 );
   return t;
}

int MPoisonManager::blockPage() {
  uintptr_t addr = getRandomPage();
  if( addr ) {
     return injectFault(addr);
     // FZ: test fault injection with bitflip.
     //return blockSpecificPage( addr );
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

int MPoisonManager::injectFault( uintptr_t page_addr ) {
   unsigned char* page_head = (unsigned char*)page_addr;
   for (int i = 0; i < (int)page_size; i++) {
      *page_head = 10;
      page_head++;
   }

   // TODO: set isFault to true.
   return 0;
}

int MPoisonManager::unblockPage( uintptr_t page_addr ) {
  LockBlock lock( mgr_lock );
  if( blocked_pages.erase( page_addr ) > 0 ) {
      return mprotect( (void*)page_addr, page_size, PROT_READ | PROT_WRITE );
  }
  return 0;// Page didn't exist. Don't report error cause another thread could have unblocked it.
}

}//namespace vm
}//namespace nanos
