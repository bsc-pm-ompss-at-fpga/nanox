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

#ifdef STANDALONE_TEST

#ifdef message
#undef message
#define message(x)
#else
#define message(x)
#endif
#ifdef ensure
#undef ensure
#define ensure(x,y)
#else
#define ensure(x,y)
#endif
#ifndef NEW
#define NEW new
#endif

#else
#include "basethread.hpp"
#include "debug.hpp"
#endif

#include "newregiondirectory.hpp"
#include "hashfunction_decl.hpp"
#include "regiondict.hpp"
#include "os.hpp"

#if VERBOSE_CACHE
 #define _VERBOSE_CACHE 1
#else
 #define _VERBOSE_CACHE 0
 //#define _VERBOSE_CACHE ( sys.getNetwork()->getNodeNum() == 0 )
#endif

using namespace nanos;

std::ostream & nanos::operator<< (std::ostream &o, nanos::NewNewDirectoryEntryData const &ent)
{
   //o << "WL: " << ent._writeLocation << " V: " << ent.getVersion() << " Locs: ";
   o << " V: " << ent.getVersion() << " Locs: ";
   for ( std::set<memory_space_id_t>::iterator it = ent._location.begin(); it != ent._location.end(); it++ ) {
      o << *it << " ";
   }
   return o;
}

NewNewRegionDirectory::HashBucket::HashBucket() : _lock(), _bobjects() { }

NewNewRegionDirectory::HashBucket::HashBucket( NewNewRegionDirectory::HashBucket const &hb ) : _lock(), _bobjects( hb._bobjects ) { }

NewNewRegionDirectory::HashBucket &NewNewRegionDirectory::HashBucket::operator=( NewNewRegionDirectory::HashBucket const &hb ) {
   _bobjects = hb._bobjects;
   return *this;
}

#define HASH_BUCKETS 256

NewNewRegionDirectory::NewNewRegionDirectory() : _objects( HASH_BUCKETS, HashBucket() ) {}

GlobalRegionDictionary *NewNewRegionDirectory::getRegionDictionaryRegisterIfNeeded( CopyData const &cd ) {
   uint64_t objectAddr = ( cd.getHostBaseAddress() == 0 ? ( uint64_t ) cd.getBaseAddress() : cd.getHostBaseAddress() );

   unsigned int key = ( jen_hash( objectAddr ) & (HASH_BUCKETS-1) );
   HashBucket &hb = _objects[ key ];

   hb._lock.acquire();
   std::map< uint64_t, GlobalRegionDictionary * >::iterator it = hb._bobjects.lower_bound( objectAddr );
   if ( it == hb._bobjects.end() || hb._bobjects.key_comp()( objectAddr, it->first) ) {
      it = hb._bobjects.insert( it, std::map< uint64_t, GlobalRegionDictionary * >::value_type( objectAddr, NEW GlobalRegionDictionary( cd ) ) );
      NewNewDirectoryEntryData *entry = getDirectoryEntry( *(it->second), 1 );
      if ( entry == NULL ) {
         entry = NEW NewNewDirectoryEntryData();
         //entry->addAccess( 0, 1 );
         it->second->setRegionData( 1, entry );
      }
   }
   hb._lock.release();
   return it->second;
}

GlobalRegionDictionary *NewNewRegionDirectory::getRegionDictionary( CopyData const &cd ) const {
   uint64_t objectAddr = ( cd.getHostBaseAddress() == 0 ? ( uint64_t ) cd.getBaseAddress() : cd.getHostBaseAddress() );
   return getRegionDictionary( objectAddr );
}

GlobalRegionDictionary *NewNewRegionDirectory::getRegionDictionary( uint64_t objectAddr ) const {
   unsigned int key = ( jen_hash( objectAddr ) & (HASH_BUCKETS-1) );
   HashBucket const &hb = _objects[ key ];

   std::map< uint64_t, GlobalRegionDictionary * >::const_iterator it = hb._bobjects.lower_bound( objectAddr );
   if ( it == hb._bobjects.end() || hb._bobjects.key_comp()( objectAddr, it->first) ) {
     *(myThread->_file) << "Error, CopyData object not registered in the RegionDictionary " << (void *) objectAddr << std::endl;
     printBt( *(myThread->_file) );
     fatal("can not continue");
   }
   return it->second;
}

