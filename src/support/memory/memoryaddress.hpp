/*************************************************************************************/
/*      Copyright 2009-2015 Barcelona Supercomputing Center                          */
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

#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include <algorithm>
#include <ostream>

/*
 * \brief Abstraction layer for memory addresses.
 * \defails Address provides an easy to use wrapper
 * for address manipulation. Very useful if pointer
 * arithmetic is need to be used.
 */
class Address {
	private:
		uintptr_t value; //!< Memory address
	public:
		//! \brief Default constructor
		constexpr
		Address() : value(0) {}

		/*! \brief Constructor by initialization.
		 *  \details Creates a new Address instance
		 *  using an unsigned integer.
		 */
		constexpr
		Address( uintptr_t v ) : value( v ) {}

		/*! \brief Constructor by initialization.
		 *  \details Creates a new Address instance
		 *  using a pointer's address.
		 */
		template< typename T >
		constexpr
		Address( T* v ) : value( reinterpret_cast<uintptr_t>(v) ) {}

		//! \brief Copy constructor
		constexpr
		Address( Address const& o ) : value(o.value) {}

		//! \brief Null pointer constructor is not supported.
		Address( std::nullptr_t ) = delete;

		//! \brief Null pointer assignment is not supported.
		Address const& operator=( std::nullptr_t ) = delete;

		//! \brief Null pointer comparison is not supported.
		bool operator==( std::nullptr_t ) = delete;

		//! \brief Null pointer comparison is not supported.
		bool operator!=( std::nullptr_t ) = delete;

		//! \brief Checks if two addresses are equal
		constexpr
		bool operator==( Address const& o )  {
			return value == o.value;
		}

		//! \brief Checks if two addresses differ
		constexpr
		bool operator!=( Address const& o )  {
			return value != o.value;
		}

		/*! \brief Calculate an address using
		 *  a base plus an offset.
		 *  @param[in] size Offset to be applied
		 *  @returns A new address object displaced size bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		Address operator+( size_t size )  {
			return Address( reinterpret_cast<uintptr_t>(value) + size );
		}

		/*! \brief Calculate an address using
		 *  a base minus an offset.
		 *  @param[in] size Offset to be applied
		 *  @returns A new address object displaced size bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		size_t operator-( Address const& base )  {
			return reinterpret_cast<uintptr_t>(base.value)
				  - reinterpret_cast<uintptr_t>(value);
		}

		//! @returns the integer representation of the address
		constexpr
		operator uintptr_t() {
			return value;
		}

		/*! @returns the pointer representation of the address
		 * using any type.
		 * \tparam T type of the represented pointer. Default: void
		 */
		template< typename T = void >
		operator T*() {
			return reinterpret_cast<T*>(value);
		}

		/*! @returns returns an aligned address
		 * @param[in] alignment_constraint the alignment to be applied
		 */
		constexpr
		Address align( size_t alignment_constraint ) {
			return Address( value & ~(alignment_constraint-1) );
		}
};

/*! \brief Prints an address object to an output stream.
 *  \details String representation of an address in hexadecimal.
 */
std::ostream& operator<<(std::ostream& out, Address const &entry)
{
	return out << std::hex << static_cast<uintptr_t>( entry );
}

#endif // ADDRESS_HPP

