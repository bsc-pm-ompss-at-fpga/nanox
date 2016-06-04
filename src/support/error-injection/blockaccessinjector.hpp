
#ifndef BLOCK_ACCESS_INJECTOR_HPP
#define BLOCK_ACCESS_INJECTOR_HPP

#include "error-injection/errorinjectionconfiguration.hpp"
#include "error-injection/errorinjectionpolicy.hpp"
#include "exception/failurestats.hpp"
#include "memory/memorypage.hpp"
#include "memory/blockedpage.hpp"

#include <deque>
#include <random>

namespace nanos {
namespace error {

using namespace memory;

template < typename RandomEngine = std::minstd_rand >
class BlockMemoryPageAccessInjector : public ErrorInjectionPolicy
{
	private:
		std::deque<std::pair<BlockableAccessMemoryPage, bool> > _pages;
		RandomEngine                          _generator;

	public:
		BlockMemoryPageAccessInjector( ErrorInjectionConfig const& properties ) noexcept :
			ErrorInjectionPolicy( properties ),
			_pages(),
			_generator( properties.getInjectionSeed() )
		{
		}

		virtual ~BlockMemoryPageAccessInjector()
		{
		}

		RandomEngine& getRandomGenerator() { return _generator; }

		void injectError( std::pair<BlockableAccessMemoryPage,bool>& page )
		{
			if( !page.second && !page.first.isBlocked() ) {
				page.second = true;
				page.first.blockAccess();
			}
		}

		virtual void injectError()
		{
			using distribution = std::uniform_int_distribution<size_t>;
			using dist_param = distribution::param_type;

			static distribution pageFaultDistribution;
			
			if( !_pages.empty() ) {
				dist_param parameter( 0, _pages.size()-1 );
				size_t position = pageFaultDistribution( _generator, parameter );
				injectError( _pages[position] );
			}
			FailureStats<ErrorInjection>::increase();
		}

		virtual void injectError( void *handle )
		{
			Address address( handle );

			for( std::pair<BlockableAccessMemoryPage,bool>& page : _pages ) {
				if( page.first.contains( address ) ) {
					injectError( page );
					return;
				}
			}
			// This address has not been registered before. Insert it.
			_pages.emplace_back( BlockableAccessMemoryPage(address, block_page_at_creation), true );
		}

		virtual void declareResource( void *address, size_t size )
		{
			std::vector<MemoryPage> pagesInsideChunk =
				MemoryPage::getPagesInsideChunk( memory::MemoryChunk( address, size ) );
			for( const MemoryPage& page : pagesInsideChunk ) {
				_pages.emplace_back( BlockableAccessMemoryPage(page, block_page_manually), false );
			}
		}

		void insertCandidatePage( MemoryPage const& page )
		{
			_pages.emplace_back( BlockableAccessMemoryPage(page, block_page_manually), false );
		}

		virtual void recoverError( void* handle ) noexcept {
			Address failedAddress( handle );

			for( std::pair<BlockableAccessMemoryPage,bool>& page : _pages ) {
				if( page.first.contains( failedAddress ) ) {
					page.second = true;
					page.first.unblockAccess();
					return;
				}
			}
		}
};

} // namespace error
} // namespace nanos

#endif // BLOCK_ACCESS_INJECTOR_HPP

