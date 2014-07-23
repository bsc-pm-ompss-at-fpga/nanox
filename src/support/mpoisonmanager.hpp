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

namespace nanos {
namespace vm {

extern uint64_t page_size;
extern uint64_t pn_mask;

class MPoisonManager {

public:
  MPoisonManager( int seed );

  virtual ~MPoisonManager();

  void addAllocation( uintptr_t addr, size_t size );

  void deleteAllocation( uintptr_t addr );

  void clearAllocations ( );

  uintptr_t getRandomPage( );

  int blockPage( );

  int unblockPage( uintptr_t page_addr );

private: 
  typedef struct {
    uintptr_t addr;
    std::size_t size;
  } alloc_t;

  std::deque<alloc_t> alloc_list; //!< List that contains all the allocations made by the user.
  std::set<uintptr_t> blocked_pages; //!< Set of addresses that which address has been unauthorised randomly.
  size_t total_size;//!< Indicates the total amount of memory allocated by the user.

  std::mt19937 generator;//!< RNG engine
};

}// namespace vm
}// namespace nanos

#endif // MPOISON_MANAGER_HPP
