/*************************************************************************************/
/*      Copyright 2010 Barcelona Supercomputing Center                               */
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

#include "libxdma.h"

#include "fpgaprocessor.hpp"
#include "fpgaprocessorinfo.hpp"
#include "fpgamemorytransfer.hpp"
#include "fpgadevice.hpp"
#include "fpgaconfig.hpp"
#include "deviceops.hpp"
#include "fpgapinnedallocator.hpp"
#include "instrumentation_decl.hpp"

using namespace nanos;
using namespace nanos::ext;

#define DIRTY_SYNC

FPGADevice::FPGADevice ( const char *n ): Device( n ) {}

void FPGADevice::_copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, WD const *wd, void *hostObject, reg_t hostRegionId ) {

   CopyDescriptor cd( hostAddr );
   cd._ops = ops;
   ops->addOp();
   ProcessingElement &pe = mem.getPE();
   bool done = copyIn((void*)devAddr, cd, len, &pe, wd);
   if ( done ) ops->completeOp();
}


#ifdef NANOS_INSTRUMENTATION_ENABLED
static void dmaSubmitStart( FPGAProcessor *fpga, const WD *wd ) {
   Instrumentation *instr = sys.getInstrumentation();
   DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
   unsigned long long timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      warning("Could not read accelerator clock (dma submit start)");
   }

   instr->addDeviceEvent(
           Instrumentation::DeviceEvent( timestamp, TaskBegin, submitInstr, wd ) );
   instr->addDeviceEvent(
           Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, NULL, wd) );
}

static void dmaSubmitEnd( FPGAProcessor *fpga, const WD *wd ) {
   Instrumentation *instr = sys.getInstrumentation();
   //FIXME: Emit the accelerator ID
   DeviceInstrumentation *submitInstr = fpga->getSubmitInstrumentation();
   unsigned long long timestamp;
   xdma_status status;
   status = xdmaGetDeviceTime( &timestamp );
   if ( status != XDMA_SUCCESS ) {
      warning("Could not read accelerator clock (dma submit end)");
   }

   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskSwitch, submitInstr, wd, NULL) );
   instr->addDeviceEvent(
         Instrumentation::DeviceEvent( timestamp, TaskEnd, submitInstr, wd) );
}
#endif  //NANOS_INSTRUMENTATION_ENABLED

/*
 * Allow transfers to be performed synchronously 
 */
inline bool FPGADevice::copyIn( void *localDst, CopyDescriptor &remoteSrc, size_t size, ProcessingElement *pe, const WD* wd )
{
   verbose("fpga copy in");

   nanos::ext::FPGAProcessor *fpga = ( nanos::ext::FPGAProcessor* ) pe;

   uint64_t  src_addr = remoteSrc.getTag();
   xdma_status status;
   xdma_channel iChan;
   xdma_transfer_handle dmaHandle;
   xdma_device device;

   iChan =  fpga->getFPGAProcessorInfo()->getInputChannel();
   device = fpga->getFPGAProcessorInfo()->getDeviceHandle();

   //syncTransfer(src_addr, fpga);

   debug("submitting input transfer:" << std::endl
           << "  @:" << std::hex << src_addr << std::dec << " size:" << size
           << "  iChan:" << iChan );

   FPGAPinnedAllocator& allocator = FPGAProcessor::getPinnedAllocator();
   uint64_t baseAddress = (uint64_t)allocator.getBasePointer( (void *)src_addr, size);
   xdma_buf_handle bufHandle;

   if ( baseAddress ) {
      //Transferring pinned kernel space buffer
      uint64_t offset;
      offset = src_addr - baseAddress;
      bufHandle = allocator.getBufferHandle( (void *)baseAddress );

      NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_SUBMIT_IN_DMA_EVENT );
      NANOS_INSTRUMENT( dmaSubmitStart( fpga, wd ) );
      //Support synchronous transfers??
      status = xdmaSubmitKBuffer( bufHandle, size, (unsigned int)offset, XDMA_ASYNC,
            device, iChan, &dmaHandle );
      NANOS_INSTRUMENT( dmaSubmitEnd ( fpga, wd ) );
      NANOS_FPGA_CLOSE_RUNTIME_EVENT;
   } else {
      //Transfer user space memory (pin buffer & submit transfer)
      status = xdmaSubmitBuffer( (void*)src_addr, size, XDMA_ASYNC, device, iChan, &dmaHandle );
   }

   debug ( "  got intput handle: " << dmaHandle );
   if ( status != XDMA_SUCCESS )
      warning("Error submitting output: " << status);

   if ( FPGAConfig::getSyncTransfersEnabled() ) {
      {  NANOS_INSTRUMENT( InstrumentBurst i( "in-xdma" ,ext::NANOS_FPGA_WAIT_INPUT_DMA_EVENT); )
         xdmaWaitTransfer( dmaHandle );
         xdmaReleaseTransfer( &dmaHandle );
      }
   } else {
      fpga->getInTransferList()->addTransfer( remoteSrc, dmaHandle );
   }

   /*
    * This is not actually true because the data is copied asynchronously.
    * As long as the transfer is submitted and 
    */
   return true; // true means sync transfer
}
void FPGADevice::_copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len,
      SeparateMemoryAddressSpace &mem, DeviceOps *ops,
      WorkDescriptor const *wd, void *hostObject, reg_t hostRegionId ) {

   CopyDescriptor cd( hostAddr );
   cd._ops = ops;
   ops->addOp();
   ProcessingElement &pe = mem.getPE();
   bool done = copyOut(cd, (void*)devAddr, len, &pe, wd);
   if ( done ) ops->completeOp();
}

