#ifndef SMPTRANSFERQUEUE_DECL
#define SMPTRANSFERQUEUE_DECL

#include <list>
#include "atomic_decl.hpp"
#include "deviceops_fwd.hpp"

namespace nanos {

class SMPTransfer {
   DeviceOps   *_ops;
   memory::Address _dst;
   memory::Address _src;
   size_t          _len;
   size_t          _count;
   size_t          _ld;
   bool            _in;
   public:
   SMPTransfer();
   SMPTransfer( DeviceOps *ops, memory::Address dst, memory::Address src, size_t len, size_t count, size_t ld, bool in );
   SMPTransfer( SMPTransfer const &s );
   SMPTransfer &operator=( SMPTransfer const &s );
   ~SMPTransfer();
   void execute();
};

class SMPTransferQueue {
   Lock _lock;
   std::list< SMPTransfer > _transfers;
   public:
   SMPTransferQueue();
   void addTransfer( DeviceOps *ops, memory::Address dst, memory::Address src, size_t len, size_t count, size_t ld, bool in );
   void tryExecuteOne();
};

} // namespace nanos

#endif
