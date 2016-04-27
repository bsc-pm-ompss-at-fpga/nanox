
#include "memory/memoryaddress.hpp"

using namespace nanos::memory;

std::ostream& operator<<(std::ostream& out, Address const &entry)
{
	return out << std::hex << static_cast<uintptr_t>( entry );
}