// reg_t NewNewRegionDirectory::tryGetLocation( RegionDirectoryKey dict, CopyData const &cd, NewLocationInfoList &missingParts, unsigned int &version, WD const &wd ) {
//    reg_t reg = 0;
//    if ( dict->tryLock() ) {
//    //NANOS_INSTRUMENT( InstrumentState inst1(NANOS_POST_OUTLINE_WORK2 ); );
//     reg = _getLocation( dict, cd, missingParts, version, wd );
//    //NANOS_INSTRUMENT( inst1.close(); );
//       dict->unlock();
//    }
//    return reg;
// }

void NewNewRegionDirectory::tryGetLocation( RegionDirectoryKey dict, reg_t reg, NewLocationInfoList &missingParts, unsigned int &version, WD const &wd ) {
   NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) dict->getRegionData( reg );

   if ( entry == NULL ) {
      entry = NEW NewNewDirectoryEntryData();
      //entry->addAccess( 0, 1 );
      dict->setRegionData( reg, entry );
   }
   if ( dict->getVersion() != entry->getVersion() || entry->getVersion() == 1 ) {
      if ( dict->tryLock() ) {
         //NANOS_INSTRUMENT( InstrumentState inst1(NANOS_POST_OUTLINE_WORK2 ); );
         __getLocation( dict, reg, missingParts, version, wd );
         //NANOS_INSTRUMENT( inst1.close(); );
         dict->unlock();
      }
   } else {
      //std::cerr << "Avoid checking of global directory because dict Version == reg Version." << std::endl; 
      missingParts.push_back( std::make_pair( reg, reg ) );
      version = dict->getVersion();
   }
}

// reg_t NewNewRegionDirectory::_getLocation( RegionDirectoryKey dict, CopyData const &cd, NewLocationInfoList &missingParts, unsigned int &version, WD const &wd )
// {
//    reg_t reg = 0;
//    ensure( missingParts.empty(), "Non empty list provided." );
//    missingParts.clear();
//    //sys.getMasterRegionDirectory().print();
// 
//    reg = dict->registerRegion( cd, missingParts, version );
// 
//    for ( std::list< std::pair< reg_t, reg_t > >::iterator it = missingParts.begin(); it != missingParts.end(); it++ ) {
//       if ( it->first != it->second ) {
//          NewNewDirectoryEntryData *firstEntry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->first );
//          NewNewDirectoryEntryData *secondEntry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->second );
//          if ( firstEntry == NULL ) {
//             if ( secondEntry != NULL ) {
//       //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"(null),"<< it->second <<"("<< *secondEntry<<")"<<std::endl;
//                firstEntry = NEW NewNewDirectoryEntryData( *secondEntry );
//       //if( sys.getNetwork()->getNodeNum() ) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, null, !null"<<std::endl;
//           } else {
//       //if( sys.getNetwork()->getNodeNum() ) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, null, null"<<std::endl;
//       //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"(null),"<< it->second <<"(null)"<<std::endl;
//                firstEntry = NEW NewNewDirectoryEntryData();
//                firstEntry->addAccess( 0, 1 );
//                secondEntry = NEW NewNewDirectoryEntryData();
//                secondEntry->addAccess( 0, 1 );
//                dict->setRegionData( it->second, secondEntry );
//             }
//             dict->setRegionData( it->first, firstEntry );
//          } else {
//             if ( secondEntry != NULL ) {
//       //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"("<<*firstEntry<<"),"<< it->second <<"("<< *secondEntry<<")"<<std::endl;
//       //if( wd.getId() == 27) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, !null, !null"<<std::endl;
// 
//                *firstEntry = *secondEntry;
//             } else {
//                std::cerr << "Dunno what to do..."<<std::endl;
//             }
//          }
//       } else {
//       //if( wd.getId() == 27) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, same id"<<std::endl;
//          NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->first );
//       //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"( " << (void*) entry << " ),"<< it->second <<"( )"<<std::endl;
//          if ( entry == NULL ) {
//             entry = NEW NewNewDirectoryEntryData();
//             entry->addAccess( 0, 1 );
//             dict->setRegionData( it->first, entry );
//          }
//       }
//    }
// 
//    return reg;
// }

