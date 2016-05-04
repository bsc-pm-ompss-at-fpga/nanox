/*************************************************************************************/
/*      Copyright 2015 Barcelona Supercomputing Center                               */
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

#include "xstring.hpp"
#include "memcontroller.hpp"
#include "workdescriptor.hpp"
#include "regiondict.hpp"
#include "newregiondirectory.hpp"
#include "memcachecopy.hpp"
#include "globalregt.hpp"

#include "cachedregionstatus.hpp"

#ifdef NANOS_RESILIENCY_ENABLED
#   include "backupmanager.hpp"
#   include "backupprivatecopy.hpp"
#endif

#include "debug.hpp"

#if VERBOSE_CACHE
 #define _VERBOSE_CACHE 1
#else
 #define _VERBOSE_CACHE 0
 //#define _VERBOSE_CACHE ( sys.getNetwork()->getNodeNum() == 0 )
#endif

namespace nanos {
MemController::MemController( WD &wd ) : 
   _initialized( false )
   , _preinitialized( false )
   , _inputDataReady( false )
   , _outputDataReady( false )
   , _memoryAllocated( false )
   , _invalidating( false )
   , _mainWd( false )
   , _wd( wd )
   , _pe( NULL )
   , _provideLock()
   , _providedRegions()
   , _inOps( NULL )
   , _outOps( NULL )
#ifdef NANOS_RESILIENCY_ENABLED
   , _backupOpsIn()
   , _backupOpsOut()
   , _restoreOps()
   , _backupCacheCopies()
   , _backupInOutCopies()
#endif
   , _affinityScore( 0 )
   , _maxAffinityScore( 0 )
   , _ownedRegions()
   , _parentRegions()
   , _memCacheCopies()
{
   if ( _wd.getNumCopies() > 0 ) {
      _memCacheCopies = NEW MemCacheCopy[ wd.getNumCopies() ];
#ifdef NANOS_RESILIENCY_ENABLED
      if( sys.isResiliencyEnabled() && wd.isRecoverable() ) {
			_backupCacheCopies.reserve( wd.getNumCopies() );
			_backupInOutCopies.reserve( wd.getNumCopies() );
      }
#endif
   }
}

MemController::~MemController() {
   delete _inOps;
   delete _outOps;
   delete[] _memCacheCopies;
   if( _backupOpsIn )
      delete _backupOpsIn;
   if( _backupOpsOut )
      delete _backupOpsOut;
}

bool MemController::ownsRegion( global_reg_t const &reg ) {
   bool i_has_it = _ownedRegions.hasObjectOfRegion( reg );
   bool parent_has_it  = _parentRegions.hasObjectOfRegion( reg );
   return i_has_it || parent_has_it;
}

void MemController::preInit( ) {
   unsigned int index;
   if ( _preinitialized ) return;

   // std::set<reg_key_t> dicts;
   for ( index = 0; index < _wd.getNumCopies(); index += 1 ) {
      new ( &_memCacheCopies[ index ] ) MemCacheCopy( _wd, index );
   //    dicts.insert( _memCacheCopies[ index ]._reg.key );
   }
             for ( index = 0; index < _wd.getNumCopies(); index += 1 ) {
                _memCacheCopies[ index ]._reg.id = _memCacheCopies[ index ]._reg.key->obtainRegionId( _wd.getCopies()[index], _wd, index );
                NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) _memCacheCopies[ index ]._reg.key->getRegionData( _memCacheCopies[ index ]._reg.id );
                if ( entry == NULL ) {
                   entry = NEW NewNewDirectoryEntryData();
                   _memCacheCopies[ index ]._reg.key->setRegionData( _memCacheCopies[ index ]._reg.id, entry ); //preInit memCacheCopy._reg
                }
   for ( index = 0; index < _wd.getNumCopies(); index += 1 ) {
      if ( _wd.getParent() != NULL /* && !_wd.getParent()->_mcontrol._mainWd */ ) {
         for ( unsigned int parent_idx = 0; parent_idx < _wd.getParent()->getNumCopies(); parent_idx += 1 ) {
            if ( _wd.getParent()->_mcontrol.getAddress( parent_idx ) == (memory::Address) _wd.getCopies()[ index ].getBaseAddress() ) {
               memory::Address host_copy_addr = (memory::Address) _wd.getParent()->getCopies()[ parent_idx ].getHostBaseAddress();
               _wd.getCopies()[ index ].setHostBaseAddress( host_copy_addr );
            }
         }
      }
   }

   for ( index = 0; index < _wd.getNumCopies(); index += 1 ) {

      if ( sys.usePredecessorCopyInfo() ) {
         unsigned int predecessorsVersion;
         if ( _providedRegions.hasVersionInfoForRegion( _memCacheCopies[ index ]._reg, predecessorsVersion, _memCacheCopies[ index ]._locations ) ) {
            _memCacheCopies[ index ].setVersion( predecessorsVersion );
         }
      }
      if ( _memCacheCopies[ index ].getVersion() != 0 ) {
         _memCacheCopies[ index ]._locationDataReady = true;
      } else {
         _memCacheCopies[ index ].getVersionInfo();
      }

      if ( _wd.getParent() != NULL && _wd.getParent()->_mcontrol.ownsRegion( _memCacheCopies[ index ]._reg ) ) {
         /* do nothing, maybe here we can add a correctness check,
          * to ensure that the region is a subset of the Parent regions
          */
         _parentRegions.addRegion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ].getVersion() );
      } else { /* this should be for private data */
         if ( _wd.getParent() != NULL ) {
            _wd.getParent()->_mcontrol._ownedRegions.addRegion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ].getVersion() );
         }
      }
   }

