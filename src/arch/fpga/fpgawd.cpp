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

#include "fpgawd_decl.hpp"
#include "libxtasks_wrapper.hpp"
#include "fpgadd.hpp"
#include "fpgapinnedallocator.hpp"
#include "workdescriptor.hpp"
#include "simpleallocator.hpp"

using namespace nanos;
using namespace nanos::ext;

FPGAWD::FPGAWD ( int ndevices, DeviceData **devs, size_t data_size, size_t data_align, void *wdata,
   size_t numCopies, CopyData *copies, nanos_translate_args_t translate_args, const char *description )
   : WorkDescriptor( ndevices, devs, data_size, data_align, wdata, numCopies, copies, translate_args, description ),
   _origFpgaCopiesAddrs( numCopies, 0 )
{}

void FPGAWD::notifyParent() {
   NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-notify-task", getParent()->getId() ) );
   //FIXME: The current WD may not have a parent task

   //Copy the data back to the FPGA memory before doing the notification
   //NOTE: First the WD cache must be updated to get the data back into the host addressspace
   CopyData const * copies = getCopies();
   const size_t numCopies = getNumCopies();
   DataAccess * regions = ( DataAccess * )( calloc( numCopies, sizeof( DataAccess ) ) );
   for ( size_t cIdx = 0; cIdx < numCopies; ++cIdx ) {
      //NOTE: Only setting the used fields
      new (regions + cIdx) DataAccess( copies[cIdx].address, copies[cIdx].flags.input, copies[cIdx].flags.output,
         0 /*canRename*/, 0 /*concurrent*/, 0 /*commutative*/, 0 /*dimCount*/, NULL, 0 );
   }
   sys.getHostMemory().synchronize( *getParent(), numCopies, regions, true /*forceUnregister*/ );
   for ( size_t cIdx = 0; cIdx < numCopies; ++cIdx ) {
      void * hostAddr = copies[cIdx].address;
      if ( copies[cIdx].flags.output ) {
         const uint64_t devAddr = _origFpgaCopiesAddrs[cIdx];
         fpgaCopyDataToFPGA( fpgaAllocator->getBufferHandle(),
            devAddr - fpgaAllocator->getBaseAddress(), copies[cIdx].dimensions[0].size,
            hostAddr );
      }
      free(hostAddr);
   }

   //NOTE: Before sending the notification to the FPGA, free the host runtime resources.
   //      Otherwise, another thread may pick the parent task finalization message before we
   //      update the host runtime status. This will lead to a finalization of a WD with components.
   ext::FPGADD &parentDD = ( ext::FPGADD & )( getParent()->getActiveDevice() );
   xtasks_task_handle parentTask = ( xtasks_task_handle )( parentDD.getHandle() );
   WorkDescriptor::notifyParent();

   //NOTE: FPGA WD are internally handled, do not notify about its finalization
   if ( dynamic_cast<const ext::FPGADD *>( &getActiveDevice() ) == NULL ) {
      xtasks_stat status = xtasksNotifyFinishedTask( parentTask, 1 /*num finished tasks*/ );
      if ( status != XTASKS_SUCCESS ) {
         ensure( status == XTASKS_SUCCESS, " Error notifing FPGA about remote finished task" );
      }
   }
}
