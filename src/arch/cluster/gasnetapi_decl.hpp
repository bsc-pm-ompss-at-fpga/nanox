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


#ifndef _GASNETAPI_DECL
#define _GASNETAPI_DECL

#include "clusterplugin_fwd.hpp"
#include "basethread_decl.hpp"
#include "networkapi.hpp"
#include "network_decl.hpp"
#include "simpleallocator_decl.hpp"
#include "requestqueue_decl.hpp"
#include "remoteworkdescriptor_fwd.hpp"
#include <vector>

extern "C" {
#include <gasnet.h>
}

namespace nanos {
namespace ext {

   class GASNetAPI : public NetworkAPI
   {
      private:
         static GASNetAPI *_instance;
         static GASNetAPI *getInstance();

         ClusterPlugin &_plugin;
         Network *_net;
         RemoteWorkDescriptor *_rwgGPU;
         RemoteWorkDescriptor *_rwgSMP;
#ifndef GASNET_SEGMENT_EVERYTHING
         SimpleAllocator *_thisNodeSegment;
#endif
         SimpleAllocator *_packSegment;
         std::vector< SimpleAllocator * > _pinnedAllocators;
         std::vector< Lock * > _pinnedAllocatorsLocks;
         Atomic<unsigned int> *_seqN;

         class WorkBufferManager {
            std::map<unsigned int, char *> _buffers;
            Lock _lock;

            public:
            WorkBufferManager();
            char *_add(unsigned int wdId, unsigned int num, std::size_t totalLen, std::size_t thisLen, char *buff );
            char *get(unsigned int wdId, std::size_t totalLen, std::size_t thisLen, char *buff );
         };

         class GASNetSendDataRequest : public SendDataRequest {
            protected:
            GASNetAPI *_gasnetApi;
            public:
            GASNetSendDataRequest( GASNetAPI *api, unsigned int seqNumber, void *origAddr, void *destAddr, std::size_t len,
               std::size_t count, std::size_t ld, unsigned int dst, unsigned int wdId, void *hostObject, reg_t hostRegId,
               unsigned int metaSeq );
         };

         struct SendDataPutRequestPayload {
            unsigned int  _seqNumber;
            void         *_origAddr;
            void         *_destAddr;
            std::size_t   _len;
            std::size_t   _count;
            std::size_t   _ld;
            unsigned int  _destination;
            unsigned int  _wdId;

            void         *_tmpBuffer;
            WD const     *_wd;
            Functor      *_functor;
            void         *_hostObject;
            reg_t         _hostRegId;
            unsigned int  _metaSeq;

            SendDataPutRequestPayload ( unsigned int seqNumber, void *origAddr, void *dstAddr, std::size_t len, std::size_t count,
               std::size_t ld, unsigned int dest, unsigned int wdId, void *tmpBuffer, WD const *wd, Functor *func,
               void *hostObject, reg_t hostRegId, unsigned int _metaSeq );
            };

         struct SendDataGetRequestPayload {
            unsigned int  _seqNumber;
            void         *_origAddr;
            void         *_destAddr;
            std::size_t   _len;
            std::size_t   _count;
            std::size_t   _ld;
            void         *_waitObj;

            //void         *_hostObject;
            //reg_t         _hostRegId;
            CopyData      _cd;
            

            SendDataGetRequestPayload ( unsigned int seqNumber, void *origAddr, void *dstAddr, std::size_t len, std::size_t count,
               std::size_t ld, void *waitObj, CopyData const &cd );
            };

         class SendDataPutRequest : public GASNetSendDataRequest {
            void *_tmpBuffer;
            WD const *_wd;
            Functor *_functor;
            public:
            //SendDataPutRequest( GASNetAPI *api, unsigned int seqNumber, unsigned int dest, void *origAddr, void *destAddr, std::size_t len, std::size_t count, std::size_t ld, void *tmpBuffer, unsigned int wdId, WD const *wd, Functor *f );
            SendDataPutRequest( GASNetAPI *api, SendDataPutRequestPayload *msg );
            virtual ~SendDataPutRequest();
            virtual void doSingleChunk();
            virtual void doStrided( void *localAddr );
         };

         class SendDataGetRequest : public GASNetSendDataRequest {
            void *_waitObj;
            public:
            CopyData _cd;
            SendDataGetRequest( GASNetAPI *api, unsigned int seqNumber, void *origAddr, void *destAddr, std::size_t len,
            std::size_t count, std::size_t ld, void *waitObj, CopyData const &cd, nanos_region_dimension_internal_t *dims );
            virtual ~SendDataGetRequest();
            virtual void doSingleChunk();
            virtual void doStrided( void *localAddr );
         };

