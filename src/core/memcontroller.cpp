#include "memcontroller_decl.hpp"
#include "workdescriptor.hpp"
#include "regiondict.hpp"
#include "newregiondirectory.hpp"
#include "cachedregionstatus.hpp"

#if VERBOSE_CACHE
 #define _VERBOSE_CACHE 1
#else
 #define _VERBOSE_CACHE 0
 //#define _VERBOSE_CACHE ( sys.getNetwork()->getNodeNum() == 0 )
#endif

#ifdef NANOS_RESILIENCY_ENABLED
#include "backupmanager.hpp"
#endif
#include <execinfo.h>

namespace nanos {
MemController::MemController ( WD &wd ) :
      _initialized( false), _preinitialized(false), _inputDataReady(false), _outputDataReady(
      false), _memoryAllocated( false), _mainWd( false), _is_private_backup_aborted(false), _wd(wd), _pe( NULL ), 
      _provideLock(), _providedRegions(), _inOps(NULL), _outOps(NULL), 
#ifdef NANOS_RESILIENCY_ENABLED
      _backupOpsIn(NULL), _backupOpsOut(NULL), _restoreOps(NULL), 
      _backupCacheCopies(NULL), _backupInOutCopies(NULL),
#endif
      _affinityScore(0), _maxAffinityScore(0), _ownedRegions(), _parentRegions()
{
   if (_wd.getNumCopies() > 0) {
      _memCacheCopies = NEW MemCacheCopy[wd.getNumCopies()];
#ifdef NANOS_RESILIENCY_ENABLED
      if( sys.isResiliencyEnabled() ) {
         _backupCacheCopies = NEW MemCacheCopy[_wd.getNumCopies()];
         _backupInOutCopies = NEW Chunk[_wd.getNumCopies()];
      }
#endif
   }
}

bool MemController::ownsRegion( global_reg_t const &reg ) {
   bool i_has_it = _ownedRegions.hasObjectOfRegion( reg );
   bool parent_has_it  = _parentRegions.hasObjectOfRegion( reg );
   //std::cerr << " wd: " << _wd.getId() << " i has it? " << (i_has_it ? "yes" : "no") << " " << &_ownedRegions << ", parent has it? " << (parent_has_it ? "yes" : "no") << " " << &_parentRegions << std::endl;
   return i_has_it || parent_has_it;
}

void MemController::preInit ( )
{
   unsigned int index;
   if (_preinitialized)
      return;
   if ( _VERBOSE_CACHE ) {
      *(myThread->_file) << " (preinit)INITIALIZING MEMCONTROLLER for WD "
            << _wd.getId() << " "
            << (_wd.getDescription() != NULL ? _wd.getDescription() : "n/a")
            << " NUM COPIES " << _wd.getNumCopies() << std::endl;
   }

   //std::ostream &o = (*myThread->_file);
   //o << "### preInit wd " << _wd.getId() << std::endl;
   for (index = 0; index < _wd.getNumCopies(); index += 1) {
      //std::cerr << "WD "<< _wd.getId() << " Depth: "<< _wd.getDepth() <<" Creating copy "<< index << std::endl;
      //std::cerr << _wd.getCopies()[ index ];
      uint64_t host_copy_addr = 0;
      if ( _wd.getParent() != NULL /* && !_wd.getParent()->_mcontrol._mainWd */ ) {
         for ( unsigned int parent_idx = 0; parent_idx < _wd.getParent()->getNumCopies(); parent_idx += 1 ) {
            if ( _wd.getParent()->_mcontrol.getAddress( parent_idx ) == (uint64_t) _wd.getCopies()[ index ].getBaseAddress() ) {
            host_copy_addr = (uint64_t) _wd.getParent()->getCopies()[ parent_idx ].getHostBaseAddress();
               //std::cerr << "TADAAAA this comes from a father's copy "<< std::hex << host_copy_addr << std::endl;
               _wd.getCopies()[ index ].setHostBaseAddress( host_copy_addr );
            }
         }
      }
      new (&_memCacheCopies[index]) MemCacheCopy(_wd, index);

      // o << "## " << (_wd.getCopies()[index].isInput() ? "in" : "") << (_wd.getCopies()[index].isOutput() ? "out" : "") << " " <<  _wd.getCopies()[index] << std::endl; 

      unsigned int predecessorsVersion;
      if ( _providedRegions.hasVersionInfoForRegion( _memCacheCopies[ index ]._reg, predecessorsVersion, _memCacheCopies[ index ]._locations ) ) {
         _memCacheCopies[ index ].setVersion( predecessorsVersion );
      }
      if ( _memCacheCopies[ index ].getVersion() != 0 ) {
         if ( _VERBOSE_CACHE ) { *(myThread->_file) << "WD " << _wd.getId() << " copy "<< index <<" got location info from predecessor "<<  _memCacheCopies[ index ]._reg.id << " got version " << _memCacheCopies[ index ].getVersion()<< " "; }
         _memCacheCopies[ index ]._locationDataReady = true;
      } else {
         if ( _VERBOSE_CACHE ) {
            *(myThread->_file) << "WD " << _wd.getId() << " copy " << index
                  << " got requesting location info to global directory for region "
                  << _memCacheCopies[index]._reg.id << " ";
         }
         _memCacheCopies[index].getVersionInfo();
      }
      if ( _VERBOSE_CACHE ) {
         for (NewLocationInfoList::const_iterator it =
               _memCacheCopies[index]._locations.begin();
               it != _memCacheCopies[index]._locations.end(); it++) {
            NewNewDirectoryEntryData *rsentry =
                  (NewNewDirectoryEntryData *) _memCacheCopies[index]._reg.key->getRegionData(
                        it->first);
            NewNewDirectoryEntryData *dsentry =
                  (NewNewDirectoryEntryData *) _memCacheCopies[index]._reg.key->getRegionData(
                        it->second);
            *(myThread->_file) << "<" << it->first << ": [" << *rsentry << "] ,"
                  << it->second << " : [" << *dsentry << "] > ";
         }
         *(myThread->_file) << std::endl;
      }

      if ( _wd.getParent() != NULL && _wd.getParent()->_mcontrol.ownsRegion( _memCacheCopies[ index ]._reg ) ) {
         /* do nothing, maybe here we can add a correctness check,
          * to ensure that the region is a subset of the Parent regions
          */
         //std::cerr << "I am " << _wd.getId() << " parent: " <<  _wd.getParent()->getId() << " NOT ADDING THIS OBJECT "; _memCacheCopies[ index ]._reg.key->printRegion(std::cerr, 1); std::cerr << " adding it to " << &_parentRegions << std::endl;
         _parentRegions.addRegion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ].getVersion() );
      } else { /* this should be for private data */
         if ( _wd.getParent() != NULL ) {
         //std::cerr << "I am " << _wd.getId() << " parent: " << _wd.getParent()->getId() << " ++++++ ADDING THIS OBJECT "; _memCacheCopies[ index ]._reg.key->printRegion(std::cerr, 1); std::cerr << std::endl;
            _wd.getParent()->_mcontrol._ownedRegions.addRegion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ].getVersion() );
         }
      }

