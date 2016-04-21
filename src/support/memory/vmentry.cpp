/*************************************************************************************/
/*      Copyright 2019-2015 Barcelona Supercomputing Center                          */
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

#include "vmentry.hpp"
#include "memoryaddress.hpp"

#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

namespace nanos {
namespace vm {
	VMEntry::VMEntry() :
		_region(), _offset(0), _inode(0), _major(0), _minor(0), _path(), _prot(), _flags()
	{
	}
	
	VMEntry::VMEntry(MemoryChunk const& area, AccessRights const& prot, MemoryPageFlags const& flags) :
		_region(area), _offset(0), _inode(0), _major(0), _minor(0), _path(),
		_prot(prot), _flags(flags)
	{
	}
	
	VMEntry::VMEntry(MemoryChunk const& area, AccessRights const& prot, MemoryPageFlags const& flags,
		uint64_t offset, uint64_t inode, uint8_t major, uint8_t minor,	std::string path) :
		_region(area), _offset(offset), _inode(inode), _major(major),
		_minor(minor), _path(path), _prot(prot), _flags(flags)
	{
	}
	
	VMEntry::VMEntry(const VMEntry &other) :
		_region(other._region), _offset(other._offset),
		_inode(other._inode), _major(other._major), _minor(other._minor),
		_path(other._path), _prot(other._prot), _flags(other._flags)
	{
	}
	
	VMEntry::~VMEntry()
	{
	}
	
	VMEntry& VMEntry::operator=(const VMEntry &other)
	{
		_region = other._region;
		_offset = other._offset;
		_inode = other._inode;
		_major = other._major;
		_minor = other._minor;
		_path = other._path;
		_prot = other._prot;
		_flags = other._flags;
		
		return *this;
	}
	
	std::ostream& operator<<(std::ostream& out, VMEntry const &entry)
	{
		using namespace std;
		std::stringstream ss;
		
		char visibility;
		if( entry.getPageFlags().isShared() )
			visibility = 's';
		else
			visibility = 'p';
		
		ss << setfill('0') << setw(8) << entry.getMemoryRegion().begin()
			<< '-' 
			<< setfill('0') << setw(8) << entry.getMemoryRegion().end()
			<< ' ' << entry.getAccessRights() << visibility
			<< ' ' << setfill('0') << setw(8) << entry.getOffset() 
			<< ' ' << setfill('0') << setw(2) << hex << entry.getDeviceMajor() 
			<< ':' << setfill('0') << setw(2) << hex << entry.getDeviceMinor()
			<< ' ' << dec << entry.getInode() << ' ';
		
		if (!entry._path.empty())
		{
			int pos = ss.tellp();
			for (int i = pos; i < 73; i++)
				ss.put(' ');
			ss << entry._path;
		}
		
		return out << ss;
	}
	
	std::istream& operator>>(std::istream& input, VMEntry &entry)
	{
		using namespace std;

		uintptr_t start, end;
		input >> start;
		input.get();
		input >> end;

		entry.setMemoryRegion( MemoryChunk(start, end) );

		input >> entry._prot;

		MemoryPageFlags pageflags;
		pageflags.setShared( input.get() == 's' );

		input >> entry._offset;
		
		input >> hex >> setw(2) >> entry._major;
		input.get();
		input >> hex >> setw(2) >> entry._minor;
		input >> dec >> entry._inode;
		
		// Next character will be a newline (non aditional info) or a whitespace
		while (input.peek() == ' ')
			input.get(); // Discards leading whitespaces...
		// ... and consumes the remaining of the line
		getline(input, entry._path, '\n');
		
		if (!entry._path.empty()) // dont consume the second character, as it might belong to the 'path'
		{
			pageflags.setSyscall( entry._path == "[syscall]" || entry._path == "[vectors]" || entry._path == "[vsyscall]" );
			pageflags.setVDSO( entry._path == "[vdso]" );
			pageflags.setHeap( entry._path == "[heap]" );
			pageflags.setStack( entry._path == "[stack]" );
		} else {
			pageflags.setAnonymous(true);
		}
		entry.setPageFlags( pageflags );
		return input;
	}
}
}