#ifdef NANOS_RESILIENCY_ENABLED
   if( sys.isResiliencyEnabled() && _wd.isRecoverable() ) {
      for ( index = 0; index < _memCacheCopies.size(); index ++ ) {
            _backupCacheCopies.emplace_back( _wd.getCopies()[index], _memCacheCopies[index], _wd, index );
      }
   }
#endif

   for ( index = 0; index < _wd.getNumCopies(); index += 1 ) {
      std::list< std::pair< reg_t, reg_t > > &missingParts = _memCacheCopies[index]._locations;
      reg_key_t dict = _memCacheCopies[index]._reg.key;
      for ( std::list< std::pair< reg_t, reg_t > >::iterator it = missingParts.begin(); it != missingParts.end(); it++ ) {
         if ( it->first != it->second ) {
            NewNewDirectoryEntryData *firstEntry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->first );
            NewNewDirectoryEntryData *secondEntry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->second );
            if ( firstEntry == NULL ) {
               if ( secondEntry != NULL ) {
                  firstEntry = NEW NewNewDirectoryEntryData();
                  *firstEntry = *secondEntry;
               } else {
                  if ( secondEntry != NULL ) {
                     *firstEntry = *secondEntry;
                  } else {
                     *myThread->_file << "Dunno what to do..."<<std::endl;
                  }
               }
            } else {
               NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->first );
               if ( entry == NULL ) {
                  entry = NEW NewNewDirectoryEntryData();
                  dict->setRegionData( it->first, entry ); //preInit fragment
               } else {
               }
            }
         }
      }
   }

   memory_space_id_t rooted_loc = 0;
   if ( this->isRooted( rooted_loc ) ) {
      _wd.tieToLocation( rooted_loc );
   }

   _preinitialized = true;
}

void MemController::initialize( ProcessingElement &pe ) {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   if ( !_initialized ) {
      _pe = &pe;

      if ( _pe->getMemorySpaceId() == 0 /* HOST_MEMSPACE_ID */) {
         _inOps = NEW HostAddressSpaceInOps( _pe, false );
      } else {
         _inOps = NEW SeparateAddressSpaceInOps( _pe, false, sys.getSeparateMemory( _pe->getMemorySpaceId() ) );
      }
#ifdef NANOS_RESILIENCY_ENABLED
      if( sys.isResiliencyEnabled() && _wd.isRecoverable() ) {
         _backupOpsIn = NEW SeparateAddressSpaceInOps(_pe, true, sys.getBackupMemory() );
         _backupOpsOut = NEW SeparateAddressSpaceInOps(_pe, true, sys.getBackupMemory() );
      }
#endif
      _initialized = true;
   } else {
      ensure(_pe == &pe, " MemController, called initialize twice with different PE!");
   }
}

