#ifndef REGIONCACHE_HPP
#define REGIONCACHE_HPP

#include <stdio.h>
#include "regioncache_decl.hpp"
#include "processingelement_decl.hpp"


inline uint64_t Chunk::getAddress() const {
   return _address;
}

inline uint64_t Chunk::getHostAddress() const {
   return _hostAddress;
}

inline void Chunk::setHostAddress( uint64_t addr ) {
   _hostAddress = addr;
}

inline std::size_t Chunk::getSize() const {
   return _size;
}

inline void AllocatedChunk::addReference( int wdId, unsigned int loc ) {
   _refs++;
   _refWdId[wdId]++;
   _refLoc[wdId].insert(loc);
   //std::cerr << "add ref to chunk "<< (void*)this << " " << _refs.value() << std::endl;
}

inline void AllocatedChunk::removeReference( int wdId ) {
   _refs--;
   _refWdId[wdId]--;
   if ( _refWdId[wdId] == 0 ) {
      _refLoc[wdId].clear();
   }
   
   //std::cerr << "del ref to chunk "<< (void*)this << " " << _refs.value() << std::endl;
   //if ( _refs == (unsigned int) -1 ) {
   //   std::cerr << "overflow at references chunk "<< (void*)this << std::endl; sys.printBt();
   //} else if ( _refs == 0 ) {
   //   std::cerr << "zeroed at references chunk "<< (void*)this << std::endl;
   //}
   
}

inline unsigned int AllocatedChunk::getReferenceCount() const { return _refs.value(); }

inline bool AllocatedChunk::isDirty() const {
   return _dirty;
}

inline unsigned int AllocatedChunk::getLruStamp() const {
   return _lruStamp;
}

inline void AllocatedChunk::increaseLruStamp() {
   _lruStamp += 1;
}

inline global_reg_t AllocatedChunk::getAllocatedRegion() const {
   return _allocatedRegion;
}

inline bool AllocatedChunk::isRooted() const {
   return _rooted;
}

inline Device /*const*/ &RegionCache::getDevice() const {
   return _device;
}

inline bool RegionCache::canCopyFrom( RegionCache const &from ) const {
   return _device == from._device;
}

inline unsigned int RegionCache::getLruTime() const {
   return _lruTime;
}

inline void RegionCache::increaseLruTime() {
   _lruTime += 1;
}

inline unsigned int RegionCache::getSoftInvalidationCount() const {
   return _softInvalidationCount.value();
}

inline unsigned int RegionCache::getHardInvalidationCount() const {
   return _hardInvalidationCount.value();
}


#endif /* REGIONCACHE_HPP */