void NewNewRegionDirectory::__getLocation( RegionDirectoryKey dict, reg_t reg, NewLocationInfoList &missingParts, unsigned int &version, WD const &wd )
{
   ensure( missingParts.empty(), "Non empty list provided." );
   missingParts.clear();
   //sys.getMasterRegionDirectory().print();

   //std::cerr << "1. Leaf for reg " << reg << " leaf is " << (void*) dict->getRegionNode(reg) << " dict is "  << (void *) &dict << std::endl;
   dict->registerRegion( reg, missingParts, version );
   //std::cerr << "2. Leaf for reg " << reg << " leaf is " << (void*) dict->getRegionNode(reg) << " dict is "  << (void *) &dict << std::endl;

   for ( std::list< std::pair< reg_t, reg_t > >::iterator it = missingParts.begin(); it != missingParts.end(); it++ ) {
      if ( it->first != it->second ) {
         NewNewDirectoryEntryData *firstEntry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->first );
         NewNewDirectoryEntryData *secondEntry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->second );
         if ( firstEntry == NULL ) {
            if ( secondEntry != NULL ) {
       //if (1 ) { std::cerr << (void *)dict << " INIT DATA ENTRY FOR REG " << it->first << " USING REG " << it->second<< std::endl; }
      //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"(null),"<< it->second <<"("<< *secondEntry<<")"<<std::endl;
               firstEntry = NEW NewNewDirectoryEntryData( *secondEntry );
      //if( sys.getNetwork()->getNodeNum() ) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, null, !null"<<std::endl;
          } else {
       //if (1 ) { std::cerr << (void *)dict << " INIT DATA ENTRY FOR REG " << it->first << " USING NEW VAL, second also " << it->second<< std::endl; }
      //if( sys.getNetwork()->getNodeNum() ) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, null, null"<<std::endl;
      //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"(null),"<< it->second <<"(null)"<<std::endl;
               firstEntry = NEW NewNewDirectoryEntryData();
               //firstEntry->addAccess( 0, 1 );
               secondEntry = NEW NewNewDirectoryEntryData();
               //secondEntry->addAccess( 0, 1 );
               dict->setRegionData( it->second, secondEntry );
            }
            dict->setRegionData( it->first, firstEntry );
         } else {
            if ( secondEntry != NULL ) {
      //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"("<<*firstEntry<<"),"<< it->second <<"("<< *secondEntry<<")"<<std::endl;
      //if( wd.getId() == 27) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, !null, !null"<<std::endl;
       //if (1 ) { std::cerr << (void *)dict << " INIT DATA ENTRY FOR REG " << it->first << " COPY FROM " << it->second << std::endl; }

               *firstEntry = *secondEntry;
            } else {
               std::cerr << "Dunno what to do..."<<std::endl;
            }
         }
      } else {
      //if( wd.getId() == 27) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] case, same id"<<std::endl;
         NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) dict->getRegionData( it->first );
      //if( sys.getNetwork()->getNodeNum() == 0) std::cerr <<"["<<sys.getNetwork()->getNodeNum()<< "] getLocation: " << it->first <<"( " << (void*) entry << " ),"<< it->second <<"( )"<<std::endl;
         if ( entry == NULL ) {
       //if (1 ) { std::cerr << (void *)dict << " INIT DATA ENTRY FOR REG " << it->first << " USING NEW VAL, same second " << std::endl; }
            entry = NEW NewNewDirectoryEntryData();
            //entry->addAccess( 0, 1 );
            dict->setRegionData( it->first, entry );
         } else {
       //if (1 ) { std::cerr << (void *)dict << " ENTRY EXISTS FOR REG " << it->first << std::endl; }
         }
      }
   }

   //if ( wd.getId() == 27 ) {
   //std::cerr << "Git region " << reg << std::endl;
   //for ( std::list< std::pair< reg_t, reg_t > >::iterator it = missingParts.begin(); it != missingParts.end(); it++ ) {
   //   std::cerr << "\tPart " << it->first << " comes from " << it->second << " dict " << (void *) dict << std::endl;
   //}
   //std::cerr <<" end of getLocation "<< std::endl;
   //}

}