bool MemController::allocateTaskMemory() {
   bool result = true;
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");
   if ( _pe->getMemorySpaceId() != 0 ) {
      bool pending_invalidation = false;
      bool initially_allocated = _memoryAllocated;

      if ( !sys.useFineAllocLock() ) {
      sys.allocLock();
      }
      
      if ( !_memoryAllocated && !_invalidating ) {
         bool tmp_result = sys.getSeparateMemory( _pe->getMemorySpaceId() ).prepareRegions( _memCacheCopies, _wd.getNumCopies(), _wd );
         if ( tmp_result ) {
            for ( unsigned int idx = 0; idx < _wd.getNumCopies() && !pending_invalidation; idx += 1 ) {
               pending_invalidation = (_memCacheCopies[idx]._invalControl._invalOps != NULL);
            }
            if ( pending_invalidation ) {
               _invalidating = true;
               result = false;
            } else {
               _memoryAllocated = true;
            }
         } else {
            result = false;
         }

      } else if ( _invalidating ) {
         for ( unsigned int idx = 0; idx < _wd.getNumCopies(); idx += 1 ) {
            if ( _memCacheCopies[idx]._invalControl._invalOps != NULL ) {
               _memCacheCopies[idx]._invalControl.waitOps( _pe->getMemorySpaceId(), _wd );

               if ( _memCacheCopies[idx]._invalControl._invalChunk != NULL ) {
                  _memCacheCopies[idx]._chunk = _memCacheCopies[idx]._invalControl._invalChunk;
                  *(_memCacheCopies[idx]._invalControl._invalChunkPtr) = _memCacheCopies[idx]._invalControl._invalChunk;
               }

               _memCacheCopies[idx]._invalControl.abort( _wd );
               _memCacheCopies[idx]._invalControl._invalOps = NULL;
            }
         }
         _invalidating = false;

         bool tmp_result = sys.getSeparateMemory( _pe->getMemorySpaceId() ).prepareRegions( _memCacheCopies, _wd.getNumCopies(), _wd );
         if ( tmp_result ) {
            pending_invalidation = false;
            for ( unsigned int idx = 0; idx < _wd.getNumCopies() && !pending_invalidation; idx += 1 ) {
               pending_invalidation = (_memCacheCopies[idx]._invalControl._invalOps != NULL);
            }
            if ( pending_invalidation ) {
               _invalidating = true;
               result = false;
            } else {
               _memoryAllocated = true;
            }
         } else {
            result = false;
         }
      } else {
         result = true;
      }

      if ( !initially_allocated && _memoryAllocated ) {
         for ( unsigned int idx = 0; idx < _wd.getNumCopies(); idx += 1 ) {
            int targetChunk = _memCacheCopies[ idx ]._allocFrom;
            if ( targetChunk != -1 ) {
               _memCacheCopies[ idx ]._chunk = _memCacheCopies[ targetChunk ]._chunk;
               _memCacheCopies[ idx ]._chunk->addReference( _wd, 133 ); //allocateTaskMemory, chunk allocated by other copy
            }
         }
      }
      if ( !sys.useFineAllocLock() ) {
      sys.allocUnlock();
      }
   } else {

      _memoryAllocated = true;

      for ( unsigned int idx = 0; idx < _wd.getNumCopies(); idx += 1 ) {
         if ( _memCacheCopies[idx]._reg.key->getKeepAtOrigin() ) {
            _memCacheCopies[idx]._reg.setOwnedMemory( _pe->getMemorySpaceId() );
         }
      }
   }

#ifdef NANOS_RESILIENCY_ENABLED
   if( !_backupCacheCopies.empty() ) {
      // TODO/FIXME: take care with reinterpret_cast if we add new members to BackupCacheCopy, as it could lead to
      // invalid memory accesses (buffer overflows, etc.)
      result &= sys.getBackupMemory().prepareRegions( reinterpret_cast<std::vector<MemCacheCopy>& >(_backupCacheCopies), _wd );
   }
#endif
   return result;
}

void MemController::copyDataIn() {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");
  
   if ( _VERBOSE_CACHE || sys.getVerboseCopies() ) {
   }
   
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      _memCacheCopies[ index ].generateInOps( *_inOps, _wd.getCopies()[index].isInput(), _wd.getCopies()[index].isOutput(), _wd, index );
   }

   _inOps->issue( &_wd );

