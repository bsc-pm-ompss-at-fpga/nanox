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

#ifndef _NANOS_FPGA_THREAD
#define _NANOS_FPGA_THREAD

#include <queue>

#include "fpgaprocessor.hpp"
#include "smpthread.hpp"
#include "libxdma.h"

namespace nanos {
namespace ext {

   class FPGAThread : public BaseThread
   {
      public:
         //FPGAThread(WD &wd, PE *pe, SMPProcessor *core, Atomic<int> fpgaDevice) :
            //SMPThread(wd, pe, core), _pendingWD(), _hwInstrCounters() {}
         FPGAThread(WD &wd, PE *pe, SMPMultiThread *parent, Atomic<int> fpgaDevice);

         void initializeDependent( void );
         void runDependent ( void );
         bool inlineWorkDependent( WD &work );
         virtual void preOutlineWorkDependent ( WD &work );
         virtual void outlineWorkDependent ( WD &work );

         void yield();
         void idle( bool debug );

         int getPendingWDs() const;
         void finishPendingWD( int numWD );
         void addPendingWD( WD *wd );
         void finishAllWD();

         virtual void switchTo( WD *work, SchedulerHelper *helper );
         virtual void exitTo( WD *work, SchedulerHelper *helper );
         virtual void switchHelperDependent( WD* oldWD, WD* newWD, void *arg );
         virtual void exitHelperDependent( WD* oldWD, WD* newWD, void *arg );
         virtual void switchToNextThread();

         virtual void start() {}
         virtual void join() {}
         virtual BaseThread *getNextThread();
         virtual bool isCluster() { return false; }



#ifdef NANOS_INSTRUMENTATION_ENABLED
         void setupTaskInstrumentation( WD *wd );
         void submitInstrSync( WD *wd );
#endif

      private:
         std::queue< WD* > _pendingWD;
         std::map< WD*, xdma_instr_times* > _hwInstrCounters;
         std::map< WD*, xdma_transfer_handle > _instrSyncHandles;
         xdma_buf_handle _syncHandle;
         unsigned int *_syncBuffer;
#ifdef NANOS_INSTRUMENTATION_ENABLED
         void readInstrCounters( WD *wd );
#endif
   };
} // namespace ext
} // namespace nanos

#endif
