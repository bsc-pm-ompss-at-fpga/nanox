
#include "errorinjectionpolicy.hpp"
#include "memorytracker.hpp"

class BlockMemoryPageAccessInjector : public ErrorInjectionPolicy
{
	private:
		MemoryTracker<MemoryPage> candidatePages;
		MemoryTracker<BlockedPage> blockedPages;

		std::mt19937 randomNumberGenerator;
		std::uniform_real_distribution pageFaultDistribution;
		std::exponential_distribution<float> waitTimeDistribution;

	public:
		BlockmemoryPageAccessInjector() std::noexcept :
			ErrorInjectionPolicy(),
			randomNumberGenerator(),
			waitTimeDistribution(),
			pageFaultDistribution(0, 1)
		{}

		void config( ErrorInjectionConfig const& properties ) {
			randomNumberGenerator( properties.getInjectionSeed() );
			waitTimeDistribution( properties.getInjectionRate() );
		}

		std::chrono::seconds<float> getWaitTime() std::noexcept {
			return std::chrono:seconds<float>( waitTimeDistribution(randomNumberGenerator) );
		}

		void injectError() {
			if( !candidatePages.empty() ) {
				size_t position = candidatePages.getTotalSize() 
										* pageFaultDistribution( randomNumberGenerator );

				for( auto it = candidatePages.begin(); 
						it != candidatePages.end() && position >= it->getSize();
						it++ )
				{
					position -= it->getSize();
				}

				if( it != candidatePages.end() ) {
					blockedPages.emplace_back( *it );
					candidatePages.erase( it );
				}
			}
		}

		void insertCandidatePage( MemoryPage const& page ) {
			candidatePages.emplace_back( page );
		}

		void recoverError( void* handle ) {
			Address failedAddress( handle );

			for( auto it = blockedPages.begin(); it != blockedPages.end(); it++ ) {
				if( it->contains( failedAddress ) ) {
					blockedPages.erase( it );
					return;
				}
			}
		}
};
