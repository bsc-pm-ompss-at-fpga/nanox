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

#include "mpi.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "mpidd.hpp"
#include "mpiprocessor_decl.hpp"
#include "workdescriptor_decl.hpp"
#include "processingelement_fwd.hpp"
#include "copydescriptor_decl.hpp"
#include "mpidevice.hpp"
#include <unistd.h>
#include "deviceops.hpp"
#include "request.hpp"

#include "cachecommand.hpp"
#include "allocate.hpp"

//TODO: check for errors in communications
//TODO: Depending on multithread level, wait for answers or not
using namespace nanos;
using namespace nanos::ext;

MPI_Datatype MPIDevice::cacheStruct;
char MPIDevice::_executingTask=0;
bool MPIDevice::_createdExtraWorkerThread=false;

MPIDevice::MPIDevice(const char *n) : Device(n) {
}

/*! \brief MPIDevice copy constructor
 */
MPIDevice::MPIDevice(const MPIDevice &arch) : Device(arch) {
}

/*! \brief MPIDevice destructor
 */
MPIDevice::~MPIDevice() {
};

/* \breif allocate size bytes in the device
 */
void * MPIDevice::memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem, WorkDescriptor const &wd, unsigned int copyIdx ) {
    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_ALLOC_EVENT);
    //std::cerr << "Inicio allocate\n";

    mpi::command::Allocate::Requestor order( static_cast<MPIProcessor const&>(mem.getConstPE()), size );
    order.dispatch();

    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;
    //std::cerr << "Fin allocate\n";

    return order.getData().getDeviceAddress(); // May be null
}

/* \brief free address
 */
//void memFree( uint64_t addr, SeparateMemoryAddressSpace &mem ) const;
void MPIDevice::memFree( uint64_t addr, SeparateMemoryAddressSpace &mem ) {
    if (addr == 0) return;

    //std::cerr << "Inicio free\n";
    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_FREE_EVENT);

    mpi::command::Free order( static_cast<MPIProcessor const&>(mem.getConstPE()), addr );
    order.dispatch();

    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;
    //std::cerr << "Fin free\n";
}

size_t MPIDevice::getMemCapacity( SeparateMemoryAddressSpace &mem ) {
    //MAXSIZE-1
    return (size_t)-1;
}

/* \brief Reallocate and copy from address.
 */
void * MPIDevice::realloc(void *address, size_t size, size_t old_size, ProcessingElement *pe) {
    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_REALLOC_EVENT);
    //std::cerr << "Inicio REALLOC\n";

    mpi::command::Realloc::Requestor order( static_cast<MPIProcessor const&>(*pe), address, size );
    order.dispatch();

    //std::cerr << "Fin realloc\n";
    //printf("Copiado de %p\n",(void *) order.devAddr);
    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;

    return order.getData().getDeviceAddress();
}

/* \brief Copy from remoteSrc in the host to localDst in the device
 *        Returns true if the operation is synchronous
 */
void MPIDevice::_copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_COPYIN_SYNC_EVENT);
    //std::cerr << "Inicio copyin\n";

    mpi::command::CopyIn order( static_cast<MPIProcessor&>(mem.getPE()), hostAddr, devAddr, len );
    order.dispatch();

    //std::cerr << "Fin copyin\n";
    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;
}

/* \brief Copy from localSrc in the device to remoteDst in the host
 *        Returns true if the operation is synchronous
 */
void MPIDevice::_copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_COPYOUT_SYNC_EVENT);

    MPIProcessor &destination = static_cast<MPIProcessor &>( mem.getPE() );
    //if PE is executing something, this means an extra cache-thread could be usefull, send creation signal
    if ( destination.getCurrExecutingWd() != NULL && !destination.getHasWorkerThread()) {        
    	  mpi::command::CreateAuxiliaryThread createThread( destination );
		  createThread.dispatch();

        destination.setHasWorkerThread(true);
    }

    mpi::command::CopyOut copyOrder( static_cast<MPIProcessor const&>(mem.getConstPE()), hostAddr, devAddr, len );
    copyOrder.dispatch();

    //std::cerr << "Fin copyout\n";
    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;
}