#ifdef NANOS_RESILIENCY_ENABLED
   if ( !_backupCacheCopies.empty() && !_wd.isInvalid() ) {
      ensure( _backupOpsIn, "Backup ops array has not been initialized!" );

      bool queuedOps = false;
      for (unsigned int index = 0; index < _backupCacheCopies.size(); index++) {
         if ( _wd.getCopies()[index].isInput() ) {
            //_backupCacheCopies[index].setVersion( _memCacheCopies[ index ].getChildrenProducedVersion() );
            _backupCacheCopies[index]._locations.clear();

            if ( _wd.getCopies()[index].isOutput() ) {
               // For inout parameters, make a temporary independent backup. We have to do this privately, without
               // the cache being noticed, because this backup is for exclusive use of this workdescriptor only.
               _backupInOutCopies.emplace_back( _wd.getCopies()[index], &_wd, index );

               // Note: we dont want to make the regular backup for inouts, like children tasks' "in".
               // Parameters will always do the backup later if they exist no matter whether we perform the copy or not
               //if(sys.getVerboseCopies()) //message( "Private copyIn (inout) wd:", std::dec, _wd.getId() );
               NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
               NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_INOUT );
               NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

               _backupInOutCopies[index].checkpoint( &_wd );

               NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );

            } else {
               _backupCacheCopies[index]._locations.push_back( std::pair<reg_t, reg_t>( _backupCacheCopies[index]._reg.id, _backupCacheCopies[index]._reg.id ) );
               _backupCacheCopies[index]._locationDataReady = true;

               _backupCacheCopies[ index ].generateInOps( *_backupOpsIn, true, false, _wd, index);
               queuedOps = true;
            }
         }
      }

      if( queuedOps ) {
         NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
         NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_IN );
         NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

         _backupOpsIn->issue(&_wd);

         NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
      }
   }
#endif
   //NANOS_INSTRUMENT( inst2.close(); );
}

void MemController::copyDataOut( MemControllerPolicy policy ) {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");

   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      if ( _wd.getCopies()[index].isOutput() ) {
         if ( _wd.getParent() != NULL && _wd.getParent()->_mcontrol.ownsRegion( _memCacheCopies[index]._reg ) ) {
            WD &parent = *(_wd.getParent());
            for ( unsigned int parent_idx = 0; parent_idx < parent.getNumCopies(); parent_idx += 1) {
               if ( parent._mcontrol._memCacheCopies[parent_idx]._reg.contains( _memCacheCopies[ index ]._reg ) ) {
                  if ( parent._mcontrol._memCacheCopies[parent_idx].getChildrenProducedVersion() < _memCacheCopies[ index ].getChildrenProducedVersion() ) {
                     parent._mcontrol._memCacheCopies[parent_idx].setChildrenProducedVersion( _memCacheCopies[ index ].getChildrenProducedVersion() );
                  }
               }
            }
         }
      }
   }

   if ( _pe->getMemorySpaceId() == 0 /* HOST_MEMSPACE_ID */) {
      _outputDataReady = true;
   } else {
      _outOps = NEW SeparateAddressSpaceOutOps( _pe, false, true );

      for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
         _memCacheCopies[ index ].generateOutOps( &sys.getSeparateMemory( _pe->getMemorySpaceId() ), *_outOps, _wd.getCopies()[index].isInput(), _wd.getCopies()[index].isOutput(), _wd, index );
      }

      _outOps->issue( &_wd );
   }
             NANOS_INSTRUMENT(sys.getInstrumentation()->raiseOpenBurstEvent( ikey, 0 );)
}

memory::Address MemController::getAddress( unsigned int index ) const {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");
   memory::Address addr = nullptr;
   if ( _pe->getMemorySpaceId() == 0 ) {
      addr = _wd.getCopies()[ index ].getBaseAddress();
   } else {
      addr = sys.getSeparateMemory( _pe->getMemorySpaceId() ).getDeviceAddress( _memCacheCopies[ index ]._reg, (memory::Address) _wd.getCopies()[ index ].getBaseAddress(), _memCacheCopies[ index ]._chunk );
   }
   return addr;
}

