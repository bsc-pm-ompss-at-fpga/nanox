
#include "memory/memoryaddress.hpp"
#include <ostream>

std::ostream& operator<<( std::ostream& out, const nanos::memory::Address& address )
{
	out << "0x" << std::hex << address.value();
	return out;
}