bool FPGADevice::copyOut( CopyDescriptor &remoteDst, void *localSrc, size_t size, ProcessingElement *pe, const WD *wd)
{
   verbose("fpga copy out");

   nanos::ext::FPGAProcessor *fpga = ( nanos::ext::FPGAProcessor* ) pe;
   
   //get channel
   xdma_channel oChan;
   xdma_transfer_handle dmaHandle;
   xdma_device device;
   xdma_status status;
   uint64_t src_addr = remoteDst.getTag();
   device = fpga->getFPGAProcessorInfo()->getDeviceHandle();
   oChan = fpga->getFPGAProcessorInfo()->getOutputChannel();

   debug("submitting output transfer:" << std::endl
           << "  @:" << std::hex <<  src_addr << std::dec << " size:" << size
           << "  oChan:" << oChan );

   //get pinned buffer handle for this address
   //at this point, assume that all buffers to be transferred to fpga are pinned

   FPGAPinnedAllocator& allocator = FPGAProcessor::getPinnedAllocator();
   uint64_t baseAddress = (uint64_t)allocator.getBasePointer( (void*)src_addr, size );
   if ( baseAddress ) {
      uint64_t offset;
      xdma_buf_handle bufHandle;
      offset = src_addr - baseAddress;
      bufHandle = allocator.getBufferHandle( (void *)baseAddress );

      NANOS_FPGA_CREATE_RUNTIME_EVENT( ext::NANOS_FPGA_SUBMIT_OUT_DMA_EVENT );
      NANOS_INSTRUMENT( dmaSubmitStart( fpga, wd ) );
      status = xdmaSubmitKBuffer( bufHandle, size, (unsigned int)offset, XDMA_ASYNC,
            device, oChan, &dmaHandle );
      NANOS_INSTRUMENT( dmaSubmitEnd( fpga, wd ) );
      NANOS_FPGA_CLOSE_RUNTIME_EVENT;

   } else {
      status = xdmaSubmitBuffer( (void*)src_addr, size, XDMA_ASYNC, device, oChan, &dmaHandle );
   }

   if ( status != XDMA_SUCCESS )
      warning( "Error submitting output:" << status );

   debug( "  got out handle: " << dmaHandle );
   verbose("add transfer H:" << dmaHandle << " to the transfer list");

   bool finished;

   if ( FPGAConfig::getSyncTransfersEnabled() ) {
      {  NANOS_INSTRUMENT( InstrumentBurst i( "in-xdma" ,ext::NANOS_FPGA_WAIT_OUTPUT_DMA_EVENT); )
         xdmaWaitTransfer( dmaHandle );
         xdmaReleaseTransfer( &dmaHandle );
      }
      finished = true;
   } else {
      ((FPGAMemoryOutTransferList*)fpga->getOutTransferList())->addTransfer( remoteDst, dmaHandle );
      finished = false;
   }

   return finished;
}

void *FPGADevice::memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem,
        WorkDescriptor const *wd, unsigned int copyIdx){
   //empty as we cannot allocate memory inside the fpga
   return (void *) 0xdeadbeef;
}
void FPGADevice::memFree( uint64_t addr, SeparateMemoryAddressSpace &mem ){
   //empty as we cannot allocate memory inside the fpga
}

//this is used to priorize transfers (because someone needs the data)
//In our case this causes this actually means "finish the transfer"
void FPGADevice::syncTransfer( uint64_t hostAddress, ProcessingElement *pe)
{
    //TODO: At this point we only are going to sync output transfers
    // as input transfers do not need to be synchronized
    ((FPGAProcessor *)pe)->getOutTransferList()->syncTransfer(hostAddress);
    //((FPGAProcessor *)pe)->getInTransferList()->syncTransfer(hostAddress);
}