#ifdef NANOS_RESILIENCY_ENABLED
      if ( _backupCacheCopies ) {
         new (&_backupCacheCopies[index]) MemCacheCopy(_wd, index);
         _backupCacheCopies[ index ].setVersion( _memCacheCopies[ index ].getVersion() );
         if( _wd.getCopies()[index].isInput() ) {
            _backupCacheCopies[index]._locations.insert(
                  _backupCacheCopies[index]._locations.end(),
                  _memCacheCopies[index]._locations.begin(),
                  _memCacheCopies[index]._locations.end()
               );
         }
      }
#endif

   }
   if ( _VERBOSE_CACHE ) { 
      *(myThread->_file)
            << " (preinit)END OF INITIALIZING MEMCONTROLLER for WD "
            << _wd.getId() << " "
            << (_wd.getDescription() != NULL ? _wd.getDescription() : "n/a")
            << " NUM COPIES " << _wd.getNumCopies() << std::endl;
   }
   _preinitialized = true;
}

void MemController::initialize( ProcessingElement &pe ) {
   if ( !_initialized ) {
      _pe = &pe;

      //NANOS_INSTRUMENT( InstrumentState inst2(NANOS_CC_CDIN); );

      if ( _pe->getMemorySpaceId() == 0 /* HOST_MEMSPACE_ID */) {
         _inOps = NEW HostAddressSpaceInOps( _pe, true );
      } else {
         _inOps = NEW SeparateAddressSpaceInOps( _pe, true, sys.getSeparateMemory( _pe->getMemorySpaceId() ) );
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
   if ( _pe->getMemorySpaceId() != 0 ) {
      result = sys.getSeparateMemory( _pe->getMemorySpaceId() ).prepareRegions( _memCacheCopies, _wd.getNumCopies(), _wd );
   }
   if ( result ) {
      //*(myThread->_file) << "++++ Succeeded allocation for wd " << _wd.getId() << std::endl;
      for ( unsigned int idx = 0; idx < _wd.getNumCopies(); idx += 1 ) {
         if ( _memCacheCopies[idx]._reg.key->getKeepAtOrigin() ) {
            //std::cerr << "WD " << _wd.getId() << " rooting to memory space " << _pe->getMemorySpaceId() << std::endl;
            _memCacheCopies[idx]._reg.setOwnedMemory( _pe->getMemorySpaceId() );
         }
      }
#ifdef NANOS_RESILIENCY_ENABLED
      if( sys.isResiliencyEnabled() ) {
         result &= sys.getBackupMemory().prepareRegions( _backupCacheCopies, _wd.getNumCopies(), _wd );
      }
#endif
   }
   _memoryAllocated = result;
   return result;
}

void MemController::copyDataIn() {
   ensure( _preinitialized == true, "MemController::copyDataIn: MemController not initialized!");
   ensure( _initialized == true, "MemController::copyDataIn: MemController not initialized!");
  
   if ( _VERBOSE_CACHE || sys.getVerboseCopies() ) {
      //if ( sys.getNetwork()->getNodeNum() == 0 ) {
         std::ostream &o = (*myThread->_file);
         o << "### copyDataIn wd " << std::dec << _wd.getId() << " (" << (_wd.getDescription()!=NULL?_wd.getDescription():"[no desc]")<< ") running on " << std::dec << _pe->getMemorySpaceId() << " ops: "<< (void *) _inOps << std::endl;
         for (unsigned int index = 0; index < _wd.getNumCopies(); index += 1) {
            NewNewDirectoryEntryData *d =
                  NewNewRegionDirectory::getDirectoryEntry(
                        *(_memCacheCopies[index]._reg.key),
                        _memCacheCopies[index]._reg.id);
            o << "## " << (_wd.getCopies()[index].isInput() ? "in" : "")
                  << (_wd.getCopies()[index].isOutput() ? "out" : "") << " ";
            _memCacheCopies[index]._reg.key->printRegion(o,
                  _memCacheCopies[index]._reg.id);
            if (d)
               o << " " << *d << std::endl;
            else
               o << " dir entry n/a" << std::endl;
            _memCacheCopies[index].printLocations(o);
         }
      //}
   }
   
   //if( sys.getNetwork()->getNodeNum()== 0)std::cerr << "MemController::copyDataIn for wd " << _wd.getId() << std::endl;
   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      _memCacheCopies[ index ].generateInOps( *_inOps, _wd.getCopies()[index].isInput(), _wd.getCopies()[index].isOutput(), _wd, index );
   }

   _inOps->issue( _wd );

   if ( _VERBOSE_CACHE || sys.getVerboseCopies() ) {
      if ( sys.getNetwork()->getNodeNum() == 0 ) {
         std::cerr << "### copyDataIn wd " << std::dec << _wd.getId() << " done" << std::endl;
      }
   }
#ifdef NANOS_RESILIENCY_ENABLED
   if ( sys.isResiliencyEnabled() && _wd.isRecoverable() && !_wd.isInvalid() ) {
      ensure( _backupOpsIn, "Backup ops array has not been initializedi!" );

   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_IN );
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

      bool queuedOps = false;
      for (unsigned int index = 0; index < _wd.getNumCopies(); index++) {
         if ( _wd.getCopies()[index].isInput() && _wd.getCopies()[index].isOutput() ) {
            // For inout parameters, make a temporary independent backup. We have to do this privately, without
            // the cache being noticed, because this backup is for exclusive use of this workdescriptor only.
            BackupManager& dev = (BackupManager&)sys.getBackupMemory().getCache().getDevice();

            std::size_t size = _wd.getCopies()[index].getSize();
            uint64_t host_addr = _wd.getCopies()[index].getFitAddress();
            uint64_t dev_addr = (uint64_t) dev.memAllocate(size, sys.getBackupMemory(), _wd, index);

            // FIXME Maybe it would be better to store is_private_backup_aborted value inside _backupInOutCopies...
            new (&_backupInOutCopies[index]) Chunk( dev_addr, host_addr, size );

            _is_private_backup_aborted |= !dev.rawCopyIn( dev_addr, host_addr, size, sys.getBackupMemory(), _wd );
         // Note: we dont want to make the regular backup for inouts, as children tasks' "in" 
         // parameters will always do the backup later if they exist no matter if now we perform the copy or not
         } else if ( _wd.getCopies()[index].isInput() ) {
            _backupCacheCopies[ index ].generateInOps( *_backupOpsIn, true, false, _wd, index);
            queuedOps = true;
         }
      }

      if( queuedOps )
         _backupOpsIn->issue(_wd);

   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
   }
#endif
   //NANOS_INSTRUMENT( inst2.close(); );
}

