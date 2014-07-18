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
      void commitMetadata( ProcessingElement *pe ) const;
   };
   private:
   bool                     _delayedCommit;
   bool                     _dataReady;
   ProcessingElement       *_pe;
   std::set< OwnOp >        _ownDeviceOps;
   std::set< DeviceOps * >  _otherDeviceOps;
   std::size_t              _amountOfTransferredData;

   BaseOps( BaseOps const &op );
   BaseOps &operator=( BaseOps const &op );
   protected:
   std::set< AllocatedChunk * > _lockedChunks;

   public:
   BaseOps( ProcessingElement *pe, bool delayedCommit );
   ~BaseOps();
   ProcessingElement *getPE() const;
   std::set< DeviceOps * > &getOtherOps();
   std::set< OwnOp > &getOwnOps();
   void insertOwnOp( DeviceOps *ops, global_reg_t reg, unsigned int version, memory_space_id_t location );
   bool isDataReady( WD const &wd, bool inval = false );
   std::size_t getAmountOfTransferredData() const;
   void addAmountTransferredData(std::size_t amount);
   void releaseLockedSourceChunks( WD const &wd );
};

class BaseAddressSpaceInOps : public BaseOps {
   protected:
   typedef std::map< SeparateMemoryAddressSpace *, TransferList > MapType;
   MapType _separateTransfers;

   public:
   BaseAddressSpaceInOps( ProcessingElement *pe, bool delayedCommit );
   virtual ~BaseAddressSpaceInOps();

   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   void lockSourceChunks( global_reg_t const &reg, unsigned int version, NewLocationInfoList const &locations, memory_space_id_t thisLocation, WD const &wd, unsigned int copyIdx );

   virtual void addOpFromHost( global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   virtual void issue( WD const &wd );

   virtual unsigned int getVersionNoLock( global_reg_t const &reg, WD const &wd, unsigned int copyIdx );

   virtual void copyInputData( MemCacheCopy const &memCopy, WD const &wd, unsigned int copyIdx );
   virtual void allocateOutputMemory( global_reg_t const &reg, unsigned int version, WD const &wd, unsigned int copyIdx );
};

typedef BaseAddressSpaceInOps HostAddressSpaceInOps;

class SeparateAddressSpaceInOps : public BaseAddressSpaceInOps {
   protected:
   SeparateMemoryAddressSpace &_destination;
   TransferList _hostTransfers;

   public:
   SeparateAddressSpaceInOps( ProcessingElement *pe, bool delayedCommit, SeparateMemoryAddressSpace &destination );
   ~SeparateAddressSpaceInOps();

   virtual void addOpFromHost( global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   virtual void issue( WD const &wd );

   virtual unsigned int getVersionNoLock( global_reg_t const &reg, WD const &wd, unsigned int copyIdx );

   virtual void copyInputData( MemCacheCopy const &memCopy, WD const &wd, unsigned int copyIdx );
   virtual void allocateOutputMemory( global_reg_t const &reg, unsigned int version, WD const &wd, unsigned int copyIdx );
};

class SeparateAddressSpaceOutOps : public BaseOps {
   typedef std::map< SeparateMemoryAddressSpace *, TransferList > MapType;
   bool _invalidation;
   MapType _transfers;

   public:
   SeparateAddressSpaceOutOps( ProcessingElement *pe, bool delayedCommit, bool isInval );
   ~SeparateAddressSpaceOutOps();

   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, DeviceOps *ops, AllocatedChunk *chunk, WD const &wd, unsigned int copyIdx );
   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, DeviceOps *ops, WD const &wd, unsigned int copyIdx );
   void issue( WD const &wd );
   void copyOutputData( SeparateMemoryAddressSpace *from, MemCacheCopy const &memCopy, bool output, WD const &wd, unsigned int copyIdx );
};

}

#endif /* MEMORYOPS_DECL */
