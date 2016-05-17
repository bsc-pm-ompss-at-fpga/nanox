
#ifndef BLOCK_ACCESS_INJECTOR_HPP
#define BLOCK_ACCESS_INJECTOR_HPP

#include "error-injection/errorinjectionconfiguration.hpp"
#include "error-injection/errorinjectionpolicy.hpp"
#include "exception/failurestats.hpp"
#include "memory/memorypage.hpp"
#include "memory/blockedpage.hpp"

#include <deque>
#include <random>
#include <set>

namespace nanos {
namespace error {

template < typename RandomEngine = std::minstd_rand >
class BlockMemoryPageAccessInjector : public ErrorInjectionPolicy
{
	private:
		std::deque<memory::MemoryPage>      _candidatePages;
		std::set<memory::BlockedMemoryPage> _blockedPages;
		RandomEngine                        _generator;

	public:
		BlockMemoryPageAccessInjector( ErrorInjectionConfig const& properties ) noexcept :
			ErrorInjectionPolicy( properties ),
			_candidatePages(),
			_blockedPages(),
			_generator( properties.getInjectionSeed() )
		{
		}

		virtual ~BlockMemoryPageAccessInjector()
		{
		}

		RandomEngine& getRandomGenerator() { return _generator; }

		virtual void injectError()
		{
			using distribution = std::uniform_int_distribution<size_t>;
			using dist_param = distribution::param_type;

			static distribution pageFaultDistribution;
			
			if( !_candidatePages.empty() ) {
				dist_param parameter( 0, _candidatePages.size() );
				size_t position = pageFaultDistribution( _generator, parameter );

				_blockedPages.emplace( _candidatePages[position] );
			}
			FailureStats<ErrorInjection>::increase();
		}

		virtual void injectError( void *address )
		{
			_blockedPages.emplace( memory::MemoryPage(address) );
		}

		virtual void declareResource( void *address, size_t size )
		{
			memory::MemoryPage::retrievePagesInsideChunk( _candidatePages, memory::MemoryChunk( static_cast<memory::Address>(address), size) );
		}

		void insertCandidatePage( memory::MemoryPage const& page )
		{
			_candidatePages.push_back( page );
		}

		virtual void recoverError( void* handle ) noexcept {
			memory::Address failedAddress( handle );

			for( auto it = _blockedPages.begin(); it != _blockedPages.end(); it++ ) {
				if( it->contains( failedAddress ) ) {
					_blockedPages.erase( it );
					return;
				}
			}
		}
};

} // namespace error
} // namespace nanos

#endif // BLOCK_ACCESS_INJECTOR_HPP