void NewNewRegionDirectory::addAccess( RegionDirectoryKey dict, reg_t id, ProcessingElement *pe, memory_space_id_t loc, unsigned int version )
{
   if (dict->getVersion() < version ) dict->setVersion( version );
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   //if(sys.getNetwork()->getNodeNum() > 0) { std::cerr << dict << " ADDING ACCESS reg " << id << " version " << version << " TO LOC " << memorySpaceId << " entry: " << *regEntry << std::endl; }
   //if(sys.getNetwork()->getNodeNum() == 0) { std::cerr << dict << " ADDING ACCESS reg " << id << " version " << version << " TO LOC " << memorySpaceId << " entry: " << *regEntry << std::endl; }
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "ADD ACCESS "<< (void*) dict << ":" << id << " to "<< memorySpaceId << std::endl;
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << *regEntry<<std::endl;
   regEntry->addAccess( pe, loc, version );
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << *regEntry<<std::endl;
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "---------" << std::endl;
}

void NewNewRegionDirectory::addRootedAccess( RegionDirectoryKey dict, reg_t id, memory_space_id_t loc, unsigned int version )
{
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   regEntry->addRootedAccess( loc, version );
}

//void NewNewRegionDirectory::addAccessRegisterIfNeeded( RegionDirectoryKey dict, reg_t id, unsigned int memorySpaceId, unsigned int version )
//{
//   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
//   if ( regEntry == NULL ) {
//     regEntry = NEW NewNewDirectoryEntryData();
//     dict->setRegionData( id, regEntry );
//   }
//   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "ADD ACCESS if needed "<< (void*) dict << ":" << id << " to "<< memorySpaceId << std::endl;
//   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << *regEntry<<std::endl;
//   //if(sys.getNetwork()->getNodeNum() > 0) { std::cerr << dict << " ADDING ACCESS reg " << id << " version " << version << " TO LOC " << memorySpaceId << " entry: " << *regEntry << std::endl; }
//   regEntry->addAccess( memorySpaceId, version );
//   //if(sys.getNetwork()->getNodeNum() == 0) { std::cerr << dict << " ADDING ACCESS reg " << id << " version " << version << " TO LOC " << memorySpaceId << " entry: " << *regEntry << std::endl; }
//   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << *regEntry<<std::endl;
//   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "---------" << std::endl;
//}

NewNewDirectoryEntryData *NewNewRegionDirectory::getDirectoryEntry( GlobalRegionDictionary &dict, reg_t id ) {
   NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) dict.getRegionData( id );
   //if ( !entry ) {
   //   entry = NEW NewNewDirectoryEntryData();
   //   dict.setRegionData( id, entry );
   //}
   return entry;
}

bool NewNewRegionDirectory::delAccess( RegionDirectoryKey dict, reg_t id, memory_space_id_t memorySpaceId ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   bool res;
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "DEL ACCESS "<< (void*) dict << ":" << id << " from "<< memorySpaceId << std::endl;
   //if (sys.getNetwork()->getNodeNum() == 0 && regEntry )std::cerr << *regEntry<<std::endl;
   res = regEntry->delAccess( memorySpaceId );
   //if (sys.getNetwork()->getNodeNum() == 0 && regEntry )std::cerr << *regEntry<<std::endl;
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "---------" << std::endl;
   return res;
}