void MemController::copyDataOut( MemControllerPolicy policy ) {
   ensure( _preinitialized == true, "MemController::copyDataOut: MemController not initialized! Wd: ", _wd.getId());
   if( _initialized != true ) {
      *(myThread->_file) << "Memcontroller was not initialized, dumping stacktrace..." << std::endl;
      void *trace[50];
      int trace_size = 0;
      trace_size = backtrace(trace, 50);
      char **messages = backtrace_symbols(trace, trace_size);
      for( int i = 0; i < trace_size; i++ )
         *(myThread->_file) << messages[i] << std::endl;
   }
   ensure( _initialized == true, "MemController::copyDataOut: MemController not initialized! Wd: ", _wd.getId() );

   for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
      if ( _wd.getCopies()[index].isOutput() ) {
         if ( _wd.getParent() != NULL && _wd.getParent()->_mcontrol.ownsRegion( _memCacheCopies[ index ]._reg ) ) {
            WD &parent = *(_wd.getParent());
            for ( unsigned int parent_idx = 0; parent_idx < parent.getNumCopies(); parent_idx++ ) {
               if ( parent._mcontrol._memCacheCopies[parent_idx]._reg.contains( _memCacheCopies[index]._reg ) ) {
                  if ( parent._mcontrol._memCacheCopies[parent_idx].getChildrenProducedVersion() < _memCacheCopies[index].getChildrenProducedVersion() ) {
                     parent._mcontrol._memCacheCopies[parent_idx].setChildrenProducedVersion( _memCacheCopies[index].getChildrenProducedVersion() );
                  }
               }
            }
         }
      }
   }
   //for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
   //   if ( _wd.getCopies()[index].isInput() && _wd.getCopies()[index].isOutput() ) {
   //      _memCacheCopies[ index ]._reg.setLocationAndVersion( _pe->getMemorySpaceId(), _memCacheCopies[ index ].getVersion() + 1 );
   //   }
   //}
   if ( _VERBOSE_CACHE || sys.getVerboseCopies() ) { *(myThread->_file) << "### copyDataOut wd " << std::dec << _wd.getId() << " metadata set, not released yet" << std::endl; }

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
      //_outputDataReady = true;
   } else {
      _outOps = NEW SeparateAddressSpaceOutOps( _pe, false, true );

      for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
         _memCacheCopies[ index ].generateOutOps( &sys.getSeparateMemory( _pe->getMemorySpaceId() ), *_outOps, _wd.getCopies()[index].isInput(), _wd.getCopies()[index].isOutput(), _wd, index );
      }

      //if( sys.getNetwork()->getNodeNum()== 0)std::cerr << "MemController::copyDataOut for wd " << _wd.getId() << std::endl;
      _outOps->issue( _wd );
   }
