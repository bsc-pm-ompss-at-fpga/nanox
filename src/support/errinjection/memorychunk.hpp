
#include "memoryaddress.hpp"

constexpr
bool is_properly_aligned( Address address, size_t alignment_constraint )
{
   return ( reinterpret_cast<uintptr_t>(address.data()) & (alignment_constraint-1) ) == 0;
}
	
class MemoryChunk {
   private:
      Address baseAddress;
      size_t length;
   public:
      constexpr
      MemoryChunk( Address const& base, size_t chunkLength ) :
            baseAddress( base ), length( chunkLength )
      {
      }

      constexpr
      MemoryChunk( Address const& base, Address const& end ) :
            baseAddress( base ), length( end - base )
      {
      }

      constexpr
      Address getBaseAddress() const { return baseAddress; }

      constexpr
      Address begin() const { return baseAddress; }

      constexpr
      Address end() const { return baseAddress+length; }

      constexpr
      size_t getSize() const { return length; }
};

template <size_t alignment_restriction>
class AlignedMemoryChunk : public MemoryChunk {
   public:
      constexpr
      AlignedMemoryChunk( Address const& baseAddress, size_t length ) :
            MemoryChunk( baseAddress, length )
      {
          static_assert( is_properly_aligned( baseAddress, alignment_restriction ),
                                  "Provided address is not properly aligned." );
      }

      constexpr
      AlignedMemoryChunk( Address const& baseAddress, Address const& endAddress ) :
            MemoryChunk( baseAddress, endAddress )
      {
          static_assert( is_properly_aligned( baseAddress, alignment_restriction ),
                                  "Provided address is not properly aligned." );
      }

      template<class ChunkType>
      constexpr
      AlignedMemoryChunk( ChunkType const& chunk ) :
            MemoryChunk(
                     chunk.begin().align( alignment_restriction ) + alignment_restriction,
                     chunk.end().align( alignment_restriction )
                  )
      {
      }
};

// FIXME: change literal page size by macro computed by autoconf
using MemoryPage = AlignedMemoryChunk<4096>;

