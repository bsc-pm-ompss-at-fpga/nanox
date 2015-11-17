
#ifndef FREQUENCY_HPP
#define FREQUENCY_HPP

#include <chrono>
#include <ratio>
#include <type_traits>

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

		template < class Rep2, class Resolution2 >
		frequency( frequency<Rep2,Resolution2> const& other ) :
				_cycles( other._cycles 
					* std::ratio_divide< Resolution, Resolution2 >::num
					/ std::ratio_divide< Resolution, Resolution2 >::den
				 )
		{
		}

		Rep count() const { return _cycles; }
};

template <class T>
struct is_frequency : std::false_type
{
};

template <class Rep, class Resolution>
struct is_frequency<frequency<Rep, Resolution> > : std::true_type
{
};


template<typename ToFreq, typename CF, typename CR,
        bool NumIsOne = false, bool DenIsOne = false>
struct _frequency_cast_impl
{
   template<typename Rep, typename Resolution>
   static constexpr ToFreq cast(const frequency<Rep, Resolution>& d)
   {
       typedef typename ToFreq::rep to_rep;
       return ToFreq(static_cast<to_rep>(static_cast<CR>(d.count())
         * static_cast<CR>(CF::num)
         / static_cast<CR>(CF::den)));
   }
};

template<typename ToFreq, typename CF, typename CR>
struct _frequency_cast_impl<ToFreq, CF, CR, true, true>
{
   template<typename Rep, typename Resolution>
   static constexpr ToFreq cast(const frequency<Rep, Resolution>& d)
   {
       typedef typename ToFreq::rep to_rep;
       return ToFreq(static_cast<to_rep>(static_cast<CR>(d.count())));
   }
};

template<typename ToFreq, typename CF, typename CR>
struct _frequency_cast_impl< ToFreq, CF, CR, true, false>
{
   template<typename Rep, typename Resolution>
   static constexpr ToFreq cast(const frequency<Rep, Resolution>& d)
   {
       typedef typename ToFreq::rep to_rep;
       return ToFreq(static_cast<to_rep>(static_cast<CR>(d.count())
         / static_cast<CR>(CF::den)));
   }
};

template<typename ToFreq, typename CF, typename CR>
struct _frequency_cast_impl< ToFreq, CF, CR, false, true>
{
   template<typename Rep, typename Resolution>
   static constexpr ToFreq cast(const frequency<Rep, Resolution>& d)
   {
       typedef typename ToFreq::rep to_rep;
       return ToFreq(static_cast<to_rep>(static_cast<CR>(d.count())
         * static_cast<CR>(CF::num)));
   }
};

template <class ToFrequency, class Rep, class Resolution>
constexpr
typename std::enable_if< is_frequency<ToFrequency>::value, ToFrequency >::type
frequency_cast( frequency<Rep,Resolution> const& f )
{
	typedef typename ToFrequency::resolution to_resolution;
	typedef typename ToFrequency::rep to_rep;
	typedef std::ratio_divide<Resolution, to_resolution> cf;
	typedef typename std::common_type<to_rep, Rep, intmax_t>::type cr;
	typedef _frequency_cast_impl<ToFrequency, cf, cr, 
							cf::num == 1, cf::den == 1> freq_cast;
	return freq_cast::cast( f );
}

#endif // FREQUENCY_HPP

