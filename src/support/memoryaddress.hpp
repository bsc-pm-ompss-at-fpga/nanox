#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include <algorithm>
#include <iostream>

class Address {
	private:
		uintptr_t value;
	public:
		constexpr
		Address() : value(0) {}

		constexpr
		Address( uintptr_t v ) : value( v ) {}

		template< typename T >
		constexpr
		Address( T* v ) : value( reinterpret_cast<uintptr_t>(v) ) {}

		constexpr
		Address( Address const& o ) : value(o.value) {}

		Address( std::nullptr_t ) = delete;

		Address const& operator=( std::nullptr_t ) = delete;

		bool operator==( std::nullptr_t ) = delete;

		bool operator!=( std::nullptr_t ) = delete;

		constexpr
		bool operator==( Address const& o )  {
			return value == o.value;
		}

		constexpr
		bool operator!=( Address const& o )  {
			return value != o.value;
		}

		constexpr
		Address operator+( size_t size )  {
			return Address( reinterpret_cast<uintptr_t>(value) + size );
		}

		constexpr
		size_t operator-( Address const& base )  {
			return reinterpret_cast<uintptr_t>(base.value)
				  - reinterpret_cast<uintptr_t>(value);
		}

		constexpr
		operator uintptr_t() {
			return value;
		}

		template< typename T >
		operator T*() {
			return reinterpret_cast<T*>(value);
		}

		void* data()  {
			return reinterpret_cast<void*>(value);
		}

		constexpr
		Address align( size_t alignment_constraint ) {
			return Address( value & ~(alignment_constraint-1) );
		}
};

std::ostream& operator<<(std::ostream& out, Address const &entry)
{
	return out << std::hex << static_cast<uintptr_t>( entry );
}

#endif // ADDRESS_HPP

