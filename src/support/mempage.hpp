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

#ifndef MEM_PAGE_HPP_
#define MEM_PAGE_HPP_

#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

/*
 /proc/pid/pagemap.  This file lets a userspace process find out which
 physical frame each virtual page is mapped to.  It contains one 64-bit
 value for each virtual page, containing the following data (from
 fs/proc/task_mmu.c, above pagemap_read):

 * Bits 0-54  page frame number (PFN) if present
 * Bits 0-4   swap type if swapped
 * Bits 5-54  swap offset if swapped
 * Bits 55-60 page shift (page size = 1<<page shift)
 * Bit  61    page is file-page or shared-anon
 * Bit  62    page swapped
 * Bit  63    page present
 *
 If the page is not present but in swap, then the PFN contains an
 encoding of the swap file number and the page's offset into the
 swap. Unmapped pages return a null PFN. This allows determining
 precisely which pages are mapped (or in swap) and comparing mapped
 pages between processes.

 For more info, see: /usr/src/linux-{kernel-version}/tools/vm/page-types.c
 */

// Following macros copied from page-types.c file
#define PM_ENTRY_BYTES      sizeof(uint64_t)
#define PM_STATUS_BITS      3
#define PM_STATUS_OFFSET    (64 - PM_STATUS_BITS)
#define PM_STATUS_MASK      (((1LL << PM_STATUS_BITS) - 1) << PM_STATUS_OFFSET)
#define PM_STATUS(nr)       (((nr) << PM_STATUS_OFFSET) & PM_STATUS_MASK)
#define PM_PSHIFT_BITS      6
#define PM_PSHIFT_OFFSET    (PM_STATUS_OFFSET - PM_PSHIFT_BITS)
#define PM_PSHIFT_MASK      (((1LL << PM_PSHIFT_BITS) - 1) << PM_PSHIFT_OFFSET)
#define PM_PSHIFT(x)        (((uint64_t) (x) << PM_PSHIFT_OFFSET) & PM_PSHIFT_MASK)
#define PM_PFRAME_MASK      ((1LL << PM_PSHIFT_OFFSET) - 1)
#define PM_PFRAME(x)        ((x) & PM_PFRAME_MASK)

#define PM_PRESENT          PM_STATUS(4LL)
#define PM_SWAP             PM_STATUS(2LL)

namespace vm
{
  typedef uint64_t pm_entry_t;
/*!
 * This class is used for storing information contained in /proc/{pid}/pagemap 
 * and /proc/{pid}/maps files
 */
  class MemPage
  {
  private:

    friend std::ostream&
    operator<<(std::ostream&, const MemPage &);

    uint64_t _pn; // Page number (virtual addr)
    uint64_t _pfn; // Page frame number (physical addr)

    //uint64_t _swap_offset;
    //uint16_t _swap_type;

    //uint16_t _shift;

    bool _swapped; // Is this page located in swap or in main memory?
    bool _present; // Is this page present?

  public:
    MemPage();

    MemPage(uint64_t pn, pm_entry_t entry);

    MemPage(uint64_t pn, uint64_t pfn, uint8_t shift);

    MemPage(uint64_t pn, uint8_t swap_type, uint64_t swap_offset,
        uint8_t shift);

    MemPage(MemPage const & other);

    virtual
    ~MemPage();

    MemPage&
    operator=(const MemPage &other);

    uint64_t
    getPN() const
    {
      return _pn;
    }

    uint64_t
    getPFN() const
    {
      return _pfn;
    }

    /*
     uint8_t
     getSwapType() const
     {
     return _swap_type;
     }

     uint64_t
     getSwapOffset() const
     {
     return _swap_offset;
     }

     uint8_t
     getPageShift() const
     {
     return _shift;
     }
     */
    bool
    isSwapped() const
    {
      return _swapped;
    }

    bool
    isPresent() const
    {
      return _present;
    }
  };
}
#endif /* MEM_PAGE_HPP_ */
