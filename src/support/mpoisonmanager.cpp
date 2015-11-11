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
const unsigned char word_size = sizeof(uintptr_t);
const uintptr_t addr_mask = ~(sizeof(uintptr_t) - 1);


MPoisonManager::MPoisonManager( int seed, size_t size, float rate ):
   mgr_lock(),
   alloc_list(),
   blocked_pages(),
   interested_mem_allocations(10),
   total_size(0),
   total_interested_memory_allocation_size(0),
   generator(seed),
   page_fault_dist(0, size),
   wait_time_dist(rate),
   addr_in_page_dist(0, (unsigned short)page_size - 1),
   bit_in_addr_dist(0, word_size - 1),
   bit_value_dist(0, 1)
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
   sys.getExceptionStats().setTotalMemoryExposedToFaultInjection(total_size);
}

void MPoisonManager::addInterestedMemoryAllocation( uintptr_t addr, size_t size )
{
   interested_mem_allocations.push_back({ addr, size });
   total_interested_memory_allocation_size += size;
   sys.getExceptionStats().setSizeOfMemoryInterestedInFaultInjection(total_interested_memory_allocation_size);
}

bool MPoisonManager::isInInterestedMemoryAllocation( uintptr_t addr )
{
   std::vector<alloc_t>::iterator it = interested_mem_allocations.begin();
   for (it = interested_mem_allocations.begin(); it != interested_mem_allocations.end(); it++ )
   {
      uintptr_t start_addr = it->addr;
      uintptr_t end_addr = start_addr + it->size;
      if (addr >= start_addr && addr <= end_addr) {
         return true;
      }
   }
   return false;
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

unsigned short MPoisonManager::getRandomAddressInPage() {
   return addr_in_page_dist(generator);
}

unsigned char MPoisonManager::getRandomBitIndex() {
   return bit_in_addr_dist(generator);
}

unsigned char MPoisonManager::getRandomBitValue() {
   return bit_value_dist(generator);
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
     return injectBitFlipInPage(addr);
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

int MPoisonManager::injectBitFlipInPage( uintptr_t page_addr ) {
   unsigned short addr_offset = getRandomAddressInPage();
   uintptr_t addr = page_addr + addr_offset;

   return injectBitFlipInAddress(addr);
}

int MPoisonManager::injectBitFlipInAddress( uintptr_t addr ) {
   unsigned char bit_index = getRandomBitIndex();
   unsigned char bit_value = getRandomBitValue();

   // Align the address
   addr = addr - (addr % word_size);

   uintptr_t *addr_ptr = (uintptr_t*)addr;

   uintptr_t most_significant_bits_of_addr = (addr_mask << (bit_index + 1)) & *addr_ptr;
   uintptr_t faulty_bit = bit_value << bit_index;
   uintptr_t less_significant_bits_of_addr = (addr_mask >> (word_size - bit_index)) & *addr_ptr;

   // The new value at the address.
   *addr_ptr = most_significant_bits_of_addr | faulty_bit | less_significant_bits_of_addr;

   if (isInInterestedMemoryAllocation(addr)) {
      sys.getExceptionStats().incrFaultsInInterestedMemoryRegion();
   }
   sys.setFaultyAddress(addr);

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