         RequestQueue< SendDataRequest > _dataSendRequests;
         struct FreeBufferRequest {
            FreeBufferRequest(void *addr, WD const *w, Functor *f );
            void *address;
            WD const * wd;
            Functor *functor;
         };
         RequestQueue< FreeBufferRequest > _freeBufferReqs;
         RequestQueue< std::pair< void *, unsigned int > > _workDoneReqs;

         std::size_t _rxBytes;
         std::size_t _txBytes;
         std::size_t _totalBytes;

         unsigned int _numSegments;
         void ** _segmentAddrList;
         std::size_t * _segmentLenList;

         WorkBufferManager _incomingWorkBuffers;

      public:
         GASNetAPI( ClusterPlugin &p );
         ~GASNetAPI();
         void initialize ( Network *net );
         void finalize ();
         void poll ();
         void sendExitMsg ( unsigned int dest );
         void sendWorkMsg ( unsigned int dest, void ( *work ) ( void * ), unsigned int arg0, unsigned int arg1, unsigned int numPe, std::size_t argSize, char * arg, void ( *xlate ) ( void *, void * ), int arch, void *wd, std::size_t expectedData );
         void sendWorkDoneMsg ( unsigned int dest, void *remoteWdAddr, int peId);
         void _sendWorkDoneMsg ( unsigned int dest, void *remoteWdAddr, int peId);
         void put ( unsigned int remoteNode, uint64_t remoteAddr, void *localAddr, std::size_t size, unsigned int wdId, WD const &wd, void *hostObject, reg_t hostRegId, unsigned int metaSeq );
         void putStrided1D ( unsigned int remoteNode, uint64_t remoteAddr, void *localAddr, void *localPack, std::size_t size, std::size_t count, std::size_t ld, unsigned int wdId, WD const &wd, void *hostObject, reg_t hostRegId, unsigned int metaSeq );
         void get ( void *localAddr, unsigned int remoteNode, uint64_t remoteAddr, std::size_t size, volatile int *requestComplete, CopyData const &cd );
         void getStrided1D ( void *packedAddr, unsigned int remoteNode, uint64_t remoteTag, uint64_t remoteAddr, std::size_t size, std::size_t count, std::size_t ld, volatile int *requestComplete, CopyData const &cd );
         void malloc ( unsigned int remoteNode, std::size_t size, void *waitObjAddr );
         void memFree ( unsigned int remoteNode, void *addr );
         void memRealloc ( unsigned int remoteNode, void *oldAddr, std::size_t oldSize, void *newAddr, std::size_t newSize );
         void nodeBarrier( void );
         
         void sendMyHostName( unsigned int dest );
         void sendRequestPut( unsigned int dest, uint64_t origAddr, unsigned int dataDest, uint64_t dstAddr, std::size_t len, unsigned int wdId, WD const &wd, Functor *f, void *hostObject, reg_t hostRegId, unsigned int metaSeq );
         void sendRequestPutStrided1D( unsigned int dest, uint64_t origAddr, unsigned int dataDest, uint64_t dstAddr, std::size_t len, std::size_t count, std::size_t ld, unsigned int wdId, WD const &wd, Functor *f, void *hostObject, reg_t hostRegId, unsigned int metaSeq );
         void sendRegionMetadata( unsigned int dest, CopyData *cd, unsigned int seq );

         std::size_t getMaxGetStridedLen() const;
         std::size_t getTotalBytes();
         std::size_t getRxBytes();
         std::size_t getTxBytes();
         SimpleAllocator *getPackSegment() const;
         void *allocateReceiveMemory( std::size_t len );
         void freeReceiveMemory( void * addr );
         void processSendDataRequest( SendDataRequest *req );
         void addSegments( unsigned int numSegments, void **segmentAddr, std::size_t *segmentSize );
         void * getSegmentAddr( unsigned int idx );
         std::size_t getSegmentLen( unsigned int idx );
         unsigned int getNumNodes() const;
         unsigned int getNodeNum() const;
         void synchronizeDirectory( unsigned int dest );

      private:
         void _put ( unsigned int remoteNode, uint64_t remoteAddr, void *localAddr, std::size_t size, void *remoteTmpBuffer, unsigned int wdId, WD const &wd, Functor *f, void *hostObject, reg_t hostRegId, unsigned int metaSeq );
         void _putStrided1D ( unsigned int remoteNode, uint64_t remoteAddr, void *localAddr, void *localPack, std::size_t size, std::size_t count, std::size_t ld, void *remoteTmpBuffer, unsigned int wdId, WD const &wd, Functor *f, void *hostObject, reg_t hostRegId, unsigned int metaSeq );
         void sendFreeTmpBuffer( void *addr, WD const *wd, Functor *f );
         void sendWaitForRequestPut( unsigned int dest, uint64_t addr, unsigned int wdId );
         static void print_copies( WD const *wd, int deps );
         void enqueueFreeBufferNotify( void *bufferAddr, WD const *wd, Functor *f );
         void checkForPutReqs();
         void checkForFreeBufferReqs();
         void checkWorkDoneReqs();
         unsigned int getPutRequestSequenceNumber( unsigned int dest );

