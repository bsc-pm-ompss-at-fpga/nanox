#ifndef CACHEDREGIONSTATUS_HPP
#define CACHEDREGIONSTATUS_HPP
#include "cachedregionstatus_decl.hpp"
#include "version.hpp"
using namespace nanos; 

inline CachedRegionStatus::CachedRegionStatus() : Version(), _ops(), _dirty( false ), _valid(true) {
}

inline CachedRegionStatus::CachedRegionStatus( CachedRegionStatus const &rs ) : Version( rs ), _ops ( ), _dirty( rs._dirty ), _valid( rs._valid ) {
}

inline DeviceOps *CachedRegionStatus::getDeviceOps() {
   return &_ops;
}

inline CachedRegionStatus &CachedRegionStatus::operator=( CachedRegionStatus const &rs ) {
   Version::operator=(rs);
   _dirty = rs._dirty;
   _valid = rs._valid;
   return *this;
}

inline CachedRegionStatus::CachedRegionStatus( CachedRegionStatus &rs ) : Version( rs ), _ops (), _dirty( rs._dirty ), _valid( rs._valid ) {
}

inline CachedRegionStatus &CachedRegionStatus::operator=( CachedRegionStatus &rs ) {
   Version::operator=(rs);
   _dirty = rs._dirty;
   return *this;
}

inline bool CachedRegionStatus::isDirty() const {
   return _dirty;
}
inline void CachedRegionStatus::setDirty() {
   _dirty = true;
}
inline void CachedRegionStatus::clearDirty() {
   _dirty = false;
}

inline bool CachedRegionStatus::isValid() const {
   return _valid;
}
inline void CachedRegionStatus::setValid( bool flag ) {
   _valid = flag;
}

inline void CachedRegionStatus::resetVersion() {
   Version::resetVersion();
}

#endif /* CACHEDREGIONSTATUS_HPP */
