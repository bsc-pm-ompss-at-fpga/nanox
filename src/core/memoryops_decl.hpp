#ifndef MEMORYOPS_DECL
#define MEMORYOPS_DECL

#include "addressspace_decl.hpp"
#include "memcachecopy_fwd.hpp"
namespace nanos {
class BaseOps {
   public:
   struct OwnOp {
      DeviceOps         *_ops;
      global_reg_t       _reg;
      unsigned int       _version;
      memory_space_id_t  _location;
      OwnOp( DeviceOps *ops, global_reg_t reg, unsigned int version, memory_space_id_t location );
      OwnOp( OwnOp const &op );
      OwnOp &operator=( OwnOp const &op );
      bool operator<( OwnOp const &op ) const {
         return ( ( uintptr_t ) _ops ) < ( ( uintptr_t ) op._ops );
      }
      void commitMetadata() const;
   };
   private:
   bool _delayedCommit;
   std::set< OwnOp > _ownDeviceOps;
   std::set< DeviceOps * > _otherDeviceOps;
   std::size_t _amountOfTransferredData;

   public:
   BaseOps( bool delayedCommit );
   ~BaseOps();
   std::set< DeviceOps * > &getOtherOps();
   std::set< OwnOp > &getOwnOps();
   void insertOwnOp( DeviceOps *ops, global_reg_t reg, unsigned int version, memory_space_id_t location );
   bool isDataReady( WD const &wd );
   std::size_t getAmountOfTransferredData() const;
   void addAmountTransferredData(std::size_t amount);
};

class BaseAddressSpaceInOps : public BaseOps {
   protected:
   typedef std::map< SeparateMemoryAddressSpace *, TransferList > MapType;
   MapType _separateTransfers;
   std::set< AllocatedChunk * > _lockedChunks;

   public:
   BaseAddressSpaceInOps( bool delayedCommit );
   virtual ~BaseAddressSpaceInOps();

   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   void lockSourceChunks( global_reg_t const &reg, unsigned int version, NewLocationInfoList const &locations, memory_space_id_t thisLocation, WD const &wd, unsigned int copyIdx );
   void releaseLockedSourceChunks();

   virtual void addOpFromHost( global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   virtual void issue( WD const &wd );

   virtual bool prepareRegions( MemCacheCopy *memCopies, unsigned int numCopies, WD const &wd );
   virtual unsigned int getVersionNoLock( global_reg_t const &reg, WD const &wd, unsigned int copyIdx );

   virtual void copyInputData( MemCacheCopy const &memCopy, bool output, WD const &wd, unsigned int copyIdx );
   virtual void allocateOutputMemory( global_reg_t const &reg, unsigned int version, WD const &wd, unsigned int copyIdx );
};

typedef BaseAddressSpaceInOps HostAddressSpaceInOps;

class SeparateAddressSpaceInOps : public BaseAddressSpaceInOps {
   protected:
   SeparateMemoryAddressSpace &_destination;
   TransferList _hostTransfers;

   public:
   SeparateAddressSpaceInOps( bool delayedCommit, SeparateMemoryAddressSpace &destination );
   ~SeparateAddressSpaceInOps();

   virtual void addOpFromHost( global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   virtual void issue( WD const &wd );

   virtual bool prepareRegions( MemCacheCopy *memCopies, unsigned int numCopies, WD const &wd );
   virtual unsigned int getVersionNoLock( global_reg_t const &reg, WD const &wd, unsigned int copyIdx );

   virtual void copyInputData( MemCacheCopy const &memCopy, bool output, WD const &wd, unsigned int copyIdx );
   virtual void allocateOutputMemory( global_reg_t const &reg, unsigned int version, WD const &wd, unsigned int copyIdx );
};

class SeparateAddressSpaceOutOps : public BaseOps {
   typedef std::map< SeparateMemoryAddressSpace *, TransferList > MapType;
   bool _invalidation;
   MapType _transfers;

   public:
   SeparateAddressSpaceOutOps( bool delayedCommit, bool isInval );
   ~SeparateAddressSpaceOutOps();

   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, DeviceOps *ops, AllocatedChunk *chunk, unsigned int copyIdx );
   void issue( WD const &wd );
};

}

#endif /* MEMORYOPS_DECL */
