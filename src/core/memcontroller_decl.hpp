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

#ifndef MEMCONTROLLER_DECL
#define MEMCONTROLLER_DECL
#include <map>
#include "workdescriptor_fwd.hpp"

#include "addressspace_decl.hpp"
#include "atomic_decl.hpp"
#include "backupcachecopy_decl.hpp"
#include "backupprivatecopy_decl.hpp"
#include "lock_decl.hpp"
#include "newregiondirectory_decl.hpp"
#include "addressspace_decl.hpp"
#include "memoryops_decl.hpp"
#include "memcachecopy_decl.hpp"
#include "newregiondirectory_decl.hpp"
#include "regionset_decl.hpp"

#include <vector>

namespace nanos {

typedef enum {
   NANOS_FT_CP_IN = 1,
   NANOS_FT_CP_OUT,  // 2
   NANOS_FT_CP_INOUT,// 3
   NANOS_FT_RT_IN,   // 4
   NANOS_FT_RT_INOUT // 5
} checkpoint_event_value_t;

class MemController {
   bool                        _initialized;
   bool                        _preinitialized;
   bool                        _inputDataReady;
   bool                        _outputDataReady;
   bool                        _dataRestored;
   bool                        _memoryAllocated;
   bool                        _invalidating;
   bool                        _mainWd;
   WD                         *_wd;
   ProcessingElement          *_pe;
   Lock                        _provideLock;
   RegionSet _providedRegions;
   BaseAddressSpaceInOps      *_inOps;
   SeparateAddressSpaceOutOps *_outOps;
#ifdef NANOS_RESILIENCY_ENABLED   // compile time disable
   SeparateAddressSpaceInOps     *_backupOpsIn;
   SeparateAddressSpaceInOps     *_backupOpsOut;
   SeparateAddressSpaceOutOps    *_restoreOps;
   std::vector<BackupCacheCopy>   _backupCacheCopies;
   std::vector<BackupPrivateCopy> _backupInOutCopies;
#endif
   size_t    _affinityScore;
   size_t    _maxAffinityScore;
   RegionSet _ownedRegions;
   RegionSet _parentRegions;

public:
   std::vector<MemCacheCopy> _memCacheCopies;

   enum MemControllerPolicy {
      WRITE_BACK,
      WRITE_THROUGH,
      NO_CACHE
   };

   MemController( WD *wd, unsigned int numCopies );
   ~MemController();
   bool hasVersionInfoForRegion( global_reg_t reg, unsigned int &version, NewLocationInfoList &locations );
   void getInfoFromPredecessor( MemController const &predecessorController );
   void preInit();
   void initialize( ProcessingElement &pe );
   bool allocateTaskMemory();
   void copyDataIn();
   void copyDataOut( MemControllerPolicy policy );
#ifdef NANOS_RESILIENCY_ENABLED
   void restoreBackupData(); /* Restores a previously backed up input data */
   bool isDataRestored( WD const &wd );
#endif
   bool isDataReady( WD const &wd );
   bool isOutputDataReady( WD const &wd );
   memory::Address getAddress( unsigned int index ) const;
   bool canAllocateMemory( memory_space_id_t memId, bool considerInvalidations ) const;
   void setAffinityScore( size_t score );
   size_t getAffinityScore() const;
   void setMaxAffinityScore( size_t score );
   size_t getMaxAffinityScore() const;
   size_t getAmountOfTransferredData() const;
   size_t getTotalAmountOfData() const;
   bool isRooted( memory_space_id_t &loc ) const ;
   bool isMultipleRooted( std::list<memory_space_id_t> &locs ) const ;
   void setMainWD();
   void synchronize();
   void synchronize( size_t numDataAccesses, DataAccess *data);
   bool isMemoryAllocated() const;
   void setCacheMetaData();
   bool ownsRegion( global_reg_t const &reg );
   bool hasObjectOfRegion( global_reg_t const &reg );
   bool containsAllCopies( MemController const &target ) const;
   const WorkDescriptor* getWorkDescriptor() const { return _wd; }
};

} // namespace nanos
#endif /* MEMCONTROLLER_DECL */
