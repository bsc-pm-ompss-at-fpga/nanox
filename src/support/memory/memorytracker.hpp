
#ifndef MEMORY_TRACKER_HPP
#define MEMORY_TRACKER_HPP

#include "memorychunk.hpp"

#include <list>

template <typename ChunkType>
class MemoryTracker {
	private:
		using ChunkList = std::list<ChunkType>;
		using iterator = typename ChunkList::iterator;

		size_t _totalSize;
		ChunkList _listOfChunks;

	public:
		MemoryTracker() :
			_listOfChunks()
		{
		}

		MemoryTracker( std::initializer_list<ChunkType> startingListOfChunks ) :
			_listOfChunks( startingListOfChunks )
		{
		}

		void insert( MemoryChunk const& chunk )
		{
			_listOfChunks.push_back( chunk );
			_totalSize += _listOfChunks.back().size();
		}

		template< typename... Args>
		void emplace( Args... args )
		{
			_listOfChunks.emplace_back( args... );
			_totalSize += _listOfChunks.back().getSize();
		}

		void erase( iterator position )
		{
			_totalSize -= position->size();
			_listOfChunks.erase( position );
		}

		size_t getTotalSize() const { return _totalSize; }

		bool isEmpty() const { return _listOfChunks.empty(); }

		iterator begin() const { return _listOfChunks.begin(); }

		iterator end() const { return _listOfChunks.begin(); }

		ChunkType const& at( unsigned position ) const { return _listOfChunks.at(position); }
};

#endif // MEMORY_TRACKER_HPP

