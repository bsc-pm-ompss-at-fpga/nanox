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
#include "debug.hpp"
#include "instrumentation.hpp"
#include "system.hpp"

#ifdef NANOS_RESILIENCY_ENABLED
#   include "backupmanager.hpp"
#   include "backupprivatecopy.hpp"
#   include "exception/taskrecoveryfailed.hpp"
#endif

namespace nanos {
MemController::MemController( WD *wd, unsigned int numCopies ) : 
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
   _memCacheCopies.reserve( numCopies );
}

MemController::~MemController() {
   delete _inOps;
   delete _outOps;
#ifdef NANOS_RESILIENCY_ENABLED
   if( _backupOpsIn )
      delete _backupOpsIn;
   if( _backupOpsOut )
      delete _backupOpsOut;
#endif
}

bool MemController::ownsRegion( global_reg_t const &reg ) {
   bool i_has_it = _ownedRegions.hasObjectOfRegion( reg );
   bool parent_has_it  = _parentRegions.hasObjectOfRegion( reg );
   return i_has_it || parent_has_it;
}

void MemController::preInit( ) {
   verbose_cache(" (preinit)INITIALIZING MEMCONTROLLER for WD:", *_wd, "; ", _wd->getNumCopies(), " copies" );

   if ( _preinitialized ) return;

   unsigned index, parent_index;
   for ( index = 0; index < _wd->getNumCopies(); index++ ) {
      _memCacheCopies.emplace_back( *_wd, index );
   }

   WorkDescriptor* parent = _wd->getParent();
   for ( index = 0; index < _wd->getNumCopies(); index++ ) {
      memory::Address host_copy_addr = nullptr;
      if ( parent != NULL /* && !parent->_mcontrol._mainWd */ ) {
         for ( parent_index = 0; parent_index < parent->getNumCopies(); parent_index++ ) {
            if ( parent->_mcontrol.getAddress( parent_index ) == _wd->getCopies()[ index ].getBaseAddress() ) {
               host_copy_addr = parent->getCopies()[ parent_index ].getHostBaseAddress();
               _wd->getCopies()[ index ].setHostBaseAddress( host_copy_addr );
            }
         }
      }
   }

   for ( index = 0; index < _memCacheCopies.size(); index++ ) {
      MemCacheCopy& cacheCopy = _memCacheCopies[index];

      if ( sys.usePredecessorCopyInfo() ) {
         unsigned int predecessorsVersion;
         if ( _providedRegions.hasVersionInfoForRegion( cacheCopy._reg, predecessorsVersion, cacheCopy._locations ) ) {
            cacheCopy.setVersion( predecessorsVersion );
         }
      }

      if ( cacheCopy.getVersion() != 0 ) {
         verbose_cache( "WD:", *_wd, "; copy ", index, "got location info from predecessor, "
                        "reg[", _memCacheCopies[index]._reg.key, ",", _memCacheCopies[index]._reg.id, "] "
                        "got version ", _memCacheCopies[index].getVersion() );

         cacheCopy._locationDataReady = true;
      } else {
         verbose_cache( "WD:", *_wd, "; copy ", index, "got location info from global directory, "
                        "reg[", _memCacheCopies[index]._reg.key, ",", _memCacheCopies[index]._reg.id, "]" );

         cacheCopy.getVersionInfo();
      }
      verbose_cache( _memCacheCopies[index] );

      if( parent != NULL ) {
         if ( parent->_mcontrol.ownsRegion( cacheCopy._reg ) ) {
            /* do nothing, maybe here we can add a correctness check,
             * to ensure that the region is a subset of the Parent regions
             */
            _parentRegions.addRegion( cacheCopy._reg, cacheCopy.getVersion() );
         } else { /* this should be for private data */
             parent->_mcontrol._ownedRegions.addRegion( cacheCopy._reg, cacheCopy.getVersion() );
         }
      }
   }

#ifdef NANOS_RESILIENCY_ENABLED
   if( sys.isResiliencyEnabled() && _wd->isRecoverable() ) {
      // Doing vector::reserve inside the constructor is not
      // possible, since workdescriptor flags are set after
      // the workdescriptor is created
      _backupCacheCopies.reserve( _memCacheCopies.size() );
      _backupInOutCopies.reserve( _memCacheCopies.size() );

      for ( index = 0; index < _memCacheCopies.size(); index ++ ) {
            _backupCacheCopies.emplace_back( _wd->getCopies()[index], _memCacheCopies[index], *_wd, index );
      }
   }
#endif

       for ( index = 0; index < _wd->getNumCopies(); index += 1 ) {
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
                      firstEntry = NEW NewNewDirectoryEntryData();
                      secondEntry = NEW NewNewDirectoryEntryData();
                      dict->setRegionData( it->second, secondEntry ); // preInit fragment
                   }
                   dict->setRegionData( it->first, firstEntry ); //preInit fragment
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


   verbose_cache( "### preInit WD:", *_wd );
   verbose_cache( " ## ", _memCacheCopies );

   memory_space_id_t rooted_loc = 0;
   if ( this->isRooted( rooted_loc ) ) {
      _wd->tieToLocation( rooted_loc );
   }

   verbose_cache(" (preinit)END OF INITIALIZING MEMCONTROLLER for WD ", *_wd, "; ", _wd->getNumCopies(), " copies &_preinitialized=", &_preinitialized );
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
      if( sys.isResiliencyEnabled() && _wd->isRecoverable() ) {
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
         bool tmp_result = sys.getSeparateMemory( _pe->getMemorySpaceId() ).prepareRegions( _memCacheCopies.data(), _wd->getNumCopies(), *_wd );
         if ( tmp_result ) {
            for ( unsigned int idx = 0; idx < _wd->getNumCopies() && !pending_invalidation; idx += 1 ) {
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
         for ( unsigned int idx = 0; idx < _wd->getNumCopies(); idx += 1 ) {
            if ( _memCacheCopies[idx]._invalControl._invalOps != NULL ) {
               _memCacheCopies[idx]._invalControl.waitOps( _pe->getMemorySpaceId(), *_wd );

               if ( _memCacheCopies[idx]._invalControl._invalChunk != NULL ) {
                  _memCacheCopies[idx]._chunk = _memCacheCopies[idx]._invalControl._invalChunk;
                  *(_memCacheCopies[idx]._invalControl._invalChunkPtr) = _memCacheCopies[idx]._invalControl._invalChunk;
               }

               _memCacheCopies[idx]._invalControl.abort( *_wd );
               _memCacheCopies[idx]._invalControl._invalOps = NULL;
            }
         }
         _invalidating = false;

         bool tmp_result = sys.getSeparateMemory( _pe->getMemorySpaceId() ).prepareRegions( _memCacheCopies.data(), _wd->getNumCopies(), *_wd );
         if ( tmp_result ) {
            pending_invalidation = false;
            for ( unsigned int idx = 0; idx < _wd->getNumCopies() && !pending_invalidation; idx += 1 ) {
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
         for ( unsigned int idx = 0; idx < _wd->getNumCopies(); idx += 1 ) {
            int targetChunk = _memCacheCopies[ idx ]._allocFrom;
            if ( targetChunk != -1 ) {
               _memCacheCopies[ idx ]._chunk = _memCacheCopies[ targetChunk ]._chunk;
               _memCacheCopies[ idx ]._chunk->addReference( *_wd, 133 ); //allocateTaskMemory, chunk allocated by other copy
            }
         }
      }
      if ( !sys.useFineAllocLock() ) {
      sys.allocUnlock();
      }
   } else {

      _memoryAllocated = true;

      // *(myThread->_file) << "++++ Succeeded allocation for wd " << _wd->getId();
      for ( unsigned int idx = 0; idx < _wd->getNumCopies(); idx += 1 ) {
         // *myThread->_file << " [c: " << (void *) _memCacheCopies[idx]._chunk << " w/hAddr " << (void *) _memCacheCopies[idx]._chunk->getHostAddress() << " - " << (void*)(_memCacheCopies[idx]._chunk->getHostAddress() + _memCacheCopies[idx]._chunk->getSize()) << "]";
         if ( _memCacheCopies[idx]._reg.key->getKeepAtOrigin() ) {
            _memCacheCopies[idx]._reg.setOwnedMemory( _pe->getMemorySpaceId() );
         }
      }
   }

#ifdef NANOS_RESILIENCY_ENABLED
   if( !_backupCacheCopies.empty() ) {
      // TODO/FIXME: take care with reinterpret_cast if we add new members to BackupCacheCopy, as it could lead to
      // invalid memory accesses (buffer overflows, etc.)
      result &= sys.getBackupMemory().prepareRegions( reinterpret_cast<MemCacheCopy*>(_backupCacheCopies.data()), _backupCacheCopies.size(), *_wd );
   }
#endif
   return result;
}

void MemController::copyDataIn() {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");
  
   verbose_cache( "### copyDataIn WD:", *_wd, "; ", _wd->getNumCopies(), " copies; running on ", _pe->getMemorySpaceId(), "; ops:", _inOps );
   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
      const CopyData& copy = _wd->getCopies()[index];

      verbose_cache( " ## input:",copy.isInput(), " output:",copy.isOutput(), "; ", _memCacheCopies[index] );
      _memCacheCopies[ index ].generateInOps( *_inOps, copy.isInput(), copy.isOutput(), *_wd, index );
   }
   _inOps->issue( _wd );

#ifdef NANOS_RESILIENCY_ENABLED
   if ( !_backupCacheCopies.empty() && !_wd->isInvalid() ) {
      ensure( _backupOpsIn, "Backup ops array has not been initialized!" );

      bool queuedOps = false;
      for (unsigned int index = 0; index < _backupCacheCopies.size(); index++) {
         if ( _wd->getCopies()[index].isInput() ) {
            //_backupCacheCopies[index].setVersion( _memCacheCopies[ index ].getChildrenProducedVersion() );
            _backupCacheCopies[index]._locations.clear();

            if ( _wd->getCopies()[index].isOutput() ) {
               // For inout parameters, make a temporary independent backup. We have to do this privately, without
               // the cache being noticed, because this backup is for exclusive use of this workdescriptor only.
               _backupInOutCopies.emplace_back( _wd->getCopies()[index], _wd, index );
               _backupInOutCopies.back().checkpoint( _wd );

            } else {
               _backupCacheCopies[index]._locations.push_back( std::pair<reg_t, reg_t>( _backupCacheCopies[index]._reg.id, _backupCacheCopies[index]._reg.id ) );
               _backupCacheCopies[index]._locationDataReady = true;

               _backupCacheCopies[ index ].generateInOps( *_backupOpsIn, true, false, *_wd, index);
               queuedOps = true;
            }
         }
      }

      if( queuedOps ) {
         NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
         NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_IN );
         NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

         _backupOpsIn->issue(_wd);

         NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
      }
   }
#endif

   verbose_cache( "### copyDataIn WD:", *_wd, " done" );
}

void MemController::copyDataOut( MemControllerPolicy policy ) {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");

   verbose_cache( "### CopyDataOut WD:", *_wd, " metadata set, not released yet" );

   //for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
   //   if ( _wd->getCopies()[index].isInput() && _wd->getCopies()[index].isOutput() ) {
   //      _memCacheCopies[ index ]._reg.setLocationAndVersion( _pe->getMemorySpaceId(), _memCacheCopies[ index ].getVersion() + 1 );
   //   }
   //}

   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
      const CopyData& copy = _wd->getCopies()[index];
      MemCacheCopy& cacheCopy = _memCacheCopies[index];

      if ( copy.isOutput() ) {
         WD* parent = _wd->getParent();
         if ( parent != NULL && parent->_mcontrol.ownsRegion( cacheCopy._reg ) ) {

            ensure( cacheCopy._reg.id != 0, "Error reg == 0!! this!" );
            for ( unsigned int parent_idx = 0; parent_idx < parent->getNumCopies(); parent_idx++ ) {
               MemCacheCopy& parentCacheCopy = parent->_mcontrol._memCacheCopies[parent_idx];

               ensure( parentCacheCopy._reg.id != 0 , "Error reg == 0!! parent!" );

               if ( parentCacheCopy._reg.contains( cacheCopy._reg ) ) {
                  if ( parentCacheCopy.getChildrenProducedVersion() < cacheCopy.getChildrenProducedVersion() ) {
                     parentCacheCopy.setChildrenProducedVersion( cacheCopy.getChildrenProducedVersion() );
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

      for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
         const CopyData& copy = _wd->getCopies()[index];
         SeparateMemoryAddressSpace& memory = sys.getSeparateMemory( _pe->getMemorySpaceId() );

         _memCacheCopies[ index ].generateOutOps( &memory, *_outOps, copy.isInput(), copy.isOutput(), *_wd, index );
      }

      _outOps->issue( _wd );
   }

#ifdef NANOS_RESILIENCY_ENABLED
   if (sys.isResiliencyEnabled() && _wd->isRecoverable() ) {

      if( !_wd->isInvalid() ) {
         ensure( _backupOpsOut, "Backup ops array has not been initialized!" );

         bool ops_queued = false;
         for ( unsigned int index = 0; index < _wd->getNumCopies(); index++) {
            // Needed for CP input data
            if( _wd->getCopies()[index].isOutput() ) {

               AllocatedChunk *backup = _backupCacheCopies[index]._chunk;
               if( backup ) {
                  CachedRegionStatus* entry = (CachedRegionStatus*)backup->getNewRegions()->getRegionData( backup->getAllocatedRegion().id );
                  const bool valid_entry = entry && entry->isValid();
                  if( entry && !valid_entry ) {
                     // If the entry is not valid, we set up its version to 0 so future backup overwrites aren't given any errors
                     entry->resetVersion();
                     _backupCacheCopies[index].setVersion( 0 );
                  }
                  if( !entry || valid_entry ) {
                     _backupCacheCopies[index].setVersion( _memCacheCopies[ index ].getChildrenProducedVersion() );
                     // Delete locations from input copies, we don't want to checkpoint read-only data again
                     _backupCacheCopies[index]._locations.clear();
                     // Add locations for output copies, they will be checkpointed immediately
                     _backupCacheCopies[index]._locations.push_back( std::pair<reg_t, reg_t>( _backupCacheCopies[index]._reg.id, _backupCacheCopies[index]._reg.id ) );
                     _backupCacheCopies[index]._locationDataReady = true;

                     _backupCacheCopies[index].generateInOps( *_backupOpsOut, true, false, *_wd, index);
                     ops_queued = true;
                  }
               }
            }
         }

         // We try to issue valid copies' checkpoint, if any.
         if( ops_queued ) {
            NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
            NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_OUT );
            NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

            _backupOpsOut->issue( _wd );

            NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
         }
      }

      // Inoutparameters' backup have to be cleaned: they are private
      _backupInOutCopies.clear();
   }
#endif

   verbose_cache( "### copyDataOut WD:", *_wd, " done" );
}

#ifdef NANOS_RESILIENCY_ENABLED
void MemController::restoreBackupData ( )
{
   ensure( _preinitialized == true, "MemController::restoreBackupData: MemController not initialized!");
   ensure( _initialized == true, "MemController::restoreBackupData: MemController not initialized!");
   ensure( _wd->isRecoverable(), "Cannot restore data of an unrecoverable task!" );
   ensure( !_wd->getNumCopies() || !_backupCacheCopies.empty(), "There are no backup copies defined for this task." );

   if (!_backupCacheCopies.empty()) {
      _restoreOps = NEW SeparateAddressSpaceOutOps( _pe, false, true);

      bool failed = false;
      unsigned int index = 0;
      // Inout args have to be restored even if were not corrupted (they may be dirty).
      while( !failed && index < _backupInOutCopies.size() ) {

         _backupInOutCopies[index].restore( _wd );

         failed = _backupInOutCopies[index].isAborted();
         index++;
      }

      if( !failed ) {
         SeparateMemoryAddressSpace& memory = sys.getBackupMemory();
         // Restore the rest of the input data
         for( index = 0; index < _wd->getNumCopies(); index++ ) {
            if ( _wd->getCopies()[index].isInput()
             && !_wd->getCopies()[index].isOutput() ) {
               _backupCacheCopies[index].generateOutOps( &memory, *_restoreOps, false, true, *_wd, index);
            }
            index++;
         }

         NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
         NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_RT_IN );
         NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

         _restoreOps->issue(_wd);

         NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
      } else {
         throw error::TaskRecoveryFailed();
      }
   }
}
#endif

memory::Address MemController::getAddress( unsigned int index ) const {
   ensure( _preinitialized == true, "MemController not preinitialized!");
   ensure( _initialized == true, "MemController not initialized!");
   memory::Address addr = nullptr;
   if ( _pe->getMemorySpaceId() == 0 ) {
      addr = _wd->getCopies()[ index ].getBaseAddress();
   } else {
      addr = sys.getSeparateMemory( _pe->getMemorySpaceId() ).getDeviceAddress( _memCacheCopies[ index ]._reg, (memory::Address) _wd->getCopies()[ index ].getBaseAddress(), _memCacheCopies[ index ]._chunk );
   }
   return addr;
}

void MemController::getInfoFromPredecessor( MemController const &predecessorController ) {
   if ( sys.usePredecessorCopyInfo() ) {
      for( unsigned int index = 0; index < predecessorController._wd->getNumCopies(); index++) {
         unsigned int version = predecessorController._memCacheCopies[ index ].getChildrenProducedVersion(); 
         unsigned int predecessorProducedVersion = predecessorController._memCacheCopies[ index ].getVersion() + (predecessorController._wd->getCopies()[ index ].isOutput() ? 1 : 0);
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
         if ( _wd->isRecoverable() && _backupOpsIn) {
            _inputDataReady &= _backupOpsIn->isDataReady(wd);
            _backupOpsIn->releaseLockedSourceChunks(wd);
         }
#endif
      }
      return _inputDataReady;
   }
   return false;
}


bool MemController::isOutputDataReady( WD const &wd ) {
   if ( _preinitialized != true ) {
      std::cerr << " CANT CALL " << __func__ << " BEFORE PRE INIT (wd: " << wd.getId() << " - " << (wd.getDescription() != NULL ? wd.getDescription() : "n/a") << ")" << std::endl;
   }
   ensure( _preinitialized == true, "MemController not initialized! wd " );
   if ( _initialized ) {
      if ( !_outputDataReady ) {
         _outputDataReady = _outOps->isDataReady( wd );
         if ( _outputDataReady ) {
            verbose_cache( "Output data is ready for wd ", *_wd, "; obj:", _outOps );

            sys.getSeparateMemory( _pe->getMemorySpaceId() ).releaseRegions( _memCacheCopies.data(), _wd->getNumCopies(), *_wd ) ;
         }
      }
      return _outputDataReady;
   }
   return false;
}

#ifdef NANOS_RESILIENCY_ENABLED
bool MemController::isDataRestored( WD const &wd ) {
   ensure( _preinitialized == true, "MemController::isDataRestored: MemController not initialized!");
   ensure( _wd->isRecoverable(), "Task is not recoverable. There wasn't any data to be restored. ");
   if ( _initialized ) {
      if ( _restoreOps && !_dataRestored ) {
         _dataRestored = _restoreOps->isDataReady( wd );

         for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
            AllocatedChunk *backup = _backupCacheCopies[index]._chunk;
            if( backup ) {
               CachedRegionStatus* entry = (CachedRegionStatus*)backup->getNewRegions()->getRegionData( backup->getAllocatedRegion().id );
               const bool invalid_entry = entry && !entry->isValid();
               if( invalid_entry ) {
                  throw error::TaskRecoveryFailed();
               }
            }
         }

         if ( _dataRestored ) {
            if ( _VERBOSE_CACHE ) { *(myThread->_file) << "Restored data is ready for wd " << _wd->getId() << " obj " << (void *)_restoreOps << std::endl; }

            /* Is this the data invalidation? I don't think so
            for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
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
      // FIXME: change canAllocateMemory MemCacheCopy* argument to const
      return sys.getSeparateMemory( memId ).canAllocateMemory( _memCacheCopies.data(), _wd->getNumCopies(), considerInvalidations, *_wd );
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
   size_t transferred = 0;
   if ( _inOps != NULL )
      transferred = _inOps->getAmountOfTransferredData();
   return transferred;
}

std::size_t MemController::getTotalAmountOfData() const {
   std::size_t total = 0;
   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
      total += _memCacheCopies[ index ]._reg.getDataSize();
   }
   return total;
}

bool MemController::isRooted( memory_space_id_t &loc ) const {
   bool result = false;
   memory_space_id_t refLoc = (memory_space_id_t) -1;
   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
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
   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
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


void MemController::synchronize( std::size_t numDataAccesses, DataAccess *data ) {
   sys.getHostMemory().synchronize( *_wd, numDataAccesses, data );
}

void MemController::synchronize() {
   sys.getHostMemory().synchronize( *_wd );
/*
   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
      if ( _wd->getCopies()[index].isOutput() ) {
         unsigned int newVersion = _memCacheCopies[index].getChildrenProducedVersion() +1;
         _memCacheCopies[index]._reg.setLocationAndVersion( _pe, _pe->getMemorySpaceId(), newVersion ); // update directory
         _memCacheCopies[index].setChildrenProducedVersion( newVersion );
      }
   }
*/
}

bool MemController::isMemoryAllocated() const {
   return _memoryAllocated;
}

void MemController::setCacheMetaData() {
   for ( unsigned int index = 0; index < _wd->getNumCopies(); index++ ) {
      if ( _wd->getCopies()[index].isOutput() ) {
         unsigned int newVersion = _memCacheCopies[ index ].getVersion() + 1;
         _memCacheCopies[ index ]._reg.setLocationAndVersion( _pe, _pe->getMemorySpaceId(), newVersion ); //update directory, OUT copies, (upgrade version)
         _memCacheCopies[ index ].setChildrenProducedVersion( newVersion );

         if ( _pe->getMemorySpaceId() != 0 /* HOST_MEMSPACE_ID */) {
            sys.getSeparateMemory( _pe->getMemorySpaceId() ).setRegionVersion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ]._chunk, newVersion, *_wd, index );
         }
      } else if ( _wd->getCopies()[index].isInput() ) {
         _memCacheCopies[ index ].setChildrenProducedVersion( _memCacheCopies[ index ].getVersion() );
      }
   }
}

bool MemController::hasObjectOfRegion( global_reg_t const &reg ) {
   return _ownedRegions.hasObjectOfRegion( reg );
}


bool MemController::containsAllCopies( MemController const &target ) const {
   bool result = true;
   for ( unsigned int idx = 0; idx < target._wd->getNumCopies() && result; idx += 1 ) {
      bool this_reg_is_contained = false;
      for ( unsigned int this_idx = 0; this_idx < _wd->getNumCopies() && !this_reg_is_contained; this_idx += 1 ) {
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