void MemController::getInfoFromPredecessor( MemController const &predecessorController ) {
   if ( sys.usePredecessorCopyInfo() ) {
      for( unsigned int index = 0; index < predecessorController._wd.getNumCopies(); index += 1) {
         unsigned int version = predecessorController._memCacheCopies[ index ].getChildrenProducedVersion(); 
         unsigned int predecessorProducedVersion = predecessorController._memCacheCopies[ index ].getVersion() + (predecessorController._wd.getCopies()[ index ].isOutput() ? 1 : 0);
         if ( predecessorProducedVersion == version ) {
            // if the predecessor's children produced new data, then the father can not
            // guarantee that the version is correct (the children may have produced a subchunk
            // of the region). The version is not added here and then the global directory is checked.
            _providedRegions.addRegion( predecessorController._memCacheCopies[ index ]._reg, version );
         }
      }
   }
}

bool MemController::isDataReady ( WD const &wd )
{
   ensure( _preinitialized == true, "MemController not initialized!");
   if ( _initialized ) {
      if ( !_inputDataReady ) {
         _inputDataReady = _inOps->isDataReady( wd );
#ifdef NANOS_RESILIENCY_ENABLED
         if ( _wd.isRecoverable() && _backupOpsIn) {
            _inputDataReady &= _backupOpsIn->isDataReady(wd);
            _backupOpsIn->releaseLockedSourceChunks(wd);
         }
#endif
      }
      return _inputDataReady;
   }
   return false;
}

bool MemController::isOutputDataReady( WD const &wd )
{
   ensure( _preinitialized == true, "MemController::isOutputDataReady: MemController not initialized!");

   if ( _initialized ) {
      if ( !_outputDataReady ) {
         _outputDataReady = _outOps->isDataReady( wd );
         if ( _outputDataReady ) {
            if ( _VERBOSE_CACHE ) { *(myThread->_file) << "Output data is ready for wd " << _wd.getId() << " obj " << (void *)_outOps << std::endl; }

            sys.getSeparateMemory( _pe->getMemorySpaceId() ).releaseRegions( _memCacheCopies, _wd.getNumCopies(), _wd ) ;
         }
      }
#ifdef NANOS_RESILIENCY_ENABLED
      if ( _wd.isRecoverable() && _backupOpsOut) {
         _outputDataReady = _backupOpsOut->isDataReady(wd);
         _backupOpsOut->releaseLockedSourceChunks(wd);
      }
#endif
      return _outputDataReady;
   }
   return false;
}

#ifdef NANOS_RESILIENCY_ENABLED
bool MemController::isDataRestored( WD const &wd )
{
   ensure( _preinitialized == true, "MemController::isDataRestored: MemController not initialized!");
   ensure( _wd.isRecoverable(), "Task is not recoverable. There wasn't any data to be restored. ");

   if ( _initialized ) {
      if ( _restoreOps && !_dataRestored ) {
         _dataRestored = _restoreOps->isDataReady( wd );

         for ( unsigned int index = 0; index < _backupCacheCopies.size(); index++ ) {
            AllocatedChunk *backup = _backupCacheCopies[index]._chunk;
            if( backup ) {
               CachedRegionStatus* entry = (CachedRegionStatus*)backup->getNewRegions()->getRegionData( backup->getAllocatedRegion().id );
               const bool invalid_entry = entry && !entry->isValid();
               if( invalid_entry ) {
                  throw std::runtime_error("Invalidated region found");
               }
            }
         }

         if ( _dataRestored ) {
            if ( _VERBOSE_CACHE ) { *(myThread->_file) << "Restored data is ready for wd " << _wd.getId() << " obj " << (void *)_restoreOps << std::endl; }

            /* Is this the data invalidation? I don't think so
            for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
               sys.getBackupMemory().releaseRegion( _backupCacheCopies[ index ]._reg, _wd, index, _backupCacheCopies[ index ]._policy ) ;
            }*/
         }
      } else {
         _dataRestored = true;
      }
      return _dataRestored;
   }
   return false;
}
#endif

bool MemController::canAllocateMemory( memory_space_id_t memId, bool considerInvalidations ) const {
   if ( memId > 0 ) {
      return sys.getSeparateMemory( memId ).canAllocateMemory( _memCacheCopies, _wd.getNumCopies(), considerInvalidations, _wd );
   } else {
      return true;
   }
}


void MemController::setAffinityScore( std::size_t score ) {
   _affinityScore = score;
}

std::size_t MemController::getAffinityScore() const {
   return _affinityScore;
}