bool NewNewRegionDirectory::isOnlyLocated( RegionDirectoryKey dict, reg_t id, ProcessingElement *pe ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   bool res;
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "DEL ACCESS "<< (void*) dict << ":" << id << " from "<< memorySpaceId << std::endl;
   //if (sys.getNetwork()->getNodeNum() == 0 && regEntry )std::cerr << *regEntry<<std::endl;
   res = ( ( regEntry->isLocatedIn( pe ) ) && ( regEntry->getNumLocations() == 1 ) );
   //if (sys.getNetwork()->getNodeNum() == 0 && regEntry )std::cerr << *regEntry<<std::endl;
   //if (sys.getNetwork()->getNodeNum() == 0 )std::cerr << "---------" << std::endl;
   return res;
}

bool NewNewRegionDirectory::isOnlyLocated( RegionDirectoryKey dict, reg_t id, memory_space_id_t loc ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   bool res;
   res = ( ( regEntry->isLocatedIn( loc ) ) && ( regEntry->getNumLocations() == 1 ) );
   return res;
}

//void NewNewRegionDirectory::invalidate( RegionDirectoryKey dict, reg_t id ) {
//   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
//   regEntry->invalidate();
//}

//bool NewNewRegionDirectory::hasBeenInvalidated( RegionDirectoryKey dict, reg_t id ) {
//   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
//   return regEntry->hasBeenInvalidated();
//}

void NewNewRegionDirectory::updateFromInvalidated( RegionDirectoryKey dict, reg_t id, reg_t from ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   NewNewDirectoryEntryData *fromEntry = getDirectoryEntry( *dict, from );
   *regEntry = *fromEntry;
}

void NewNewRegionDirectory::print() const {
   for ( std::vector< HashBucket >::const_iterator bit = _objects.begin(); bit != _objects.end(); bit++ ) {
      HashBucket const &hb = *bit;

   for ( std::map< uint64_t, GlobalRegionDictionary * >::const_iterator it = hb._bobjects.begin(); it != hb._bobjects.end(); it++ ) {
      std::cerr <<"Object "<< (void*)it->second << std::endl;
      for (reg_t i = 1; i < it->second->getMaxRegionId(); i++ ) {
         NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) it->second->getRegionData( i );
         if ( !entry ) {
            std::cerr << "\t" << i << " "; it->second->printRegion( std::cerr, i ); std::cerr << " : null " << std::endl;
         } else {
            std::cerr << "\t" << i << " "; it->second->printRegion( std::cerr, i ); std::cerr << " : ("<< entry <<") "<< *entry << std::endl;
         }
      }
   }

   }
}


unsigned int NewNewRegionDirectory::getVersion( RegionDirectoryKey dict, reg_t id, bool increaseVersion ) {
   NewNewDirectoryEntryData *entry = getDirectoryEntry( *dict, id );
   return entry->getVersion( increaseVersion );
}

bool NewNewRegionDirectory::isLocatedIn( RegionDirectoryKey dict, reg_t id, ProcessingElement *pe, unsigned int version ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   //std::cerr << dict << " IS LOCATED " << id << " in loc " << loc <<" entry is " <<*regEntry << " version requested " << version<< std::endl;
   return regEntry->isLocatedIn( pe, version );
}

// bool NewNewRegionDirectory::isLocatedIn( RegionDirectoryKey dict, reg_t id, memory_space_id_t loc, unsigned int version ) {
//    NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
//    //std::cerr << dict << " IS LOCATED " << id << " in loc " << loc <<" entry is " <<*regEntry << " version requested " << version<< std::endl;
//    return regEntry->isLocatedIn( loc, version );
// }

bool NewNewRegionDirectory::isLocatedIn( RegionDirectoryKey dict, reg_t id, ProcessingElement *pe ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   //std::cerr << dict << " IS LOCATED " << id << " in loc " << loc <<" entry is " <<*regEntry  << std::endl;
   return (regEntry) ? regEntry->isLocatedIn( pe ) : 0;
}

