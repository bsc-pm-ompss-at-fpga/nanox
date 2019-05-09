/*************************************************************************************/
/*      Copyright 2019 Barcelona Supercomputing Center                               */
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
   //FIXME: The copy data may not be in copies[].address as it may be moved into another address space.
   //       In addition, the directory entry for copies[].address must be removed as it probably will
   //       be used again in a future task.

   NANOS_INSTRUMENT( InstrumentBurst instBurst( "fpga-notify-task", getParent()->getId() ) );

   //Copy the data back to the FPGA memory before doing the notification
   CopyData const * copies = getCopies();
   for ( size_t cIdx = 0; cIdx < getNumCopies(); ++cIdx ) {
      void * hostAddr = copies[cIdx].address;
      if ( copies[cIdx].flags.output ) {
         const uint64_t devAddr = _origFpgaCopiesAddrs[cIdx];
         fpgaCopyDataToFPGA( fpgaAllocator->getBufferHandle(),
            devAddr - fpgaAllocator->getBaseAddress(), copies[cIdx].dimensions[0].size,
            hostAddr );
      }
      free(hostAddr);
   }

   //NOTE: FPGA WD are internally handled, do not notify about its finalization
   if ( dynamic_cast<const ext::FPGADD *>( &getActiveDevice() ) == NULL ) {
      ext::FPGADD &dd = ( ext::FPGADD & )( getParent()->getActiveDevice() );
      xtasks_task_handle parentTask = ( xtasks_task_handle )( dd.getHandle() );
      xtasks_stat status = xtasksNotifyFinishedTask( parentTask, 1 /*num finished tasks*/ );
      if ( status != XTASKS_SUCCESS ) {
         ensure( status == XTASKS_SUCCESS, " Error notifing FPGA about remote finished task" );
      }
   }

   WorkDescriptor::notifyParent();
}
