
#ifndef FREQUENCY_HPP
#define FREQUENCY_HPP

#include "frequency_traits.hpp"

#include <chrono>
#include <ratio>

/*!
 * \details Frequency class that imitates the behavior of std::chrono::duration
 * \tparam Rep an arithmetic type representing the number of cycles
 * \tparam Resolution a std::ratio representing the cycle resolution (number of ticks per second)
 *                    a std::ratio<1> represents Hertz
 */
template < class Rep, class Resolution = std::ratio<1> >
class frequency {
	private:
		Rep _cycles;
	public:
		typedef Rep rep;
		typedef Resolution resolution;

		frequency( Rep cycles ) :
				_cycles( cycles )
		{
		}

		template < class ToFrequency >
		operator ToFrequency()
		{
			return frequency_cast<ToFrequency>( *this );
		}

		operator Rep&()
		{
			return _cycles;
		}

		Rep count() const { return _cycles; }
};


#endif // FREQUENCY_HPP

