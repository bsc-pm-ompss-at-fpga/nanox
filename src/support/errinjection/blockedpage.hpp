
#include "memorychunk.hpp"

class BlockedMemoryPage : public MemoryPage {
	private:

	public:
		BlockedMemoryPage( Address const& beginAddress, Address const& endAddress ) :
			MemoryPage( beginAddress, endAddress )
		{
			// Blocks this area of memory from reading
			mprotect( getBaseAddress(), getSize(), PROT_NONE );
		}

		BlockedMemoryPage( Address const& baseAddress, size_t length ) :
			MemoryPage( baseAddress, length )
		{
			// Blocks this area of memory from reading
			mprotect( getBaseAddress(), getSize(), PROT_NONE );
		}

		virtual ~BlockedMemoryPage
		{
			// Restores access rights for this area of memory
			mprotect( getBaseAddress(), getSize(), PROT_READ | PROT_WRITE );
		}
};