void MemController::setMaxAffinityScore( std::size_t score ) {
   _maxAffinityScore = score;
}

std::size_t MemController::getMaxAffinityScore() const {
   return _maxAffinityScore;
}

std::size_t MemController::getAmountOfTransferredData() const {
   return ( _inOps != NULL ) ? _inOps->getAmountOfTransferredData() : 0 ;
}

std::size_t MemController::getTotalAmountOfData() const {
   std::size_t total = 0;
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      total += _memCacheCopies[ index ]._reg.getDataSize();
   }
   return total;
}

bool MemController::isRooted( memory_space_id_t &loc ) const {
   bool result = false;
   memory_space_id_t refLoc = (memory_space_id_t) -1;
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      memory_space_id_t thisLoc;
      if ( _memCacheCopies[ index ].isRooted( thisLoc ) ) {
         thisLoc = thisLoc == 0 ? 0 : ( sys.getSeparateMemory( thisLoc ).getNodeNumber() != 0 ? thisLoc : 0 );
         if ( refLoc == (memory_space_id_t) -1 ) {
            refLoc = thisLoc;
            result = true;
         } else {
            result = (refLoc == thisLoc);
         }
      }
   }
   if ( result ) loc = refLoc;
   return result;
}

bool MemController::isMultipleRooted( std::list<memory_space_id_t> &locs ) const {
   unsigned int count = 0;
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      memory_space_id_t thisLoc;
      if ( _memCacheCopies[ index ].isRooted( thisLoc ) ) {
         count += 1;
         locs.push_back( thisLoc );
      }
   }
   return count > 1;
}

void MemController::setMainWD() {
   _mainWd = true;
}

void MemController::synchronize() {
   sys.getHostMemory().synchronize( _wd );
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      if ( _wd.getCopies()[index].isOutput() ) {
         unsigned int newVersion = _memCacheCopies[index].getChildrenProducedVersion() +1;
         _memCacheCopies[index]._reg.setLocationAndVersion( _pe, _pe->getMemorySpaceId(), newVersion ); // update directory
         _memCacheCopies[index].setChildrenProducedVersion( newVersion );
      }
   }
}

bool MemController::isMemoryAllocated() const {
   return _memoryAllocated;
}

void MemController::setCacheMetaData() {
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      if ( _wd.getCopies()[index].isOutput() ) {
         unsigned int newVersion = _memCacheCopies[ index ].getVersion() + 1;
         _memCacheCopies[ index ]._reg.setLocationAndVersion( _pe, _pe->getMemorySpaceId(), newVersion ); //update directory, OUT copies, (upgrade version)
         _memCacheCopies[ index ].setChildrenProducedVersion( newVersion );

         if ( _pe->getMemorySpaceId() != 0 /* HOST_MEMSPACE_ID */) {
            sys.getSeparateMemory( _pe->getMemorySpaceId() ).setRegionVersion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ]._chunk, newVersion, _wd, index );
         }
      } else if ( _wd.getCopies()[index].isInput() ) {
         _memCacheCopies[ index ].setChildrenProducedVersion( _memCacheCopies[ index ].getVersion() );
      }
   }
}

bool MemController::hasObjectOfRegion( global_reg_t const &reg ) {
   return _ownedRegions.hasObjectOfRegion( reg );
}


bool MemController::containsAllCopies( MemController const &target ) const {
   bool result = true;
   for ( unsigned int idx = 0; idx < target._wd.getNumCopies() && result; idx += 1 ) {
      bool this_reg_is_contained = false;
      for ( unsigned int this_idx = 0; this_idx < _wd.getNumCopies() && !this_reg_is_contained; this_idx += 1 ) {
         if ( target._memCacheCopies[idx]._reg.key == _memCacheCopies[this_idx]._reg.key ) {
            reg_key_t key = target._memCacheCopies[idx]._reg.key;
            reg_t this_reg = _memCacheCopies[this_idx]._reg.id;
            reg_t target_reg = target._memCacheCopies[idx]._reg.id;
            if ( target_reg == this_reg || ( key->checkIntersect( target_reg, this_reg ) && key->computeIntersect( target_reg, this_reg ) == target_reg ) ) {
               this_reg_is_contained = true;
            }
         }
      }
      result = this_reg_is_contained;
   }
   return result;
}

}
