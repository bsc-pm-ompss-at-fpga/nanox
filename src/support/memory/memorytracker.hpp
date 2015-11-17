
#ifndef MEMORY_TRACKER_HPP
#define MEMORY_TRACKER_HPP

#include "memorychunk.hpp"

template <typename ChunkType>
class MemoryTracker {
	private:
		using ChunkList = std::list<ChunkType>;

		size_t _totalSize;
		ChunkList _listOfChunks;

	public:
		MemoryTracker( std::initializer_list<ChunkType> startingListOfChunks ) :
			_listOfChunks( startingListOfChunks )
		{
		}

		void insert( MemoryChunk const& chunk ) {
			_listOfChunks.push_back( chunk );
			_totalSize += _listOfChunks.back().getSize();
		}

		size_t getTotalSize() const { return _totalSize; }

		ChunkList const& getChunks() const { return _listOfChunks; }

		ChunkType const& at( unsigned position ) const { return _listOfChunks.at(position); }
};

#endif // MEMORY_TRACKER_HPP

