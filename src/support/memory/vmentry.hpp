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

#ifndef VMENTRY_HPP
#define VMENTRY_HPP

#include "accessrights.hpp"
#include "memoryaddress.hpp"

#include <string>
#include <iostream>
#include <ostream>
#include <cstdint>

namespace nanos {
namespace vm {

class MemoryPageFlags
{
	private:
		bool syscall :1;
		bool vdso :1;
		bool stack :1;
		bool heap :1;
		bool anonymous :1;
		bool shared :1;

	public:
		constexpr
		MemoryPageFlags() :
			syscall(false),
			vdso(false),
			stack(false),
			heap(false),
			anonymous(false),
			shared(false)
		{}

	constexpr bool isSyscallArea() const { return syscall; }
	
	constexpr bool isVDSO() const { return vdso; }
	
	constexpr bool isStackArea() const { return stack;	}
	
	constexpr bool isHeapArea() const { return heap; }
	
	constexpr bool isRegularArea() const { return !syscall; }
	
	constexpr bool isAnonymous() const { return anonymous; }
	
	constexpr bool isShared() const { return shared; }
};

/*!
 * \brief Parse and store process memory map information contained in /proc/{pid}/maps file.
 */
class VMEntry
{
  private:
	 MemoryChunk _region; //!< Area delimited by the mapping

	 // File related attributes
	 uint64_t _offset;       //!< Offset in the file where the mapping starts
	 uint64_t _inode;        //!< Inode identifier

	 uint16_t _major;        //!< Major device number
	 uint16_t _minor;        //!< Minor device number

	 std::string _path;      //!< Mapped file path

	 AccessRights _prot;     //!< Access rights
	 MemoryPageFlags _flags; //!< Page flags

	public:
		friend std::istream& operator>>(std::istream&, VMEntry &);

		VMEntry();

		VMEntry( MemoryChunk const& area, AccessRights const& prot, MemoryPageFlags const& flags);

		VMEntry( MemoryChunk const& area, AccessRights const& prot, MemoryPageFlags const& flags,
				uint64_t offset, uint64_t inode, uint8_t major, uint8_t minor, std::string path);

		VMEntry(VMEntry const &other);
	
		virtual ~VMEntry();
	
		VMEntry& operator=(VMEntry const &other);
	
		AccessRights const& getAccessRights() const { return _prot; }
	
		MemoryChunk const& getMemoryRegion() const { return _region; }
	
		uint64_t getOffset() const { return _offset; }
	
		uint64_t getInode() const { return _inode; }
	
		uint64_t getDeviceMajor() const { return _major; }
	
		uint64_t getDeviceMinor() const { return _minor; }
	
		std::string getPath() const { return _path; }
	
		AccessRights const& getAccessRights() const { return _prot; }
	
		MemoryPageFlags const& getPageFlags() const { return _flags; }
	
		void setMemoryRegion( MemoryChunk const& address ) { _start = address; }
	
		void setOffset( uint64_t offset ) { _offset = offset; }
	
		void setInode( uint64_t inode ) { _inode = inode; }
	
		void setDeviceMajor( uint64_t major ) { _major = major; }
	
		void setDeviceMinor() { _minor = minor; }
	
		void setPath( std::string const& path ) { _path = path; }
	
		void setAccessRights( AccessRights const& access ) { _prot = access; }
	
		void setPageFlags( MemoryPageFlags const& pageflags ) { _flags = pageflags; }
};

} // namespace vm
} // namespace nanos

#endif /* VMENTRY_HPP */

