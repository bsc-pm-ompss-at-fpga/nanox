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
		uintptr_t value; //!< Memory address
	public:
		//! \brief Default constructor: uninitialized address does not make sense
		Address() = delete;

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

		//! \brief Checks if two addresses are equal
		constexpr
		bool operator==( Address const& o ) {
			return value == o.value;
		}

		//! \brief Checks if two addresses differ
		constexpr
		bool operator!=( Address const& o ) {
			return value != o.value;
		}

		/*! \brief Calculate an address using
		 *  a base plus an offset.
		 *  @param[in] size Offset to be applied
		 *  @returns A new address object displaced size bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		Address operator+( size_t size ) {
			return Address( value + size );
		}

		/*! \brief Calculate an address using
		 *  a base minus an offset.
		 *  @param[in] size Offset to be applied
		 *  @returns A new address object displaced size bytes
		 *  with respect to the value of this object.
		 */
		constexpr
		size_t operator-( Address const& base ) {
			return base.value-value;
		}

		Address operator+=( size_t size ) {
			value += size;
			return *this;
		}

		Address operator-=( size_t size ) {
			value += size;
			return *this;
		}

		//! \returns if this address is smaller than the reference
		constexpr
		bool operator<( Address const& reference ) {
			return value < reference.value;
		}

		//! \returns if this address is greater than the reference
		constexpr
		bool operator>( Address const& reference ) {
			return value > reference.value;
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

		/*! @returns whether this address fulfills an
		 * alignment restriction or not.
		 * @param[in] alignment_constraint the alignment
		 * restriction
		 */
		constexpr
		bool isAligned( size_t alignment_constraint ) {
			return ( value & (alignment_constraint-1)) == 0;
		}

		/*! @returns whether this address fulfills an
		 * alignment restriction or not.
		 * \tparam alignment_constraint the alignment
		 * restriction
		 */
		template< size_t alignment_constraint >
		constexpr
		bool isAligned() {
			return ( value & (alignment_constraint-1)) == 0;
		}

		/*! @returns returns an aligned address
		 * @param[in] alignment_constraint the alignment to be applied
		 */
		constexpr
		Address align( size_t alignment_constraint ) {
			return Address(
						value &
						~( alignment_constraint-1 )
						);
		}

		/*! @returns returns an aligned address
		 * @tparam alignment_constraint the alignment to be applied
		 */
		template< size_t alignment_constraint >
		constexpr
		Address align() {
			return Address(
						value &
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
		Address alignToLSB( short lsb ) {
			return Address(
						value &
						~( (1<<lsb)-1 )
						);
		}

		/*! @returns returns an aligned address
		 * @tparam alignment_constraint the alignment to be applied
		 * \sa alignUsingLSB"("short lsb")"
		 */
		template< short lsb >
		constexpr
		Address alignToLSB() {
			return Address(
						value &
						~( (1<<lsb)-1 )
						);
		}
};

} // namespace nanos 
} // namespace memory

/*! \brief Prints an address object to an output stream.
 *  \details String representation of an address in hexadecimal.
 */
std::ostream& operator<<(std::ostream& out, nanos::memory::Address const &entry);

#endif // ADDRESS_HPP

