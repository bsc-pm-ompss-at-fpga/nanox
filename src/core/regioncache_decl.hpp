/*************************************************************************************/
/*      Copyright 2009 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#ifndef _NANOS_REGION_CACHE_H
#define _NANOS_REGION_CACHE_H

#include "functor_decl.hpp"
#include "memorymap_decl.hpp"
#include "region_decl.hpp"
#include "copydata_decl.hpp"
#include "atomic_decl.hpp"
#include "workdescriptor_fwd.hpp"
#include "processingelement_fwd.hpp"
#include "deviceops_decl.hpp"
#include "regiondirectory_decl.hpp"
#include "newregiondirectory_decl.hpp"
#include "memoryops_fwd.hpp"

namespace nanos {
#if 0

   Separate::hasAllData() {
      _regiocnCache->NewAddRead/write();
   }
   
   Separate::hasAllData() {
      _regiocnCache->NewAddRead/write();
   }

   class Chunk {
      copy( AddressSpace from, global_reg_t reg );
      lockFailure();
      hasAllData();
   }
   
  HostMemChunk::copy( reg, from ) {
     from.tryLock( reg )
     if ( succeeded ) {
        from.giveData( reg );
        from.unlock( reg );
     } else {
        waitForData( reg );
     }
  }

  //separate addr space
  SeparateMemChunk::copy( reg, from ) {
     origChunk = from.tryLock( reg )
     if ( succeeded ) {
        origChunk.giveData( reg );
        origChunk.unlock( reg );
     } else {
        origChunk = hostMem.tryLock( reg );
        origChunk.giveDataWhenReady( reg );
        origChunk.unlock();
     }
  }

   WD::copyDataIn() {
      for (unsigned int index = 0; index < _numCopies; index++ ) {
         if (! versionInfo ) hostMem.getVersionInfo( reg, ver, locs );

         chunk = myMem.getMyChuk( reg );
        
         if ( !chunk.hasAllData( reg, ver ) ) {
            for each region fragment
               chunk.copy( regFragment, from ); 
         } else {
            // good 2 go
         }
      }
   }


   class HostAddressSpace : public BaseAddressSpace {
      NewRegionDirectory _dir;

      getVersionInfo( global_reg_t reg, parts... ); //getLocation
   }

   class SeparateAddressSpace : public BaseAddressSpace {
      RegionCache _cache; // map reg -> chunk
      HostAddressSpace &_hostMem;
      synchronize( global_reg_t );
   }
#endif

   class RegionCache;

   class AllocatedChunk {
      private:
         RegionCache                      &_owner;
         Lock                              _lock;
         uint64_t                          _address;
         uint64_t                          _hostAddress;
         std::size_t                       _size;
         bool                              _dirty;
         bool                              _invalidated;
         unsigned int                      _lruStamp;
         std::size_t                       _roBytes;
         std::size_t                       _rwBytes;
         Atomic<unsigned int>              _refs;
         global_reg_t                      _allocatedRegion;
         
         RegionTree< CachedRegionStatus > *_regions;

         CacheRegionDictionary *_newRegions;

      public:
         static Atomic<int> numCall;
         //AllocatedChunk( );
         AllocatedChunk( RegionCache &owner, uint64_t addr, uint64_t hostAddr, std::size_t size, global_reg_t const &allocatedRegion );
         AllocatedChunk( AllocatedChunk const &chunk );
         AllocatedChunk &operator=( AllocatedChunk const &chunk );
         ~AllocatedChunk();

         uint64_t getAddress() const;
         uint64_t getHostAddress() const;
         std::size_t getSize() const;
         bool isDirty() const;
         unsigned int getLruStamp() const;
         void increaseLruStamp();
         void setHostAddress( uint64_t addr );

         void addReadRegion( Region const &reg, unsigned int version, std::set< DeviceOps * > &currentOps, std::list< Region > &notPresentRegions, DeviceOps *ops, bool alsoWriteReg );
         void addWriteRegion( Region const &reg, unsigned int version );
         void clearRegions();
         void clearNewRegions( global_reg_t const &newAllocatedRegion );
         RegionTree< CachedRegionStatus > *getRegions();
         CacheRegionDictionary *getNewRegions();
         bool isReady( Region reg );
         bool isInvalidated() const;
         void invalidate(RegionCache *targetCache, WD const &wd );

         void lock();
         void unlock();
         void NEWaddReadRegion( reg_t reg, unsigned int version, std::set< DeviceOps * > &currentOps, std::list< reg_t > &notPresentRegions, DeviceOps *ops, bool alsoWriteReg );
         bool NEWaddReadRegion2( BaseAddressSpaceInOps &ops, reg_t reg, unsigned int version, std::set< DeviceOps * > &currentOps, std::set< reg_t > &notPresentRegions, std::set<DeviceOps *> &thisRegOps, bool output, NewLocationInfoList const &locations );
         void NEWaddWriteRegion( reg_t reg, unsigned int version );
         void addReference();
         void removeReference();
         unsigned int getReferenceCount() const;
         void confirmCopyIn( reg_t id, unsigned int version );
         unsigned int getVersion( global_reg_t const &reg );
         unsigned int getVersionSetVersion( global_reg_t const &reg, unsigned int newVersion );

         DeviceOps *getDeviceOps( global_reg_t const &reg );
         void prepareRegion( reg_t reg, unsigned int version );
   };

   class CompleteOpFunctor : public Functor {
      private:
         DeviceOps *_ops;
         AllocatedChunk *_chunk;
      public:
         CompleteOpFunctor( DeviceOps *ops, AllocatedChunk *_chunk );
         virtual ~CompleteOpFunctor();
         virtual void operator()();
   };

   class CacheCopy;
   
   class RegionCache {
      public:
         enum CacheOptions {
            ALLOC_FIT,
            ALLOC_WIDE
         };
      private:
         MemoryMap<AllocatedChunk>  _chunks;
         Lock                       _lock;
         Device                    &_device;
         //ProcessingElement         &_pe;
         memory_space_id_t          _memorySpaceId;
         CacheOptions               _flags;
         unsigned int               _lruTime;

         typedef MemoryMap<AllocatedChunk>::MemChunkList ChunkList;
         typedef MemoryMap<AllocatedChunk>::ConstMemChunkList ConstChunkList;

         class Op {
               RegionCache &_parent;
               std::string _name;
            public:
               Op( RegionCache &parent, std::string name ) : _parent ( parent ), _name ( name ) { }
               RegionCache &getParent() const { return _parent; }
               std::string const &getStr() { return _name; }
               virtual void doNoStrided( int dataLocation, uint64_t devAddr, uint64_t hostAddr, std::size_t size, DeviceOps *ops, WD const &wd, bool fake ) = 0;
               virtual void doStrided( int dataLocation, uint64_t devAddr, uint64_t hostAddr, std::size_t size, std::size_t count, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake ) = 0;
         };

         class CopyIn : public Op {
            public:
               CopyIn( RegionCache &parent ) : Op( parent, "CopyIn" ) {}
               void doNoStrided( int dataLocation, uint64_t devAddr, uint64_t hostAddr, std::size_t size, DeviceOps *ops, WD const &wd, bool fake ) ;
               void doStrided( int dataLocation, uint64_t devAddr, uint64_t hostAddr, std::size_t size, std::size_t count, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake ) ;
         } _copyInObj;

         class CopyOut : public Op {
            public:
               CopyOut( RegionCache &parent ) : Op( parent, "CopyOut" ) {}
               void doNoStrided( int dataLocation, uint64_t devAddr, uint64_t hostAddr, std::size_t size, DeviceOps *ops, WD const &wd, bool fake ) ;
               void doStrided( int dataLocation, uint64_t devAddr, uint64_t hostAddr, std::size_t size, std::size_t count, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake ) ;
         } _copyOutObj;

         void doOp( Op *opObj, Region const &hostMem, uint64_t devBaseAddr, unsigned int location, DeviceOps *ops, WD const &wd ); 
         void doOp( Op *opObj, global_reg_t const &hostMem, uint64_t devBaseAddr, unsigned int location, DeviceOps *ops, WD const &wd ); 
         //void _generateRegionOps( Region const &reg, std::map< uintptr_t, MemoryMap< uint64_t > * > &opMap );

      public:
         RegionCache( memory_space_id_t memorySpaceId, Device &cacheArch, enum CacheOptions flags );
         AllocatedChunk *getAddress( global_reg_t const &reg, RegionTree< CachedRegionStatus > *&regsToInvalidate, CacheRegionDictionary *&newRegsToInvalidate, WD const &wd );
         AllocatedChunk *getAllocatedChunk( global_reg_t const &reg ) const;
         AllocatedChunk *getAddress( uint64_t hostAddr, std::size_t len );
         AllocatedChunk **selectChunkToInvalidate( /*CopyData const &cd, uint64_t addr,*/ std::size_t allocSize/*, RegionTree< CachedRegionStatus > *&regsToInval, CacheRegionDictionary *&newRegsToInval*/ );
         void syncRegion( Region const &r ) ;
         void syncRegion( std::list< std::pair< Region, CacheCopy * > > const &regions, WD const &wd ) ;
         void syncRegion( global_reg_t const &r ) ;
         void syncRegion( std::list< std::pair< global_reg_t, CacheCopy * > > const &regions, WD const &wd ) ;
         unsigned int getMemorySpaceId();
         /* device stubs */
         void _copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len, DeviceOps *ops, WD const &wd, bool fake );
         void _copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len, DeviceOps *ops, WD const &wd, bool fake );
         void _syncAndCopyIn( memory_space_id_t syncFrom, uint64_t devAddr, uint64_t hostAddr, std::size_t len, DeviceOps *ops, WD const &wd, bool fake );
         void _copyDevToDev( memory_space_id_t copyFrom, uint64_t devAddr, uint64_t hostAddr, std::size_t len, DeviceOps *ops, WD const &wd, bool fake );
         void _copyInStrided1D( uint64_t devAddr, uint64_t hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake );
         void _copyOutStrided1D( uint64_t hostAddr, uint64_t devAddr, std::size_t len, std::size_t numChunks, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake );
         void _syncAndCopyInStrided1D( memory_space_id_t syncFrom, uint64_t devAddr, uint64_t hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake );
         void _copyDevToDevStrided1D( memory_space_id_t copyFrom, uint64_t devAddr, uint64_t hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, DeviceOps *ops, WD const &wd, bool fake );
         /* *********** */
         void copyIn( Region const &hostMem, uint64_t devBaseAddr, unsigned int location, DeviceOps *ops, WD const &wd ); 
         void copyOut( Region const &hostMem, uint64_t devBaseAddr, DeviceOps *ops, WD const &wd ); 
         void copyIn( global_reg_t const &hostMem, uint64_t devBaseAddr, unsigned int location, DeviceOps *ops, WD const &wd ); 
         void copyOut( global_reg_t const &hostMem, uint64_t devBaseAddr, DeviceOps *ops, WD const &wd ); 
         void NEWcopyIn( unsigned int location, global_reg_t const &hostMem, unsigned int version, WD const &wd ); 
         void NEWcopyOut( global_reg_t const &hostMem, unsigned int version, WD const &wd ); 
         uint64_t getDeviceAddress( global_reg_t const &reg, uint64_t baseAddress ) const;
         void lock();
         void unlock();
         bool tryLock();
         bool canCopyFrom( RegionCache const &from ) const;
         Device const &getDevice() const;
         unsigned int getNodeNumber() const;
         unsigned int getLruTime() const;
         void increaseLruTime();
         bool pin( global_reg_t const &hostMem );
         void unpin( global_reg_t const &hostMem );

         //unsigned int getVersionAllocateChunkIfNeeded( global_reg_t const &hostMem, bool increaseVersion );
         unsigned int getVersionSetVersion( global_reg_t const &hostMem, unsigned int newVersion );
         unsigned int getVersion( global_reg_t const &hostMem );
         void releaseRegion( global_reg_t const &hostMem );
         void prepareRegion( global_reg_t const &hostMem, WD const &wd );
         void setRegionVersion( global_reg_t const &hostMem, unsigned int version );

         void copyInputData( BaseAddressSpaceInOps &ops, global_reg_t const &reg, unsigned int version, bool output, NewLocationInfoList const &locations );
         void allocateOutputMemory( global_reg_t const &reg, unsigned int version );
   };

   class CacheController;
   class CacheCopy {

      private:
         CopyData const &_copy;
         AllocatedChunk *_cacheEntry;
         std::list< std::pair<Region, CachedRegionStatus const &> > _cacheDataStatus;
         Region _region;
         uint64_t _offset;
         unsigned int _version;
         unsigned int _newVersion;
         NewRegionDirectory::LocationInfoList _locations;
         NewLocationInfoList _newLocations;
         DeviceOps _operations;
         std::set< DeviceOps * > _otherPendingOps;
         reg_t _regId;

      public:
         global_reg_t _reg;
         CacheCopy();
         CacheCopy( WD const &wd, unsigned int index, CacheController &ccontrol );
         CacheCopy( WD const &wd, unsigned int index );
         
         bool isReady();
         void setUpDeviceAddress( RegionCache *targetCache, NewRegionDirectory *dir );
         void generateCopyInOps( RegionCache *targetCache, std::map<unsigned int, std::list< std::pair< Region, CacheCopy * > > > &opsBySourceRegions ) ;
         void NEWgenerateCopyInOps( RegionCache *targetCache, std::map<unsigned int, std::list< std::pair< global_reg_t, CacheCopy * > > > &opsBySourceRegions ) ;
         bool tryGetLocation( WD const &wd, unsigned int index );
         void copyDataOut( RegionCache *targetCache );

         bool tryGetLocationNewInit( WD const &wd, unsigned int copyIndex );
         void getVersionInfo( WD const &wd, unsigned int copyIndex, CacheController &ccontrol );

         NewRegionDirectory::LocationInfoList const &getLocations() const;
         NewLocationInfoList const &getNewLocations() const;
         uint64_t getDeviceAddress() const;
         DeviceOps *getOperations();
         Region const &getRegion() const;
         unsigned int getVersion() const;
         unsigned int getNewVersion() const;
         void confirmCopyIn( unsigned int memorySpaceId );
         CopyData const & getCopyData() const;
         reg_t getRegId() const;
         NewNewRegionDirectory::RegionDirectoryKey getRegionDirectoryKey() const;
   };


   class CacheController {

      private:
         WD const &_wd;
         unsigned int _numCopies;
         CacheCopy *_cacheCopies;
         RegionCache *_targetCache;  
         bool _registered;
         Lock _provideLock;
         std::map< NewNewRegionDirectory::RegionDirectoryKey, std::map< reg_t, unsigned int > > _providedRegions;

      public:
         CacheController();
         CacheController( WD const &wd );
         ~CacheController();
         bool isCreated() const;
         void preInit( );
         void copyDataIn( RegionCache *targetCache );
         bool dataIsReady() ;
         uint64_t getAddress( unsigned int copyIndex ) const;
         void copyDataOut();
         void getInfoFromPredecessor( CacheController const &predecessorController );
         bool hasVersionInfoForRegion( global_reg_t reg, unsigned int &version, NewLocationInfoList &locations ) ;

         CacheCopy *getCacheCopies() const;
         RegionCache *getTargetCache() const;
   };
}

#endif
