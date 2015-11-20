
#ifndef BLOCK_ACCESS_INJECTOR_HPP
#define BLOCK_ACCESS_INJECTOR_HPP

#include "periodicinjectionpolicy.hpp"
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
		std::deque<MemoryPage> candidatePages;
		std::set<BlockedMemoryPage> blockedPages;

	public:
		BlockMemoryPageAccessInjector( ErrorInjectionConfig const& properties ) noexcept :
			PeriodicInjectionPolicy( properties ),
			candidatePages(),
			blockedPages()
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
			
			if( !candidatePages.empty() ) {
				dist_param parameter( 0, candidatePages.size() );
				size_t position = pageFaultDistribution( getRandomGenerator(), parameter );

				blockedPages.emplace( candidatePages[position] );
			}
		}

		void injectError( void *address )
		{
			blockedPages.emplace( MemoryPage(address) );
		}

		void declareResource( void *address, size_t size )
		{
			MemoryPage::retrievePagesInsideChunk( candidatePages, ::MemoryChunk( static_cast<Address>(address), size) );
		}

		void insertCandidatePage( MemoryPage const& page )
		{
			candidatePages.push_back( page );
		}

		void recoverError( void* handle ) noexcept {
			Address failedAddress( handle );

			for( auto it = blockedPages.begin(); it != blockedPages.end(); it++ ) {
				if( it->contains( failedAddress ) ) {
					blockedPages.erase( it );
					return;
				}
			}
		}
};

} // namespace error
} // namespace nanos

#endif // BLOCK_ACCESS_INJECTOR_HPP

