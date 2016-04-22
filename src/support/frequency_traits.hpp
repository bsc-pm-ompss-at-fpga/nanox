
#ifndef FREQUENCY_TRAITS_HPP
#define FREQUENCY_TRAITS_HPP

#include <chrono>
#include <ratio>
#include <type_traits>

template < class Rep, class Resolution >
class frequency;

template <class T>
struct is_frequency : std::false_type
{
};

template <class Rep, class Resolution>
struct is_frequency<frequency<Rep, Resolution> > : std::true_type
{
};

/*! \brief Frequency to duration type conversion
 * \details Since a conversion from a frequency to a duration
 * implies an inversion, measurement unit prefix (kilo, mega, etc.) must 
 * also be inverted ( kHz:1000/1 -> ms:1/1000 )
 */
template <class FromFreq>
struct to_duration
{
	//static_assert( is_frequency<FromFreq>::type, "Must provide a frequency type." );
	typedef typename FromFreq::rep _rep;
	typedef typename FromFreq::resolution _res;
	typedef std::ratio<_res::den, _res::num> _duration;

	// The type we were looking for
	typedef std::chrono::duration<_rep,_duration> type;
};

template <class ToFrequency, class Rep, class Resolution>
constexpr
typename std::enable_if< is_frequency<ToFrequency>::value, ToFrequency >::type
frequency_cast( frequency<Rep,Resolution> const& f );

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

#endif // FREQUENCY_TRATIS_HPP
