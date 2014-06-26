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

#include "mempage.hpp"
#include <iostream>

namespace vm
{
  MemPage::MemPage() :
      _pn(0), _pfn(0), /*_swap_offset(0), _swap_type(0), _shift(0), */_swapped(
          false), _present(false)
  {
  }

  MemPage::MemPage(uint64_t pn, pm_entry_t entry) :
      _pn(pn)//, _swap_offset(0), _swap_type(0)
  {
    //_shift = PM_PSHIFT(entry);
    _swapped = entry & PM_SWAP;
    if ((_present = entry & PM_PRESENT))
      {
        if (!_swapped) // Check if the map is swapped or not
          {
            _pfn = PM_PFRAME(entry); // If not swapped, set frame number
          }
        else
          {

          }
      }
  }

  MemPage::MemPage(uint64_t pn, uint64_t pfn, uint8_t shift) :
      _pn(pn), _pfn(pfn), /*_swap_offset(0), _swap_type(0), _shift(shift), */_swapped(
          false), _present(true)
  {
  }

  MemPage::MemPage(uint64_t pn, uint8_t swap_type, uint64_t swap_offset,
      uint8_t shift) :
      _pn(pn), _pfn(0), /*_swap_offset(swap_offset), _swap_type(swap_type), _shift(
       shift),*/_swapped(true), _present(true)
  {
  }

  MemPage::MemPage(MemPage const & other) :
      _pn(other._pn), _pfn(other._pfn), /*_swap_offset(other._swap_offset), _swap_type(
       other._swap_type), _shift(other._shift), */_swapped(other._swapped), _present(
          other._present)
  {
  }

  MemPage::~MemPage()
  {
  }
  /*
   std::istream&
   operator>>(std::istream& s, MemPage &mp)
   {
   uint64_t pm_entry = 0;
   // The read operation doesn't write the read value into pm_entry variable.
   s.readsome(reinterpret_cast < char * > (&pm_entry), sizeof(uint64_t)); // look for the corresponding entry in pagemaps file
   mp._present = pm_entry & PM_PRESENT;
   mp._swapped = pm_entry & PM_SWAP;
   if (mp._present)
   {
   if (!mp._swapped) // Check if the map is swapped or not
   {
   mp._pfn = PM_PFRAME(pm_entry);
   }
   }
   return s;
   }
   */

  std::ostream&
  operator<<(std::ostream& s, const MemPage &mp)
  {
    using namespace std;

    s << "{ pn: 0x" << hex << mp._pn;
    if (mp._present)
      {
        s << " (present), pfn: 0x" << hex << mp._pfn << " }";
      }
    else if (mp._swapped)
      {
        s << " (swap) }";
      }
    else
      {
        s << " (page fault) }";
      }
    return s;
  }
}