bool NewNewRegionDirectory::isLocatedIn( RegionDirectoryKey dict, reg_t id, memory_space_id_t loc ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   //std::cerr << dict << " IS LOCATED " << id << " in loc " << loc <<" entry is " <<*regEntry  << std::endl;
   return (regEntry) ? regEntry->isLocatedIn( loc ) : 0;
}

unsigned int NewNewRegionDirectory::getFirstLocation( RegionDirectoryKey dict, reg_t id ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   return regEntry->getFirstLocation();
}

// bool NewNewRegionDirectory::hasWriteLocation( RegionDirectoryKey dict, reg_t id ) {
//    NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
//    return regEntry->hasWriteLocation();
// }

// unsigned int NewNewRegionDirectory::getWriteLocation( RegionDirectoryKey dict, reg_t id ) {
//    NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
//    return regEntry->getWriteLocation();
// }

GlobalRegionDictionary &NewNewRegionDirectory::getDictionary( CopyData const &cd ) const {
   return *getRegionDictionary( cd );
}

void NewNewRegionDirectory::synchronize( WD &wd ) {
   //std::cerr << "SYNC DIR with wd " << wd.getId() << std::endl;
   //int c = 0;
   //print();
   SeparateAddressSpaceOutOps outOps( myThread->runningOn(), true, false );
   std::map< GlobalRegionDictionary *, std::set< memory_space_id_t > > locations;
   std::map< uint64_t, std::map< uint64_t, GlobalRegionDictionary * > * > objects_to_clear;

   for ( std::vector< HashBucket >::iterator bit = _objects.begin(); bit != _objects.end(); bit++ ) {
      HashBucket &hb = *bit;

   for ( std::map< uint64_t, GlobalRegionDictionary *>::iterator it = hb._bobjects.begin(); it != hb._bobjects.end(); it++ ) {
      //std::cerr << "==================  start object " << ++c << "("<< it->second <<") ================="<<std::endl;
      //if ( it->second->getKeepAtOrigin() ) {
      //   std::cerr << "Object " << it->second << " Keep " << std::endl;
      //}
      if ( !wd._mcontrol.hasObjectOfRegion( global_reg_t( 1, it->second ) ) ) {
         if ( sys.getVerboseCopies() ) {
            std::ostream &o = (*myThread->_file);
            o << "Not synchronizing this object! "; it->second->printRegion( o, 1 ); o << std::endl;
         }
         continue;
      }
      if ( it->second->getKeepAtOrigin() ) continue;

      std::list< std::pair< reg_t, reg_t > > missingParts;
      unsigned int version = 0;
      //double tini = OS::getMonotonicTime();
      /*reg_t lol =*/ it->second->registerRegion(1, missingParts, version, true);
      //double tfini = OS::getMonotonicTime();
      //std::cerr << __FUNCTION__ << " addRegion time " << (tfini-tini) << std::endl;
      //std::cerr << "Missing parts are: (want version) "<< version << " got " << lol << " { ";
      //for ( std::list< std::pair< reg_t, reg_t > >::iterator mit = missingParts.begin(); mit != missingParts.end(); mit++ ) {
      //   std::cerr <<"("<< mit->first << "," << mit->second << ") ";
      //}
      //std::cerr << "}"<<std::endl;

      objects_to_clear.insert( std::make_pair( it->first, &hb._bobjects ) );

      for ( std::list< std::pair< reg_t, reg_t > >::iterator mit = missingParts.begin(); mit != missingParts.end(); mit++ ) {
         //std::cerr << "sync region " << mit->first << " : "<< ( void * ) it->second->getRegionData( mit->first ) <<" with second reg " << mit->second << " : " << ( void * ) it->second->getRegionData( mit->second )<< std::endl;
         if ( mit->first == mit->second ) {
            global_reg_t reg( mit->first, it->second );
            if ( !reg.isRooted() ) { //ignore regions rooted to a certain location
               if ( !reg.isLocatedIn( 0 ) ) {
                  DeviceOps *thisOps = reg.getDeviceOps();
                  if ( thisOps->addCacheOp( /* debug: */ &wd ) ) {
                     NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) reg.key->getRegionData( reg.id  );
                     if ( _VERBOSE_CACHE ) {
                        std::cerr << "f SYNC REGION! "; reg.key->printRegion( std::cerr, reg.id );
                        if ( entry ) std::cerr << " " << *entry << std::endl;
                        else std::cerr << " nil " << std::endl; 
                     }
                     //std::cerr << " reg is in: " << reg.getFirstLocation() << std::endl;
                     outOps.addOp( &sys.getSeparateMemory( reg.getFirstLocation() ), reg, reg.getVersion(), thisOps, NULL, (unsigned int)0xdeadbeef ); //Out op synchronize
                     outOps.insertOwnOp( thisOps, reg, reg.getVersion()+1, 0 ); //increase version to invalidate the device copy
                  } else {
                     outOps.getOtherOps().insert( thisOps );
                  }
                  //regEntry->addAccess( 0, regEntry->getVersion() );
               }
               // another mechanism to inval data: else if ( reg.getNumLocations() > 1 ) {
               // another mechanism to inval data:    //std::cerr << " have too upgrade host region" << std::endl;
               // another mechanism to inval data:    reg.setLocationAndVersion( 0, reg.getVersion()+1 ); //increase version to invalidate the device copy
               // another mechanism to inval data: }

               // aggregate the locations, later, we will invalidate the full object from those locations
               locations[it->second].insert(reg.getLocations().begin(), reg.getLocations().end()); //this requires delayedCommit = yes in the ops object!! FIXME
            } else {
               //objects_to_clear.insert( std::make_pair( it->first, &hb._bobjects ) ); //FIXME: objects may be added later
               objects_to_clear.erase( it->first );
            }
         } else {
            global_reg_t region_shape( mit->first, it->second );
            global_reg_t data_source( mit->second, it->second );
            if ( !data_source.isRooted() ) { //ignore regions rooted to a certain location
               if ( !data_source.isLocatedIn( 0 ) ) {
                  //std::cerr << "FIXME: I should sync region! " << region_shape.id << " "; region_shape.key->printRegion( region_shape.id ); std::cerr << std::endl;
                  //std::cerr << "FIXME: I should sync region! " << data_source.id << " "; data_source.key->printRegion( data_source.id ); std::cerr << std::endl;
                  region_shape.initializeGlobalEntryIfNeeded();
                  DeviceOps *thisOps = region_shape.getDeviceOps();
                  if ( thisOps->addCacheOp( /* debug: */ &wd ) ) {
                     NewNewDirectoryEntryData *entry = ( NewNewDirectoryEntryData * ) data_source.key->getRegionData( data_source.id  );
                     if ( _VERBOSE_CACHE ) {
                        std::cerr << " SYNC REGION! "; region_shape.key->printRegion( std::cerr, region_shape.id );
                        if ( entry ) std::cerr << " " << *entry << std::endl;
                        else std::cerr << " nil " << std::endl; 
                     }
                     //std::cerr << " reg is in: " << reg.getFirstLocation() << std::endl;
                     outOps.addOp( &sys.getSeparateMemory( data_source.getFirstLocation() ), region_shape, data_source.getVersion(), thisOps, NULL, (unsigned int)0xdeadbeef ); //Out op synchronize
                     outOps.insertOwnOp( thisOps, region_shape, data_source.getVersion()+1, 0 ); //increase version to invalidate the device copy
                  } else {
                     outOps.getOtherOps().insert( thisOps );
                  }
                  //regEntry->addAccess( 0, regEntry->getVersion() );
               }
            } else {
               objects_to_clear.erase( it->first );
            }
         }
      }

      //std::cerr << "=============================================================="<<std::endl;
   }

   }
   outOps.issue( *( (WD *) NULL ) );
   while ( !outOps.isDataReady( wd ) ) { myThread->idle(); }

   //std::cerr << "taskwait flush, wd (" << wd.getId() << ") depth is " << wd.getDepth() << " this node is " <<  sys.getNetwork()->getNodeNum() << std::endl;
   //printBt();
   if ( wd.getDepth() == 0 ) {
      // invalidate data on devices
      //for ( std::map< GlobalRegionDictionary *, std::set< memory_space_id_t > >::const_iterator it = locations.begin(); it != locations.end(); it++ ) {
      //   for ( std::set< memory_space_id_t >::const_iterator locIt = it->second.begin(); locIt != it->second.end(); locIt++ ) {
      //      if ( *locIt != 0 ) {
      //         std::cerr << "inval object " << it->first << " (addr " << (void*) it->first->getKeyBaseAddress() << ") from mem space " << *locIt <<", wd (" << wd.getId() << ") depth is "<< wd.getDepth() <<" this node is "<< sys.getNetwork()->getNodeNum() << std::endl;
      //         sys.getSeparateMemory( *locIt ).invalidate( global_reg_t( 1, it->first ) );
      //      }
      //   }
      //}
      for ( std::map< uint64_t, std::map< uint64_t, GlobalRegionDictionary * > * >::iterator it = objects_to_clear.begin(); it != objects_to_clear.end(); it++ ) {
         for ( memory_space_id_t id = 1; id <= sys.getSeparateMemoryAddressSpacesCount(); id++ ) {
            sys.getSeparateMemory( id ).invalidate( global_reg_t( 1, (*it->second)[it->first] ) );
         }
      }

      //clear objects from directory
      for ( std::map< uint64_t, std::map< uint64_t, GlobalRegionDictionary * > * >::iterator it = objects_to_clear.begin(); it != objects_to_clear.end(); it++ ) {
         GlobalRegionDictionary *obj = (*it->second)[it->first];
         sys.getNetwork()->deleteDirectoryObject( obj );
         //std::cerr << "delete and unregister dict (address) " << (void *) *it << " (key) " << (void *) _objects[ *it ] << std::endl;
         //std::cerr << "delete and unregister dict (address) " << (void *) it->first << " (key) " << (void *) obj << std::endl;
         delete obj; 
         it->second->erase( it->first );
      }
      sys.getNetwork()->synchronizeDirectory();
   }
   //std::cerr << "SYNC DIR DONE" << std::endl;
   //print();
   //std::cerr << "SYNC DIR & PRINT DONE" << std::endl;
}

