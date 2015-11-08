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
#ifndef MPOISON_MANAGER_HPP
#define MPOISON_MANAGER_HPP

#include <deque>
#include <set>
#include <random>
#include <cstdint>
#include <unistd.h>

#include "atomic.hpp"

namespace nanos {
namespace vm {

extern const uint64_t page_size;
extern const uint64_t pn_mask;

class MPoisonManager {

public:
  MPoisonManager( int seed, size_t size, float rate );

  virtual ~MPoisonManager();

  //!< Registers a portion of memory susceptible to be corrupted in the manager.
  void addAllocation( uintptr_t addr, size_t size );

  /*! Unregisters a portion of memory susceptible to be corrupted in the manager.
   * In case there were blocked pages inside it, they are unblocked.
   */
  void deleteAllocation( uintptr_t addr );

  //!< Unblocks all pages that remain blocked and removes them from the manager.
  void clearAllocations ( );

  //!< Returns randomly an arrival time for the next failure.
  unsigned getWaitTime( );

  //!< Returns a randomly selected page's base address.
  uintptr_t getRandomPage( );

  //!< Resets the page fault distribution so that it fits total_size.
  void resetRndPageDist( );

  /*! \brief Removes a randomly selected page's access rights.
   * \return 0 on success. A different value means that the page could not be blocked.
   */
  int blockPage( );

  //!< Removes a given page's access rights. Address must be aligned to 'page_size'.
  int blockSpecificPage( uintptr_t page_addr );

  //!< Injects a fault in the specified page.
  int injectFault( uintptr_t page_addr);

  /*! Returns an specific page's access rights to its original value. 
   * Page address must be aligned to 'page_size'.
   */
  int unblockPage( uintptr_t page_addr );

private: 
  typedef struct {
    uintptr_t addr;
    std::size_t size;
  } alloc_t;//!< Contains information about an allocation: base address and size.

  Lock mgr_lock; //!< Provides mutual exclusion access to unblockPage function.

  std::deque<alloc_t> alloc_list; //!< List that contains all the allocations made by the user.
  std::set<uintptr_t> blocked_pages; //!< Set of addresses that which address has been unauthorised randomly.
  size_t total_size;//!< Indicates the total amount of memory allocated by the user.

  std::mt19937 generator;//!< Random number generator engine
  std::uniform_int_distribution<size_t> page_fault_dist;//!< Random distribution used for memory page fault selection
  std::exponential_distribution<float> wait_time_dist;//!< Random distribution used for time between failures
};

}// namespace vm
}// namespace nanos

#endif // MPOISON_MANAGER_HPP