///* \brief Copy localy in the device from src to dst
// */
//void MPIDevice::copyLocal(void *dst, void *src, size_t size, ProcessingElement *pe) {
//    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_COPYLOCAL_SYNC_EVENT);
//    //std::cerr << "Inicio copylocal\n";
//    //HostAddr will be src and DevAddr will be dst (both are device addresses)
//    MPIProcessor * myPE = (MPIProcessor *) pe;
//    cacheOrder order;
//    order.opId = OPID_COPYLOCAL;
//    order.devAddr = (uint64_t) dst;
//    order.hostAddr = (uint64_t) src;
//    order.size = size;
//    MPIRemoteNode::nanosMPISend(&order, 1, cacheStruct, myPE->getRank(), TAG_CACHE_ORDER, myPE->getCommunicator());
//    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;
//    //std::cerr << "Fin copyin\n";
//}


bool MPIDevice::_copyDevToDev( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
    NANOS_MPI_CREATE_IN_MPI_RUNTIME_EVENT(ext::NANOS_MPI_COPYDEV2DEV_SYNC_EVENT);
    //This will never be another PE type which is not MPI (or something is broken in core :) )
    MPIProcessor &source = static_cast<MPIProcessor &>( memOrig.getPE() );
    MPIProcessor &destination = static_cast<MPIProcessor &>( memDest.getPE() );
    int result;
    MPI_Comm_compare(source.getCommunicator(), destination.getCommunicator(), &result);
    //If both devices are in the same comunicator, they can do a dev2dev communication, otherwise go through host
    if (result == MPI_IDENT){
        ops->addOp();

        //if PE is executing something, this means an extra cache-thread could be usefull, send creation signal
        if ( destination.getCurrExecutingWd() != NULL && !destination.getHasWorkerThread()) {        
        	   mpi::command::CreateAuxiliaryThread createThread( destination );
	         createThread.dispatch();

            destination.setHasWorkerThread(true);
        }

        //Send the destination rank together with DEV2DEV OPID (we'll substract it on remote node)
        mpi::command::CopyDeviceToDevice order( source, destination, devOrigAddr, devDestAddr, len );
        order.dispatch();

        ops->completeOp(); 
        if ( f ) {
           (*f)(); 
        }
        return true;
    }
    return false;
    NANOS_MPI_CLOSE_IN_MPI_RUNTIME_EVENT;
}

void MPIDevice::initMPICacheStruct() {
    //Initialize cacheStruct in case it's not initialized
    if (cacheStruct == 0) {
        MPI_Datatype typelist[4] = {MPI_INT, MPI_UNSIGNED_LONG_LONG, MPI_UNSIGNED_LONG_LONG, MPI_UNSIGNED_LONG_LONG};
        int blocklen[4] = {1, 1, 1, 1};
        MPI_Aint disp[4] = {offsetof(cacheOrder, opId), offsetof(cacheOrder, hostAddr), offsetof(cacheOrder, devAddr), offsetof(cacheOrder, size)};
        MPI_Type_create_struct(4, blocklen, disp, typelist, &cacheStruct);
        MPI_Type_commit(&cacheStruct);
    }
}

static void createExtraCacheThread(){    
    //Create extra worker thread
    MPI_Comm mworld= MPI_COMM_WORLD;
    ext::SMPProcessor *core = sys.getSMPPlugin()->getLastFreeSMPProcessorAndReserve();
    if (core==NULL) {
        core = sys.getSMPPlugin()->getSMPProcessorByNUMAnode(0,MPIRemoteNode::getCurrentProcessor());
    }
    MPIProcessor *mpi = NEW MPIProcessor(&mworld, CACHETHREADRANK,-1, false, false, /* Dummy*/ MPI_COMM_SELF, core, /* Dummmy memspace */ 0);
    MPIDD * dd = NEW MPIDD((MPIDD::work_fct) MPIDevice::remoteNodeCacheWorker);
    WD* wd = NEW WD(dd);
    NANOS_INSTRUMENT( sys.getInstrumentation()->incrementMaxThreads(); )
    mpi->startMPIThread(wd);
}

void MPIDevice::remoteNodeCacheWorker() {                            
    //myThread = myThread->getNextThread();
    MPI_Comm parentcomm; /* intercommunicator */
    MPI_Comm_get_parent(&parentcomm);    

    //If this process was not spawned, we don't need this daemon-thread
    if (parentcomm != 0 && parentcomm != MPI_COMM_NULL) {

        mpi::Dispatcher dispatcher(parentcomm, 10 ); // reserve space for 10 elements of each generic command type
        while( ! MPIDevice::finished() ) {
            dispatcher.waitForCommands();
            dispatcher.queueAvailableCommands();
				dispatcher.executeCommands();
        }
    }
}
