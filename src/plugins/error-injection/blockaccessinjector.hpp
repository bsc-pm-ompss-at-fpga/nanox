
#ifndef BLOCK_ACCESS_INJECTOR_HPP
#define BLOCK_ACCESS_INJECTOR_HPP

#include "errorinjectionpolicy.hpp"
#include "memory/memorytracker.hpp"
#include "memory/blockedpage.hpp"

#include <chrono>
#include <random>

namespace nanos {
namespace error {

class BlockMemoryPageAccessInjector : public ErrorInjectionPolicy
{
	private:
		MemoryTracker<MemoryPage> candidatePages;
		MemoryTracker<BlockedMemoryPage> blockedPages;

		std::mt19937 randomNumberGenerator;
		std::exponential_distribution<float> waitTimeDistribution;
		std::uniform_real_distribution<float> pageFaultDistribution;

		using seconds = std::chrono::duration<float>;
	public:
		BlockMemoryPageAccessInjector( ErrorInjectionConfig const& properties ) noexcept :
			ErrorInjectionPolicy(),
			candidatePages(),
			blockedPages(),
			randomNumberGenerator( properties.getInjectionSeed() ),
			waitTimeDistribution( properties.getInjectionRate() ),
			pageFaultDistribution(0, 1)
		{
		}

		seconds getWaitTime() noexcept {
			return seconds( waitTimeDistribution(randomNumberGenerator) );
		}

		void injectError() {
			if( !candidatePages.isEmpty() ) {
				size_t position = candidatePages.getTotalSize() 
										* pageFaultDistribution( randomNumberGenerator );

				auto it = candidatePages.begin();
				while( it != candidatePages.end() && position >= it->size() )
				{
					position -= it->size();
					it++;
				}

				if( it != candidatePages.end() ) {
					blockedPages.insert( *it );
					candidatePages.erase( it );
				}
			}
		}

		void injectError( void *page )
		{
			blockedPages.emplace( BlockedPage
		}

		void insertCandidatePage( MemoryPage const& page ) {
			candidatePages.insert( page );
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

