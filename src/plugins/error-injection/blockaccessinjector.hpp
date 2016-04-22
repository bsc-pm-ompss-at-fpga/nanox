
#ifndef BLOCK_ACCESS_INJECTOR_HPP
#define BLOCK_ACCESS_INJECTOR_HPP

#include "error-injection/periodicinjectionpolicy.hpp"
#include "memory/memorypage.hpp"
#include "memory/blockedpage.hpp"

#include <chrono>
#include <deque>
#include <set>
#include <random>

namespace nanos {
namespace error {

class BlockMemoryPageAccessInjector : public PeriodicInjectionPolicy<>
{
	private:
		std::deque<MemoryPage> _candidatePages;
		std::set<BlockedMemoryPage> _blockedPages;

	public:
		BlockMemoryPageAccessInjector( ErrorInjectionConfig const& properties ) noexcept :
			PeriodicInjectionPolicy( properties ),
			_candidatePages(),
			_blockedPages()
		{
		}

		void config( ErrorInjectionConfig const& properties )
		{
		}

		void injectError()
		{
			using distribution = std::uniform_int_distribution<size_t>;
			using dist_param = distribution::param_type;

			static distribution pageFaultDistribution;
			
			if( !_candidatePages.empty() ) {
				dist_param parameter( 0, _candidatePages.size() );
				size_t position = pageFaultDistribution( getRandomGenerator(), parameter );

				_blockedPages.emplace( _candidatePages[position] );
			}
		}

		void injectError( void *address )
		{
			_blockedPages.emplace( MemoryPage(address) );
		}

		void declareResource( void *address, size_t size )
		{
			MemoryPage::retrievePagesInsideChunk( _candidatePages, ::MemoryChunk( static_cast<Address>(address), size) );
		}

		void insertCandidatePage( MemoryPage const& page )
		{
			_candidatePages.push_back( page );
		}

		void recoverError( void* handle ) noexcept {
			Address failedAddress( handle );

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