#ifdef NANOS_RESILIENCY_ENABLED
   if (sys.isResiliencyEnabled() && _wd.isRecoverable() ) {

   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_OUT );
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

      for ( unsigned int index = 0; index < _wd.getNumCopies(); index += 1) {
         if( _wd.getCopies()[index].isInput() && _wd.getCopies()[index].isOutput() ) {
            // Inoutparameters' backup have to be cleaned: they are private
            BackupManager& dev = (BackupManager&)sys.getBackupMemory().getCache().getDevice();

            dev.memFree( _backupInOutCopies[index].getAddress(),
                          sys.getBackupMemory() );
         }
      }

      if( !_wd.isInvalid() ) {
         ensure( _backupOpsOut, "Backup ops array has not been initialized!" );

         bool queuedOps = false;
         for ( unsigned int index = 0; index < _wd.getNumCopies(); index += 1) {
            // Needed for CP input data
            if( _wd.getCopies()[index].isOutput() ) {
#ifdef NANOS_RESILIENCY_ENABLED
         AllocatedChunk *backup = _backupCacheCopies[index]._chunk;
         if( backup ) {// it seems this is never being executed
            CachedRegionStatus* entry = (CachedRegionStatus*)backup->getNewRegions()->getRegionData( backup->getAllocatedRegion().id );
            const bool invalid_entry = entry && !entry->isValid();
            if( invalid_entry ) {
               // If the entry is not valid, we set up its version to 0 so future backup overwrites aren't given any errors
               entry->resetVersion();
               _backupCacheCopies[ index ].setVersion( 0 );
               continue;
            }
         }
#endif
               _backupCacheCopies[index].setVersion( _memCacheCopies[ index ].getChildrenProducedVersion() );

               _backupCacheCopies[index]._locations.clear();
               _backupCacheCopies[index]._locations.push_back( std::pair<reg_t, reg_t>( _backupCacheCopies[index]._reg.id, _backupCacheCopies[index]._reg.id ) );
               _backupCacheCopies[index]._locationDataReady = true;

              _backupCacheCopies[index].generateInOps( *_backupOpsOut, true, false, _wd, index);
              queuedOps = true;
            }
         }

         if( queuedOps ) {
            _backupOpsOut->issue( _wd );
         }
      }

   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
   }