         // Active Message handlers
         static void amFinalize( gasnet_token_t token );
         static void amFinalizeReply(gasnet_token_t token);
         static void amWork(gasnet_token_t token, void *arg, std::size_t argSize,
                             gasnet_handlerarg_t workLo,
                             gasnet_handlerarg_t workHi,
                             gasnet_handlerarg_t xlateLo,
                             gasnet_handlerarg_t xlateHi,
                             gasnet_handlerarg_t rmwdLo,
                             gasnet_handlerarg_t rmwdHi,
                             gasnet_handlerarg_t expectedDataLo,
                             gasnet_handlerarg_t expectedDataHi,
                             gasnet_handlerarg_t totalArgSizeLo,
                             gasnet_handlerarg_t totalArgSizeHi,
                             gasnet_handlerarg_t dataSize,
                             gasnet_handlerarg_t wdId,
                             gasnet_handlerarg_t arch,
                             gasnet_handlerarg_t seq );
         static void amWorkData(gasnet_token_t token, void *buff, std::size_t len,
               gasnet_handlerarg_t wdId,
               gasnet_handlerarg_t msgNum,
               gasnet_handlerarg_t totalLenLo,
               gasnet_handlerarg_t totalLenHi);
                  
         static void amWorkDone( gasnet_token_t token, gasnet_handlerarg_t addrLo, gasnet_handlerarg_t addrHi, gasnet_handlerarg_t peId );
         static void amMalloc( gasnet_token_t token, gasnet_handlerarg_t sizeLo, gasnet_handlerarg_t sizeHi,
                                gasnet_handlerarg_t waitObjAddrLo, gasnet_handlerarg_t waitObjAddrHi );
         static void amMallocReply( gasnet_token_t token, gasnet_handlerarg_t addrLo, gasnet_handlerarg_t addrHi,
                                      gasnet_handlerarg_t waitObjAddrLo, gasnet_handlerarg_t waitObjAddrHi );
         static void amFree( gasnet_token_t token, gasnet_handlerarg_t addrLo, gasnet_handlerarg_t addrHi );
         static void amRealloc( gasnet_token_t token, gasnet_handlerarg_t oldAddrLo, gasnet_handlerarg_t oldAddrHi,
                                    gasnet_handlerarg_t oldSizeLo, gasnet_handlerarg_t oldSizeHi,
                                    gasnet_handlerarg_t newAddrLo, gasnet_handlerarg_t newAddrHi,
                                    gasnet_handlerarg_t newSizeLo, gasnet_handlerarg_t newSizeHi);
         static void amMasterHostname( gasnet_token_t token, void *buff, std::size_t nbytes );
         static void amPut( gasnet_token_t token,
               void *buf,
               std::size_t len,
               gasnet_handlerarg_t origAddrLo,
               gasnet_handlerarg_t origAddrHi,
               gasnet_handlerarg_t tagAddrLo,
               gasnet_handlerarg_t tagAddrHi,
               gasnet_handlerarg_t totalLenLo,
               gasnet_handlerarg_t totalLenHi,
               gasnet_handlerarg_t wdId,
               gasnet_handlerarg_t wdLo,
               gasnet_handlerarg_t wdHi,
               gasnet_handlerarg_t seq,
               gasnet_handlerarg_t last,
               gasnet_handlerarg_t functorLo,
               gasnet_handlerarg_t functorHi,
               gasnet_handlerarg_t hostObjectLo,
               gasnet_handlerarg_t hostObjectHi,
               gasnet_handlerarg_t regId );
         static void amGet( gasnet_token_t token, void *buff, std::size_t nbytes );
         //static void amGet( gasnet_token_t token,
         //      gasnet_handlerarg_t destAddrLo,
         //      gasnet_handlerarg_t destAddrHi,
         //      gasnet_handlerarg_t origAddrLo,
         //      gasnet_handlerarg_t origAddrHi,
         //      gasnet_handlerarg_t tagAddrLo,
         //      gasnet_handlerarg_t tagAddrHi,
         //      gasnet_handlerarg_t lenLo,
         //      gasnet_handlerarg_t lenHi,
         //      gasnet_handlerarg_t totalLenLo,
         //      gasnet_handlerarg_t totalLenHi,
         //      gasnet_handlerarg_t waitObjLo,
         //      gasnet_handlerarg_t waitObjHi,
         //      gasnet_handlerarg_t seqNumber,
         //      gasnet_handlerarg_t hostObjectLo,
         //      gasnet_handlerarg_t hostObjectHi,
         //      gasnet_handlerarg_t regId );
         static void amGetReply( gasnet_token_t token,
               void *buf,
               std::size_t len,
               gasnet_handlerarg_t waitObjLo,
               gasnet_handlerarg_t waitObjHi);
         static void amPutF( gasnet_token_t token,
               gasnet_handlerarg_t destAddrLo,
               gasnet_handlerarg_t destAddrHi,
               gasnet_handlerarg_t len,
               gasnet_handlerarg_t wordSize,
               gasnet_handlerarg_t valueLo,
               gasnet_handlerarg_t valueHi );
         /*static void amRequestPut( gasnet_token_t token,
               gasnet_handlerarg_t destAddrLo,
               gasnet_handlerarg_t destAddrHi,
               gasnet_handlerarg_t origAddrLo,
               gasnet_handlerarg_t origAddrHi,
               gasnet_handlerarg_t tmpBufferLo,
               gasnet_handlerarg_t tmpBufferHi,
               gasnet_handlerarg_t lenLo,
               gasnet_handlerarg_t lenHi,
               gasnet_handlerarg_t wdId,
               gasnet_handlerarg_t wdLo,
               gasnet_handlerarg_t wdHi,
               gasnet_handlerarg_t dst,
               gasnet_handlerarg_t functorLo,
               gasnet_handlerarg_t functorHi,
               gasnet_handlerarg_t seqNumber );*/
         static void amRequestPut( gasnet_token_t token, void *buff, std::size_t nbytes );
         static void amRequestPutStrided1D( gasnet_token_t token, void *buff, std::size_t nbytes );
         static void amWaitRequestPut( gasnet_token_t token, 
               gasnet_handlerarg_t addrLo,
               gasnet_handlerarg_t addrHi,
               gasnet_handlerarg_t wdId,
               gasnet_handlerarg_t seqNumber );
         static void amFreeTmpBuffer( gasnet_token_t token, 
               gasnet_handlerarg_t addrLo,
               gasnet_handlerarg_t addrHi,
               gasnet_handlerarg_t wdLo,
               gasnet_handlerarg_t wdHi,
               gasnet_handlerarg_t functorLo,
               gasnet_handlerarg_t functorHi );
         static void amPutStrided1D( gasnet_token_t token,
               void *buf,
               std::size_t len,
               gasnet_handlerarg_t realTagLo,
               gasnet_handlerarg_t realTagHi,
               gasnet_handlerarg_t totalLenLo,
               gasnet_handlerarg_t totalLenHi,
               gasnet_handlerarg_t count,
               gasnet_handlerarg_t ld,
               gasnet_handlerarg_t wdId,
               gasnet_handlerarg_t wdLo,
               gasnet_handlerarg_t wdHi,
               gasnet_handlerarg_t seq,
               gasnet_handlerarg_t lastMsg,
               gasnet_handlerarg_t functorLo,
               gasnet_handlerarg_t functorHi,
               gasnet_handlerarg_t hostObjectLo,
               gasnet_handlerarg_t hostObjectHi,
               gasnet_handlerarg_t regId );
         static void amGetStrided1D( gasnet_token_t token, void *buff, std::size_t nbytes );
         //static void amGetStrided1D( gasnet_token_t token,
         //      gasnet_handlerarg_t destAddrLo,
         //      gasnet_handlerarg_t destAddrHi,
         //      gasnet_handlerarg_t origAddrLo,
         //      gasnet_handlerarg_t origAddrHi,
         //      gasnet_handlerarg_t origTagLo,
         //      gasnet_handlerarg_t origTagHi,
         //      gasnet_handlerarg_t lenLo,
         //      gasnet_handlerarg_t lenHi,
         //      gasnet_handlerarg_t count,
         //      gasnet_handlerarg_t ld,
         //      gasnet_handlerarg_t waitObjLo,
         //      gasnet_handlerarg_t waitObjHi,
         //      gasnet_handlerarg_t seqNumber,
         //      gasnet_handlerarg_t hostObjectLo,
         //      gasnet_handlerarg_t hostObjectHi,
         //      gasnet_handlerarg_t regId );
         static void amGetReplyStrided1D( gasnet_token_t token,
               void *buf,
               std::size_t len,
               gasnet_handlerarg_t waitObjLo,
               gasnet_handlerarg_t waitObjHi);
         static void amRegionMetadata(gasnet_token_t token,
               void *arg, std::size_t argSize, gasnet_handlerarg_t seq );
         static void amSynchronizeDirectory(gasnet_token_t token);
   };
}
}
#endif /* _GASNETAPI_DECL */
