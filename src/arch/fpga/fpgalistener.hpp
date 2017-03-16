/*************************************************************************************/
/*      Copyright 2017 Barcelona Supercomputing Center                               */
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

#include "eventdispatcher_decl.hpp"
#include "fpgathread.hpp"
#include "fpgaconfig.hpp"

namespace nanos {
namespace ext {

class FPGAListener : public EventListener {
   private:
      FPGAThread* _fpgaThread;
   public:
     FPGAListener( FPGAThread* thread ) : _fpgaThread( thread ) {}
     ~FPGAListener() {}
     void callback( BaseThread* thread );
};

void FPGAListener::callback( BaseThread* thread )
{
   if (!_fpgaThread->_lock.tryAcquire()) return;
   myThread = _fpgaThread;

   int maxPendingWD = FPGAConfig::getMaxPendingWD();
   int finishBurst = FPGAConfig::getFinishWDBurst();
   for (;;) {
      //check if we have reached maximum pending WD
      //  finalize one (or some of them)
      //FPGAThread *myThread = (FPGAThread*)getMyThreadSafe();

      if ( _fpgaThread->getPendingWDs() > maxPendingWD ) {
          _fpgaThread->finishPendingWD( finishBurst );
      }

      if ( !thread->isRunning() ) break;
      //get next WD
      WD *wd = FPGAWorker::getFPGAWD( _fpgaThread );
      if ( wd ) {
         Scheduler::prePreOutlineWork(wd);
         if ( Scheduler::tryPreOutlineWork(wd) ) {
            _fpgaThread->preOutlineWorkDependent( *wd );
         }
         //TODO: may need to increment copies version number here
         if ( wd->isInputDataReady() ) {
            Scheduler::outlineWork( _fpgaThread, wd );
         } else {
            //do whatever is needed if input is not ready
            //wait or whatever, for instance, sync needed copies
         }
         //add to the list of pending WD
         wd->submitOutputCopies();
         _fpgaThread->addPendingWD( wd );

         //Scheduler::postOutlineWork( wd, false, myThread ); <--moved to fpga thread
      } else {
         break;
      }
   }

   myThread = thread;
   _fpgaThread->_lock.release();
}

} /* namespace ext */
} /* namespace nanos */