#endif
}

#ifdef NANOS_RESILIENCY_ENABLED
void MemController::restoreBackupData ( )
{
   ensure( _preinitialized == true, "MemController::restoreBackupData: MemController not initialized!");
   ensure( _initialized == true, "MemController::restoreBackupData: MemController not initialized!");
   ensure( _wd.isRecoverable(), "Cannot restore data of an unrecoverable task!" );
   ensure( !_wd.getNumCopies() || _backupCacheCopies, "There are no backup copies defined for this task." );

   NANOS_INSTRUMENT ( static nanos_event_key_t key = sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey("ft-checkpoint") );
   NANOS_INSTRUMENT ( nanos_event_value_t val = (nanos_event_value_t) NANOS_FT_CP_RESTORE );
   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseOpenBurstEvent ( key, val ) );

   if (_backupCacheCopies) {
      _restoreOps = NEW SeparateAddressSpaceOutOps( _pe, false, true);

      bool all_commited = true;
      try {
         for (unsigned int index = 0; index < _wd.getNumCopies(); index++) {
            const uintptr_t dev_addr  = _backupInOutCopies[index].getAddress();
            const size_t size         = _backupInOutCopies[index].getSize();
            const uintptr_t host_addr = _backupInOutCopies[index].getHostAddress();

            if (_wd.getCopies()[index].isInput()
                && _wd.getCopies()[index].isOutput() ) {
               // Inoutparameters have to be restored no matter whether they were corrupted or not (they may be dirty).
               BackupManager& dev = (BackupManager&)sys.getBackupMemory().getCache().getDevice();
               if( _is_private_backup_aborted ) {
                  throw InvalidatedRegionFound();
               } else {
                  all_commited &= dev.rawCopyOut( host_addr, dev_addr, size,
                                                  sys.getBackupMemory(), _wd );
               }
            } else if (_wd.getCopies()[index].isInput()) {
               _backupCacheCopies[index]._chunk->copyRegionToHost( *_restoreOps,
                     _backupCacheCopies[index]._reg.id,
                     _backupCacheCopies[index].getVersion(), _wd, index);

            }
         }
         _restoreOps->issue(_wd);

      } catch ( InvalidatedRegionFound const &err ) {
         all_commited = false;
      }
      if( !all_commited ) {
         if( _wd.getParent() == NULL ||               // If we haven't any ancestor to recover
               !_wd.getParent()->setInvalid( true ) ) // or we cannot find any ancestor which is recoverable
            fatal("Resiliency: Unrecoverable error found. "
                  "Found an invalidated backup and I haven't any ancestor which can recover the execution." );
      }
   }

   NANOS_INSTRUMENT ( sys.getInstrumentation()->raiseCloseBurstEvent ( key, val ) );
}
#endif

