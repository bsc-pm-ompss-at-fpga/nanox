#ifndef MEMCACHECOPY_HPP
#define MEMCACHECOPY_HPP

#include "memcachecopy_decl.hpp"
#include "system_decl.hpp"
#include "memoryops_decl.hpp"
#include "deviceops.hpp"
#include "workdescriptor.hpp"
#include "addressspace.hpp"
#include "basethread.hpp"

namespace nanos {

inline MemCacheCopy::MemCacheCopy() : 
   _version( 0 ), _childrenProducedVersion( 0 )
   , _reg( 0, (reg_key_t) NULL )
   , _locations()
   , _locationDataReady( false )
   , _chunk( NULL )
   , _policy( sys.getRegionCachePolicy() )
   , _invalControl()
   , _allocFrom( -1 )
   , _regionsToCommit()
{
}

inline MemCacheCopy::MemCacheCopy( WD const &wd, unsigned int index/*, MemController &ccontrol*/ ) : 
   _version( 0 ), _childrenProducedVersion( 0 )
   , _reg( 0, (reg_key_t) NULL )
   , _locations()
   , _locationDataReady( false )
   , _chunk( NULL )
   , _policy( sys.getRegionCachePolicy() )
   , _invalControl()
   , _allocFrom( -1 )
   , _regionsToCommit()
{
   // Store region id into _reg
   sys.getHostMemory().getRegionId( wd.getCopies()[ index ], _reg, wd, index );

   // PreInit _reg
   _reg.id = _reg.key->obtainRegionId( wd.getCopies()[index], wd, index );

   NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) _reg.key->getRegionData( _reg.id );
   if ( entry == NULL ) {
      _reg.key->setRegionData( _reg.id, NEW NewNewDirectoryEntryData() );
   }
}


inline void MemCacheCopy::getVersionInfo() {
   if ( _version == 0 ) {
      unsigned int ver = 0;
      sys.getHostMemory().getVersionInfo( _reg, ver, _locations );
      setVersion( ver );
      _locationDataReady = true;
   }
}

inline void MemCacheCopy::generateOutOps( SeparateMemoryAddressSpace *from, SeparateAddressSpaceOutOps &ops, bool input, bool output, WD const &wd, unsigned int copyIdx ) {
   if ( ops.getPE()->getMemorySpaceId() != 0 ) {
      if ( _policy == RegionCache::FPGA ) { //emit copy for all data
         if ( output ) {
            _chunk->copyRegionToHost( ops, _reg.id, _version + (output ? 1 : 0), wd, copyIdx );
         }
      } else {
         if ( output ) {
            if ( _policy != RegionCache::WRITE_BACK ) {
               _chunk->copyRegionToHost( ops, _reg.id, _version + 1, wd, copyIdx );
            }
         }
      }
   }
}

inline unsigned int MemCacheCopy::getVersion() const {
   return _version;
}

inline void MemCacheCopy::setVersion( unsigned int version ) {
   _version = version;
}

inline bool MemCacheCopy::isRooted( memory_space_id_t &loc ) const {
   //bool result;
   //bool result2;
   bool result3;

   global_reg_t whole_obj( _reg.id, _reg.key );
   result3 = whole_obj.isRooted();
   if ( result3 ) {
      loc = whole_obj.getRootedLocation();
   }

    //if ( _locations.size() > 0 ) {
    //   global_reg_t refReg( _locations.begin()->second, _reg.key );
    //   result = true;
    //   result2 = true;
    //   memory_space_id_t refloc = refReg.getFirstLocation();
    //   for ( NewLocationInfoList::const_iterator it = _locations.begin(); it != _locations.end() && result; it++ ) {
    //      global_reg_t thisReg( it->second, _reg.key );
    //      result = ( thisReg.isRooted() && thisReg.getFirstLocation() == refloc );
    //      result2 = ( thisReg.isRooted() && thisReg.getRootedLocation() == refloc );
    //   }
    //   if ( result ) loc = refloc;
    //} else {
    //   result = _reg.isRooted();
    //   if ( result ) {
    //      loc = _reg.getFirstLocation();
    //   }
    //}
   //if ( sys.getNetwork()->getNodeNum() == 0 ) {
   //   std::cerr << "whole object "; whole_obj.key->printRegion(std::cerr, whole_obj.id);
   //   std::cerr << std::endl << "real reg "; _reg.key->printRegion(std::cerr, _reg.id);
   //   std::cerr << std::endl << " copy root check result: " << result << " result2: " << result2 << " result3: " << result3 << std::endl;
   //}
   return result3;
}

inline void MemCacheCopy::setChildrenProducedVersion( unsigned int version ) {
   _childrenProducedVersion = version;
}

inline unsigned int MemCacheCopy::getChildrenProducedVersion() const {
   return _childrenProducedVersion;
}

inline void MemCacheCopy::printLocations( std::ostream& os ) const {
   typedef NewNewDirectoryEntryData DirData;
   typedef NewLocationInfoList      LocationList;

   LocationList::const_iterator it;
   for ( it = _locations.begin(); it != _locations.end(); it++ ) {
      GlobalRegionDictionary& dict = *_reg.key;

      const DirData* region_src = static_cast<const DirData*>(dict.getRegionData( it->first ));
      const DirData* data_src   = static_cast<const DirData*>(dict.getRegionData( it->second ));

      os << " [ " << it->first << "," << it->second << " ]";
      if( region_src )
         _reg.key->printRegion( os, it->first );
      else
         os << "RS dir entry n/a " << std::endl;
      if( data_src )
         _reg.key->printRegion( os, it->first );
      else
         os << "DS dir entry n/a " << std::endl;
   }
}

inline std::ostream& operator<<( std::ostream& os, const MemCacheCopy& cacheCopy )
{
   cacheCopy.printLocations(os);
   return os;
}

} // namespace nanos

#endif