DeviceOps *NewNewRegionDirectory::getOps( RegionDirectoryKey dict, reg_t id ) {
   NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, id );
   DeviceOps *ops = NULL;
   if ( regEntry != NULL ) {
      ops = regEntry->getOps();
   }
   return ops;
}

void NewNewRegionDirectory::initializeEntry( RegionDirectoryKey dict, reg_t reg ) {
   NewNewDirectoryEntryData *entry = NEW NewNewDirectoryEntryData();
   //entry->addAccess( 0, 1 );
   dict->setRegionData( reg, entry );
}

reg_t NewNewRegionDirectory::getLocalRegionId(void * hostObject, reg_t hostRegionId ) const {
   GlobalRegionDictionary *dict = getRegionDictionary( (uint64_t) hostObject );
   return dict->getLocalRegionIdFromMasterRegionId( hostRegionId );
}

//reg_t NewNewRegionDirectory::getLocalRegionIdFromMasterRegionId( RegionDirectoryKey dict, reg_t masterId ) {
//   //TODO
//   return dict->getLocalRegionIdFromMasterRegionId( masterId );
//}

void NewNewRegionDirectory::addMasterRegionId( RegionDirectoryKey dict, reg_t masterId, reg_t localId ) {
   //NewNewDirectoryEntryData *regEntry = getDirectoryEntry( *dict, localId );
   //std::cerr << "Setting master region id " << masterId << " for reg [" << (void *) dict << " " << localId << "] regEntry: " << regEntry << std::endl;
   dict->addMasterRegionId( masterId, localId );
}