uint64_t MemController::getAddress( unsigned int index ) const {
   ensure( _preinitialized == true, "MemController::getAddress: MemController not preinitialized!");
   ensure( _initialized == true, "MemController::getAddress: MemController not initialized!");
   uint64_t addr = 0;
   //std::cerr << " _getAddress, reg: " << index << " key: " << (void *)_memCacheCopies[ index ]._reg.key << " id: " << _memCacheCopies[ index ]._reg.id << std::endl;
   if ( _pe->getMemorySpaceId() == 0 ) {
      addr = ((uint64_t) _wd.getCopies()[ index ].getBaseAddress());
   } else {
      addr = sys.getSeparateMemory( _pe->getMemorySpaceId() ).getDeviceAddress( _memCacheCopies[ index ]._reg, (uint64_t) _wd.getCopies()[ index ].getBaseAddress(), _memCacheCopies[ index ]._chunk );
      //std::cerr << "Hola: HostBaseAddr: " << (void*) _wd.getCopies()[ index ].getHostBaseAddress() << " BaseAddr: " << (void*)_wd.getCopies()[ index ].getBaseAddress() << std::endl;
      //if ( _wd.getCopies()[ index ].isRemoteHost() || _wd.getCopies()[ index ].getHostBaseAddress() == 0 ) {
      //   std::cerr << "Hola" << std::endl;
      //   addr = sys.getSeparateMemory( _pe->getMemorySpaceId() ).getDeviceAddress( _memCacheCopies[ index ]._reg, (uint64_t) _wd.getCopies()[ index ].getBaseAddress(), _memCacheCopies[ index ]._chunk );
      //} else {
      //   std::cerr << "Hola1" << std::endl;
      //   addr = sys.getSeparateMemory( _pe->getMemorySpaceId() ).getDeviceAddress( _memCacheCopies[ index ]._reg, (uint64_t) _wd.getCopies()[ index ].getHostBaseAddress(), _memCacheCopies[ index ]._chunk );
      //}
   }
   //std::cerr << "MemController::getAddress " << (_wd.getDescription()!=NULL?_wd.getDescription():"[no desc]") <<" index: " << index << " addr: " << (void *)addr << std::endl;
   return addr;
}

void MemController::getInfoFromPredecessor( MemController const &predecessorController ) {
   for( unsigned int index = 0; index < predecessorController._wd.getNumCopies(); index += 1) {
      unsigned int version = predecessorController._memCacheCopies[ index ].getChildrenProducedVersion();
      //(*myThread->_file) << "getInfoFromPredecessor[ " << _wd.getId() << " : "<< _wd.getDescription()<< " key: " << (void*)predecessorController._memCacheCopies[ index ]._reg.key << " ] adding version " << version << " from wd " << predecessorController._wd.getId() << " : " << predecessorController._wd.getDescription() << " : " << index << std::endl;
      _providedRegions.addRegion( predecessorController._memCacheCopies[ index ]._reg, version );
   }
#if 0
   _provideLock.acquire();
   for( unsigned int index = 0; index < predecessorController._wd.getNumCopies(); index += 1) {
      std::map< reg_t, unsigned int > &regs = _providedRegions[ predecessorController._memCacheCopies[ index ]._reg.key ];
      std::map< reg_t, unsigned int >::iterator elem = regs.find( predecessorController._memCacheCopies[ index ]._reg.id );

      unsigned int version = predecessorController._memCacheCopies[ index ].getVersion() + ( predecessorController._wd.getCopies()[index].isOutput() ? 1 : 0 );
      //(*myThread->_file) << "getInfoFromPredecessor[ " << _wd.getId() << " : "<< _wd.getDescription()<< " key: " << (void*)predecessorController._memCacheCopies[ index ]._reg.key << " ] adding version " << version << " from wd " << predecessorController._wd.getId() << " : " << predecessorController._wd.getDescription() << " : " << index << std::endl;
      if ( elem != regs.end() ) {
         if ( elem->second < version ) {
            regs[ elem->first ] = version;
         }
      } else {
         regs[ predecessorController._memCacheCopies[ index ]._reg.id ] = version;
      }
      //std::cerr << "from wd " << predecessorController._wd.getId() << " to wd " << _wd.getId()  << " provided data for copy " << index << " reg ("<<predecessorController._memCacheCopies[ index ]._reg.key<<"," << predecessorController._memCacheCopies[ index ]._reg.id << ") with version " << ( ( predecessorController._wd.getCopies()[index].isOutput() ) ? predecessorController._memCacheCopies[ index ].getVersion() + 1 : predecessorController._memCacheCopies[ index ].getVersion() ) << " isOut "<< predecessorController._wd.getCopies()[index].isOutput()<< " isIn "<< predecessorController._wd.getCopies()[index].isInput() << std::endl;
   }
   _provideLock.release();
#endif
}

