
#include "memorychunk.hpp"

template <typedef ChunkType>
class MemoryTracker {
	private:
		size_t totalSize;
		std::list<ChunkType> chunkList;

	public:
		MemoryTracker( std::initializer_list<ChunkType> startingListOfChunks ) :
			chunkList( startingListOfChunks )
		{
		}

		void addMemoryChunk( MemoryChunk const& chunk ) {
			chunkList.push_back( chunk );
			totalSize += chunkList.back().getSize()
		}

		size_t getTotalSize() const { return totalSize; }

		std::deque<ChunkType> const& getChunks() const { return chunkList; }

		ChunkType const& at( unsigned position ) const { return chunkList.at(position); }
};

