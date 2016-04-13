#ifndef MEMCONTROLLER_DECL
#define MEMCONTROLLER_DECL
#include <map>
#include "workdescriptor_fwd.hpp"
#include "atomic_decl.hpp"
#include "newregiondirectory_decl.hpp"
#include "addressspace_decl.hpp"
#include "memoryops_decl.hpp"
#include "memcachecopy_decl.hpp"
#include "regionset_decl.hpp"

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
   bool                        _mainWd;
   bool                        _is_private_backup_aborted;
   WD                         &_wd;
   ProcessingElement          *_pe;
   Lock                        _provideLock;
   //std::map< NewNewRegionDirectory::RegionDirectoryKey, std::map< reg_t, unsigned int > > _providedRegions;
   RegionSet _providedRegions;
   BaseAddressSpaceInOps      *_inOps;
   SeparateAddressSpaceOutOps *_outOps;
#ifdef NANOS_RESILIENCY_ENABLED   // compile time disable
   SeparateAddressSpaceInOps  *_backupOpsIn;
   SeparateAddressSpaceInOps  *_backupOpsOut;
   SeparateAddressSpaceOutOps *_restoreOps;
   MemCacheCopy               *_backupCacheCopies;
   Chunk                      *_backupInOutCopies;
#endif
   std::size_t                 _affinityScore;
   std::size_t                 _maxAffinityScore;
   RegionSet _ownedRegions;
   RegionSet _parentRegions;

public:
   enum MemControllerPolicy {
      WRITE_BACK,
      WRITE_THROUGH,
      NO_CACHE
   };
   MemCacheCopy *_memCacheCopies;
   MemController( WD &wd );
   //bool hasVersionInfoForRegion( global_reg_t reg, unsigned int &version, NewLocationInfoList &locations );
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
   uint64_t getAddress( unsigned int index ) const;
   bool canAllocateMemory( memory_space_id_t memId, bool considerInvalidations ) const;
   void setAffinityScore( std::size_t score );
   std::size_t getAffinityScore() const;
   void setMaxAffinityScore( std::size_t score );
   std::size_t getMaxAffinityScore() const;
   std::size_t getAmountOfTransferredData() const;
   std::size_t getTotalAmountOfData() const;
   bool isRooted( memory_space_id_t &loc ) const ;
   void setMainWD();
   void synchronize();
   bool isMemoryAllocated() const;
   void setCacheMetaData();
   bool ownsRegion( global_reg_t const &reg );
   bool hasObjectOfRegion( global_reg_t const &reg );
};

}
#endif /* MEMCONTROLLER_DECL */
