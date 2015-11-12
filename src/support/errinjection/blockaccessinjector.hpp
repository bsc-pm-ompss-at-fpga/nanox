
#include "memorytracker.hpp"

class BlockMemoryPageAccessInjector {
	private:
		MemoryTracker<MemoryPage> candidatePages;
		MemoryTracker<BlockedPage> blockedPages;

		std::mt19937 randomNumberGenerator;
		std::uniform_int_distribution<size_t> pageFaultDistribution;
		std::geometric_distribution<std::milliseconds::rep> waitTimeDistribution;

	public:
		/**
		 * @tparam Duration Any specialization type of std::chrono::duration
		 */
		template < class Duration >
		BlockmemoryPageAccessInjector( Duration const& meanTimeBetweenErrors ) :
			randomNumberGenerator(),
			waitTimeDistribution( 1 / std::chrono::duration_cast<std::chrono::milliseconds>(meanTimeBetweenErrors).count() )
		{}

		std::chrono::milliseconds getWaitTime() {
			waitTimeDistribution( randomNumberGenerator );
		}

		void injectError() {
			size_t position = pageFaultDistribution( randomNumberGenerator );

			if( position > 0 ) {
				for( auto it = candidatePages.begin(); it != candidatePages.end() && position >= it->getSize(); it++ ) {
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
