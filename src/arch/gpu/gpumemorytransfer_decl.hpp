/*************************************************************************************/
/*      Copyright 2009-2018 Barcelona Supercomputing Center                          */
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
/*      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            */
/*************************************************************************************/

#ifndef _NANOS_MEMORY_TRANSFER_DECL
#define _NANOS_MEMORY_TRANSFER_DECL

#include "atomic_decl.hpp"
#include "compatibility.hpp"
#include "copydescriptor_decl.hpp"

#include <list>

namespace nanos {
namespace ext {
   class GPUMemoryTransfer
   {
      public:
         CopyDescriptor _hostAddress; 
         void *         _deviceAddress;
         size_t         _len;
         size_t         _count;
         size_t         _ld;
         bool           _requested;

         GPUMemoryTransfer( CopyDescriptor &hostAddress, void * deviceAddress, size_t len, size_t count, size_t ld ) :
            _hostAddress( hostAddress ), _deviceAddress( deviceAddress ), _len( len ), _count( count ), _ld( ld ), _requested( false ) {}

         GPUMemoryTransfer( GPUMemoryTransfer &mt ) :
            _hostAddress( mt._hostAddress ), _deviceAddress( mt._deviceAddress ),
            _len( mt._len ), _count( mt._count ), _ld( mt._ld ),
            _requested( mt._requested ) {}

         GPUMemoryTransfer ( const GPUMemoryTransfer &mt ) :
                     _hostAddress( mt._hostAddress ), _deviceAddress( mt._deviceAddress ),
                     _len( mt._len ), _count( mt._count ), _ld( mt._ld ),
                     _requested( mt._requested ) {}

         void completeTransfer();

         ~GPUMemoryTransfer() {}
   };

   class GPUMemoryTransferList
   {
      public:

         GPUMemoryTransferList() {}
         virtual ~GPUMemoryTransferList() {}

         virtual void addMemoryTransfer ( CopyDescriptor &hostAddress, void * deviceAddress, size_t len, size_t count, size_t ld ) {}
         //virtual void addMemoryTransfer ( CopyDescriptor &hostAddress ) {}
         virtual void removeMemoryTransfer ( CopyDescriptor &hostAddress ) {}
         virtual void removeMemoryTransfer () {}
         virtual void checkAddressForMemoryTransfer ( void * address ) {}
         virtual void executeMemoryTransfers () {}
         virtual void requestTransfer( void * address ) {}
         virtual void clearRequestedMemoryTransfers () {}
   };

   class GPUMemoryTransferOutList : public GPUMemoryTransferList
   {
      protected:
         /*! Pending output transfers generated by the tasks executed by the corresponding thread
          *  A requested memory transfer means that someone else needs the data, so it should be
          *  copied out as soon as possible
          */
         std::list<GPUMemoryTransfer *> _pendingTransfersAsync;

         Lock                           _lock;

      public:
         GPUMemoryTransferOutList() : GPUMemoryTransferList(), _lock() {}

         virtual ~GPUMemoryTransferOutList();

         /*! \brief Add a new memory transfer to the list of pending transfers (synchronous or asynchronous)
          */
         virtual void addMemoryTransfer ( CopyDescriptor &hostAddress, void * deviceAddress, size_t len, size_t count, size_t ld );

         virtual void removeMemoryTransfer ( GPUMemoryTransfer * mt ) {}

         /*! \brief Execute one memory transfer from the list of pending transfers.
          *         Requested transfers have priority (synchronous or asynchronous)
          */
         virtual void removeMemoryTransfer ();

         /* \brief Check if the given address has pending transfer(s) and execute it/them (synchronous or asynchronous)
          */
         virtual void checkAddressForMemoryTransfer ( void * address );

         /*! \brief Set the associated pending transfer(s) for address to requested (synchronous or asynchronous)
          */
         virtual void requestTransfer( void * address );
   };

   class GPUMemoryTransferOutSyncList : public GPUMemoryTransferOutList
   {
      public:

         GPUMemoryTransferOutSyncList() : GPUMemoryTransferOutList() {}
         ~GPUMemoryTransferOutSyncList() {}

         /*! \brief Execute the given memory transfer (synchronous)
          */
         void removeMemoryTransfer ( GPUMemoryTransfer * mt );

         /*! \brief Execute all the requested memory transfers (synchronous)
          */
         void clearRequestedMemoryTransfers ();

         /*! \brief Execute all the memory transfers (synchronous)
          */
         void executeMemoryTransfers ();
   };

   class GPUMemoryTransferOutAsyncList : public GPUMemoryTransferOutList
   {
      private:
         /*! Execute all the memory transfers in a double buffering way (asynchronous)
          *  Each copy to the intermediate pinned buffer is overlapped with another copy
          *  from the device to host
          */
         void executeMemoryTransfers ( std::list<GPUMemoryTransfer *> &pendingTransfersAsync );

      public:
         GPUMemoryTransferOutAsyncList() : GPUMemoryTransferOutList() {}
         ~GPUMemoryTransferOutAsyncList() {}

         /*! \brief Add a new memory transfer to the list of pending transfers (asynchronous)
          */
         void addMemoryTransfer ( CopyDescriptor &hostAddress, void * deviceAddress, size_t len, size_t count, size_t ld );

         /*! \brief Remove the given memory transfer (asynchronous)
          */
         void removeMemoryTransfer ( GPUMemoryTransfer * mt );

         /*! \brief Remove the memory transfer related to the given address (asynchronous)
          */
         void removeMemoryTransfer ( CopyDescriptor &hostAddress );

         /*! \brief Execute all the requested memory transfers (asynchronous)
          */
         void executeMemoryTransfers ();

         /*! \brief Execute all the requested memory transfers (asynchronous)
          */
         void executeRequestedMemoryTransfers ();
   };

   class GPUMemoryTransferInAsyncList : public GPUMemoryTransferList
   {
      private:
         /*! Pending input transfers needed by the corresponding thread to execute its tasks
          *  The copies have been ordered, but not completed
          */
         std::list<CopyDescriptor>        _pendingTransfersAsync;

         /*! Pending input transfers that somebody else has requested
          */
         std::list<GPUMemoryTransfer *>   _requestedTransfers;

         Lock                             _lock;

      public:
         GPUMemoryTransferInAsyncList() : GPUMemoryTransferList(), _lock() {}
         ~GPUMemoryTransferInAsyncList();


         /*! \brief Add a new memory transfer to the list of requested transfers (asynchronous)
          */
         void addMemoryTransfer ( CopyDescriptor &hostAddress, void * deviceAddress, size_t len, size_t count, size_t ld );

         /*! \brief Execute the given memory transfer (asynchronous)
          */
         void removeMemoryTransfer ( GPUMemoryTransfer * mt );

         /*! \brief Execute all the requested memory transfers (asynchronous)
          */
         void executeMemoryTransfers ();
   };

} // namespace nanos
} // namespace ext

#endif // _NANOS_MEMORY_TRANSFER_DECL
