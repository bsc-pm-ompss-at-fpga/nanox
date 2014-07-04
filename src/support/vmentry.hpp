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

#ifndef VMENTRY_HPP_
#define VMENTRY_HPP_

#include <string>
#include <iostream>
#include <ostream>
#include <map>
#include <cstdint>

#include "mempage.hpp"

namespace nanos
{
namespace vm
{
  typedef struct
  {
    bool r :1;
    bool w :1;
    bool x :1;
  } prot_t;

  typedef struct
  {
    bool syscall :1;
    bool vdso :1;
    bool stack :1;
    bool heap :1;
    bool anonymous :1;
    bool shared :1;
  } vmflags_t;

  /*!
   * \brief Parse and store process memory map information contained in /proc/{pid}/maps file.
   */
  class VMEntry
  {

  private:
    uint64_t _start; // Beginning address
    uint64_t _end;   // Ending address

    // File related attributes
    uint64_t _offset; // Offset in the file where the mapping starts
    uint64_t _inode; // Inode identifier

    uint16_t _major; // Major device number
    uint16_t _minor; // Minor device number

    std::map<uint64_t, MemPage> _pages;

    std::string _path; // Mapped file path

    prot_t _prot;         // Access rights
    vmflags_t _flags;        // Page flags

  public:
    friend std::ostream&
    operator<<(std::ostream&, const VMEntry &);

    friend std::istream&
    operator>>(std::istream&, VMEntry &);

    VMEntry();

    VMEntry(uint64_t start, uint64_t end, prot_t prot, vmflags_t flags);

    VMEntry(uint64_t start, uint64_t end, prot_t prot, vmflags_t flags,
        uint64_t offset, uint64_t inode, uint8_t major, uint8_t minor,
        std::string path);

    VMEntry(const VMEntry &other);

    virtual
    ~VMEntry();

    VMEntry&
    operator=(const VMEntry &other);

    const MemPage&
    addPage(MemPage const &p);

    const MemPage&
    getPage(uint64_t pn) const;

    inline prot_t
    getAccessRights() const
    {
      return _prot;
    }

    inline uint64_t
    getStart() const
    {
      return _start;
    }

    inline uint64_t
    getEnd() const
    {
      return _end;
    }

    inline uint64_t
    getOffset() const
    {
      return _start;
    }

    inline uint64_t
    getInode() const
    {
      return _start;
    }

    inline uint64_t
    getDeviceMajor() const
    {
      return _start;
    }

    inline uint64_t
    getDeviceMinor() const
    {
      return _start;
    }

    inline std::string
    getPath() const
    {
      return _path;
    }

    inline bool
    isSyscallArea() const
    {
      return _flags.syscall;
    }

    inline bool
    isVDSO() const
    {
      return _flags.vdso;
    }

    inline bool
    isStackArea() const
    {
      return _flags.stack;
    }

    inline bool
    isHeapArea() const
    {
      return _flags.heap;
    }

    inline bool
    isRegularArea() const
    {
      return _flags.syscall;
    }

    inline bool
    isAnonymous() const
    {
      return _flags.anonymous;
    }

    inline bool
    isShared() const
    {
      return _flags.shared;
    }
  };
}
}
#endif /* VMENTRY_HPP_ */
