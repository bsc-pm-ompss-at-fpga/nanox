
#include "memoryaddress.hpp"

namespace nanos {
namespace memory {

std::ostream& operator<<(std::ostream& out, Address const &entry)
{
	out << "0x" << std::hex << entry.value();
	return out;
}

}
}

