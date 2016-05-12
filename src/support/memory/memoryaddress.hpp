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
#include <cstddef>
#include <ostream>

#include "error.hpp"

namespace nanos {
namespace memory {

/*
 * \brief Abstraction layer for memory addresses.
 * \defails Address provides an easy to use wrapper
 * for address manipulation. Very useful if pointer
 * arithmetic is need to be used.
 */
class Address {
	private:
		uintptr_t _value; //!< Memory address
	public:
		//! \brief Default constructor: uninitialized address does not make sense
		Address() = delete;

		/*! \brief Constructor by initialization.
		 *  \details Creates a new Address instance
		 *  using an unsigned integer.
		 */
		constexpr
		Address( uintptr_t v ) noexcept :
			_value( v )
		{
		}

		/*! \brief Constructor by initialization.
		 *  \details Creates a new Address instance
		 *  that points to 0x0
		 */
		constexpr
		Address( std::nullptr_t ) noexcept :
			_value( 0 )
		{
		}

		/*! \brief Constructor by initialization.
		 *  \details Creates a new Address instance
		 *  using a pointer's address.
		 */
		template< typename T >
		constexpr
		Address( T* v ) noexcept :
			_value( reinterpret_cast<uintptr_t>(v) )
		{
		}

		//! \brief Copy constructor
		constexpr
		Address( Address const& o ) noexcept :
			_value(o._value)
		{
		}

      constexpr
		uintptr_t value() noexcept
		{
			return _value;
		}

		//! \brief Checks if two addresses are equal
		constexpr
		bool operator==( Address const& o ) noexcept
		{
			return _value == o._value;
		}

		//! \brief Checks if two addresses differ
		constexpr
		bool operator!=( Address const& o ) noexcept
		{
			return _value != o._value;
		}

		/*! \brief Calculate an address using
		 *  a base plus an offset.
		 *  @param[in] offset Offset to be applied
		 *  @returns A new address object displaced offset bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		Address operator+( ptrdiff_t offset ) noexcept
		{
			//ensure( offset > 0 && _value < (_value+offset), "address value overflow" );
			//ensure( offset < 0 && _value > offset, "address value underflow" );
			return Address( _value + offset );
		}

		/*! \brief Calculate an address using
		 *  a base minus an offset.
		 *  @param[in] offset Offset to be applied
		 *  @returns A new address object displaced offset bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		Address operator-( ptrdiff_t offset ) noexcept
		{
			//ensure( offset > 0 && _value > offset, "address value underflow" );
			//ensure( offset < 0 && _value < (_value+offset), "address value overflow" );
			return Address( _value - offset );
		}

		/*! \brief Calculate an address using
		 *  a base minus an offset.
		 *  @param[in] size Offset to be applied
		 *  @returns A new address object displaced size bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		ptrdiff_t operator-( Address const& base ) noexcept
		{
			return _value - base._value;
		}

		Address operator+=( size_t size ) noexcept
		{
			//ensure( _value > (_value+size), "address value overflow" );
			_value += size;
			return *this;
		}

		Address& operator-=( size_t size ) noexcept
		{
			//ensure( _value > size, "address value underflow" );
			_value += size;
			return *this;
		}

		constexpr
		bool operator==( std::nullptr_t ) noexcept
		{
			return _value == 0;
		}

		constexpr
		bool operator!=( std::nullptr_t ) noexcept
		{
			return _value != 0;
		}

		//! \returns if this address is smaller than the reference
		constexpr
		bool operator<( Address const& reference ) noexcept
		{
			return _value < reference._value;
		}

		//! \returns if this address is greater than the reference
		constexpr
		bool operator>( Address const& reference ) noexcept
		{
			return _value > reference._value;
		}

		//! \returns if this address is smaller than or equal
		// to the reference
		constexpr
		bool operator<=( Address const& reference ) noexcept
		{
			return _value <= reference._value;
		}

		//! \returns if this address is greater than or equal
		// to the reference
		constexpr
		bool operator>=( Address const& reference ) noexcept
		{
			return _value >= reference._value;
		}

		/*! @returns the pointer representation of the address
		 * using any type.
		 * \tparam T type of the represented pointer. Default: void
		 */
		template< typename T = void >
		operator T*() noexcept
		{
			return reinterpret_cast<T*>(_value);
		}

		/*! @returns whether this address fulfills an
		 * alignment restriction or not.
		 * @param[in] alignment_constraint the alignment
		 * restriction
		 */
		constexpr
		bool isAligned( size_t alignment_constraint ) noexcept
		{
			return ( _value & (alignment_constraint-1)) == 0;
		}

		/*! @returns whether this address fulfills an
		 * alignment restriction or not.
		 * \tparam alignment_constraint the alignment
		 * restriction
		 */
		template< size_t alignment_constraint >
		constexpr
		bool isAligned() noexcept
		{
			return ( _value & (alignment_constraint-1)) == 0;
		}

		/*! @returns returns an aligned address
		 * @param[in] alignment_constraint the alignment to be applied
		 */
		constexpr
		Address align( size_t alignment_constraint ) noexcept
		{
			return Address(
						_value &
						~( alignment_constraint-1 )
						);
		}

		/*! @returns returns an aligned address
		 * @tparam alignment_constraint the alignment to be applied
		 */
		template< size_t alignment_constraint >
		constexpr
		Address align() noexcept
		{
			return Address(
						_value &
						~( alignment_constraint-1 )
						);
		}

		/*! @returns returns an aligned address
		 * @param[in] lsb least significant bit of the aligned address
		 *
		 * \detail LSB is a common term for specifying the important
		 *         part of an address in an specific context.
		 *         For example, in virtual page management, lsb is
		 *         usually 12 ( 2^12: 4096 is the page size ).
		 *
		 *         Basically we have to build a mask where all the bits
		 *         in a position less significant than lsb are equal
		 *         to 0:
		 *          1) Create a number with a '1' value in the lsb-th
		 *             position.
		 *          2) Substract one: all bits below the lsb-th will
		 *             be '1'.
		 *          3) Perform a bitwise-NOT to finish the mask with
		 *             all 1s but in the non-significant bits.
		 */
		constexpr
		Address alignToLSB( short lsb ) noexcept
		{
			return Address(
						_value &
						~( (1<<lsb)-1 )
						);
		}

		/*! @returns returns an aligned address
		 * @tparam alignment_constraint the alignment to be applied
		 * \sa alignUsingLSB"("short lsb")"
		 */
		template< short lsb >
		constexpr
		Address alignToLSB() noexcept
		{
			return Address(
						_value &
						~( (1<<lsb)-1 )
						);
		}
};

/*! \brief Prints an address object to an output stream.
 *  \details String representation of an address in hexadecimal.
 */
inline std::ostream& operator<<( std::ostream& out, const nanos::memory::Address& address )
{
	out << "0x" << std::hex << address.value();
	return out;
}

} // namespace nanos 
} // namespace memory

namespace std {
/** Hash function for memory addresses
 * TODO this should be polished to improve
 * the quality of the results.
 */
template<>
struct hash<nanos::memory::Address> {
	typedef nanos::memory::Address argument_type;
	typedef uintptr_t result_type;

	result_type operator()(argument_type const& arg) const
	{
		return arg.value();
	}
};

} // namespace std

#endif // ADDRESS_HPP

