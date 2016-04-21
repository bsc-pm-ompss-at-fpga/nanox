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

#ifndef ACCESS_RIGHTS_HPP
#define ACCESS_RIGHTS_HPP

/*!
 * \brief Represents a resource's access rights (read, write and execution).
 * \author Jorge Bellon
 */
class AccessRights
{
	private:
		bool read :1;			//!< Whether a resource is readable or not.
		bool write :1;			//!< Whether a resource is writable or not.
		bool execution :1;	//!< Whether a resource is executable or not.

	public:
		//! Default constructor. No rights are given.
		constexpr
		AccessRights() :
				read(false),
				write(false),
				execution(false)
		{}

		/*! \brief Constructor by initialization.
		 * \details Construct an AccessRights instance
		 * using a mask. Value must be representable with 3 bits.
		 * Examples (not restricted to them):
		 *  - 0 ( b'000 ): No rights.
		 *  - 4 ( b'100 ): Write rights only.
		 *  - 6 ( b'110 ): Read/write rights.
		 *  - 5 ( b'101 ): Read and execution rights.
		 *  - 7 ( b'111 ): Read, write and execution rights.
		 */
		constexpr
		AccessRights( unsigned short access_mask ) :
				read( access_mask & 1<<2 ),
				write( access_mask & 1<<1 ),
				execution( access_mask & 1<<0 )
		{
			static_assert( access_bits < 7, "Illegal access rights mask." );
		}

		/*! \brief Constructor by initialization.
		 * \details Construct an AccessRights instance
		 * giving the boolean values of each access right
		 * independently.
		 */
		constexpr
		AccessRights( bool readable, bool writable, bool executable ) :
				read( readable ),
				write( writable ),
				execution( executable )
		{}

		//! @returns whether the resource is readable or not.
		constexpr bool isReadable() const { return read; }

		//! @returns whether the resource is writable or not.
		constexpr bool isWritable() const { return write; }

		//! @returns whether the resource is executable or not.
		constexpr bool isExecutable() const { return execution; }

		/*! \brief Sets read access rights
		 * @param[in] flag value to be assigned.
		 */
		void setReadable( bool flag ) { read = flag; }

		/*! \brief Sets write access rights
		 * @param[in] flag value to be assigned.
		 */
		void setWritable( bool flag ) { write = flag; }

		/*! \brief Sets execution access rights
		 * @param[in] flag value to be assigned.
		 */
		void setExecutable( bool flag ) { execute = flag; }
};

/*! \brief Dump a string representation of the access rights to a stream.
 *  \details Print execution rights to a stream using 3 character representation (rwx):
 *  - r: resource is readable
 *  - w: resource is writable
 *  - x: resource is executable
 */
std::ostream& operator<<(std::ostream& out, AccessRights const &access)
{
	std::string protection( "---" );

	if( access.isReadable() )
		protection[0] = 'r';

	if( access.isWritable() )
		protection[1] = 'w';

	if( access.isExecutable() )
		protection[2] = 'x';

	return out << protection;
}

/*! \brief Reads access rights from a stream and sets an existing instance accordingly.
 *  \details Read execution rights from a stream using 3 character representation (rwx):
 *  - r: resource is readable
 *  - w: resource is writable
 *  - x: resource is executable
 */
std::istream& operator>>(std::istream& input, AcessRights &access)
{
	access.setReadable( input.get() == 'r' );
	access.setWritable( input.get() == 'w' );
	access.setExecutable( input.get() == 'x' );
}

#endif // ACCESS_RIGHTS_HPP