bool MemController::isDataReady ( WD const &wd )
{
   ensure( _preinitialized == true, "MemController::isDataReady: MemController not initialized!");
   if (_initialized) {
      if (!_inputDataReady) {
         _inputDataReady = _inOps->isDataReady(wd);
#if NANOS_RESILIENCY_ENABLED
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

bool MemController::isOutputDataReady( WD const &wd ) {
   ensure( _preinitialized == true, "MemController::isOutputDataReady: MemController not initialized!");
   if ( _initialized ) {
      if ( _outOps && !_outputDataReady ) {
         _outputDataReady = _outOps->isDataReady( wd );
         if ( _outputDataReady ) {
            if ( _VERBOSE_CACHE ) { *(myThread->_file) << "Output data is ready for wd " << _wd.getId() << " obj " << (void *)_outOps << std::endl; }

            sys.getSeparateMemory( _pe->getMemorySpaceId() ).releaseRegions( _memCacheCopies, _wd.getNumCopies(), _wd ) ;
            //for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
            //   sys.getSeparateMemory( _pe->getMemorySpaceId() ).releaseRegions( _memCacheCopies, _wd.getNumCopies(), _wd ) ;
            //}
         }
      } else if ( !_outOps ) {
         // Maybe the output data wasnt ready because theres no output data
         _outputDataReady = true;
      }
#if NANOS_RESILIENCY_ENABLED
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
bool MemController::isDataRestored( WD const &wd ) {
   ensure( _preinitialized == true, "MemController::isDataRestored: MemController not initialized!");
   ensure( _wd.isRecoverable(), "Task is not recoverable. There wasn't any data to be restored. ");
   if ( _initialized ) {
      if ( !_dataRestored ) {
         _dataRestored = _restoreOps->isDataReady( wd );
         if ( _dataRestored ) {
            if ( _VERBOSE_CACHE ) { *(myThread->_file) << "Restored data is ready for wd " << _wd.getId() << " obj " << (void *)_restoreOps << std::endl; }

            /* Is this the data invalidation? I don't think so
            for ( unsigned int index = 0; index < _wd.getNumCopies(); index++ ) {
               sys.getBackupMemory().releaseRegion( _backupCacheCopies[ index ]._reg, _wd, index, _backupCacheCopies[ index ]._policy ) ;
            }*/
         }
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
   size_t transferred = 0;
   if ( _inOps != NULL )
      transferred += _inOps->getAmountOfTransferredData();
   return transferred;
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
         _memCacheCopies[ index ]._reg.setLocationAndVersion( _pe, _pe->getMemorySpaceId(), newVersion ); //update directory
         _memCacheCopies[ index ].setChildrenProducedVersion( newVersion );

         if ( _pe->getMemorySpaceId() != 0 /* HOST_MEMSPACE_ID */) {
            sys.getSeparateMemory( _pe->getMemorySpaceId() ).setRegionVersion( _memCacheCopies[ index ]._reg, _memCacheCopies[ index ].getVersion() + 1, _wd, index );
         }
      } else if ( _wd.getCopies()[index].isInput() ) {
         _memCacheCopies[ index ].setChildrenProducedVersion( _memCacheCopies[ index ].getVersion() );
      }
   }
}

bool MemController::hasObjectOfRegion( global_reg_t const &reg ) {
   return _ownedRegions.hasObjectOfRegion( reg );
}

}
