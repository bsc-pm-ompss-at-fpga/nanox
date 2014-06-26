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

#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

#include "vmentry.hpp"
#include "mempage.hpp"

namespace vm
{
  VMEntry::VMEntry() :
      _start(0), _end(0), _offset(0), _inode(0), _major(0), _minor(0), _path()
  {
    _prot.r = false;
    _prot.w = false;
    _prot.x = false;
    _flags.anonymous = false;
    _flags.heap = false;
    _flags.shared = false;
    _flags.stack = false;
    _flags.syscall = false;
    _flags.vdso = false;
  }

  VMEntry::VMEntry(uint64_t start, uint64_t end, prot_t prot, vmflags_t flags) :
      _start(start), _end(end), _offset(0), _inode(0), _major(0), _minor(0), _pages(), _path(), _prot(
          prot), _flags(flags)
  {
  }

  VMEntry::VMEntry(uint64_t start, uint64_t end, prot_t prot, vmflags_t flags,
      uint64_t offset, uint64_t inode, uint8_t major, uint8_t minor,
      std::string path) :
      _start(start), _end(end), _offset(offset), _inode(inode), _major(major), _minor(
          minor), _pages(), _path(path), _prot(prot), _flags(flags)
  {
  }

  VMEntry::VMEntry(const VMEntry &other) :
      _start(other._start), _end(other._end), _offset(other._offset), _inode(
          other._inode), _major(other._major), _minor(other._minor), _pages(
          other._pages), _path(other._path), _prot(other._prot), _flags(
          other._flags)
  {
  }

  VMEntry::~VMEntry()
  {
  }

  VMEntry&
  VMEntry::operator=(const VMEntry &other)
  {
    _start = other._start;
    _end = other._end;
    _offset = other._offset;
    _inode = other._inode;
    _major = other._major;
    _minor = other._minor;
    _pages = other._pages;
    _path = other._path;
    _prot = other._prot;
    _flags = other._flags;

    return *this;
  }

  const MemPage&
  VMEntry::addPage(MemPage const &p)
  {
    std::pair<std::map<uint64_t, MemPage>::iterator, bool> it = _pages.insert(
        std::pair<uint64_t, MemPage>(p.getPN(), p));
    return it.first->second;
  }

  const MemPage&
  VMEntry::getPage(uint64_t pn) const
  {
    return _pages.find(pn)->second;
  }

  std::ostream&
  operator<<(std::ostream& out, VMEntry const &entry)
  {
    using namespace std;
    char prot[5];
    std::stringstream ss;
    prot[0] = entry._prot.r ? 'r' : '-';
    prot[1] = entry._prot.w ? 'w' : '-';
    prot[2] = entry._prot.x ? 'x' : '-';
    prot[3] = entry._flags.shared ? 's' : 'p';
    prot[4] = '\0';

    ss << setfill('0') << setw(8) << hex << entry._start << '-' << setfill('0')
        << setw(8) << hex << entry._end << ' ' << prot << ' ' << setfill('0')
        << setw(8) << entry._offset << ' ' << setfill('0') << setw(2) << hex
        << entry._major << ':' << setfill('0') << setw(2) << hex << entry._minor
        << ' ' << dec << entry._inode << ' ';

    if (!entry._path.empty())
      {
        int pos = ss.tellp();
        for (int i = pos; i < 73; i++)
          ss.put(' ');
        ss << entry._path;
      }

    return out << ss.str();
  }

  std::istream&
  operator>>(std::istream& input, VMEntry &entry)
  {
    using namespace std;

    string prot;

    input >> hex >> entry._start;
    input.get();
    input >> hex >> entry._end;
    input >> setw(4) >> prot;
    input >> hex >> entry._offset;

    input >> hex >> setw(2) >> entry._major;
    input.get();
    input >> hex >> setw(2) >> entry._minor;
    input >> dec >> entry._inode;

    entry._prot.r = prot[0] == 'r';
    entry._prot.w = prot[1] == 'w';
    entry._prot.x = prot[2] == 'x';

    entry._flags.shared = prot[3] == 's';
    // Next character will be a newline (non aditional info) or a whitespace

    while (input.peek() == ' ')
      input.get(); // Discards leading whitespaces...
    // ... and consumes the remaining of the line
    getline(input, entry._path, '\n');

    if (!entry._path.empty()) // dont consume the second character, as it might belong to the 'path'
      {
        if (entry._path == "[syscall]" || entry._path == "[vectors]"
            || entry._path == "[vsyscall]")
          entry._flags.syscall = true;
        else if (entry._path == "[vdso]")
          entry._flags.vdso = entry._prot.r && entry._prot.x;
        else if (entry._path == "[heap]")
          {
            entry._flags.heap = true;
          }
        else if (entry._path == "[stack]")
          {
            entry._flags.stack = true;
          }
      }
    else
      {
        entry._flags.anonymous = true;
      }
    return input;
  }
}
