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
      /*!
       * Thread that has to be represented when the callback is executed
       * This is needed to allow the SMPThread get ready FPGA WDs
       */
      FPGAThread * _fpgaThread;

      /*!
       * Returns the pointer to the FPGAThread associated to this listener
       */
      FPGAThread * getFPGAThread();
   public:
      /*!
       * \brief Default constructor
       * \param [in] thread      Pointer to the thread that the callback simulates
       * \param [in] ownsThread  Flag that defines if thread is exclusively for the listener and
       *                         must be deleted with the listener
       */
      FPGAListener( FPGAThread * thread, const bool ownsThread = false );
      ~FPGAListener();
      void callback( BaseThread * thread );

      /*!
       * Aux function used to create the SMPMultiThread when the there are no helper threads
       */
      static void FPGAWorkerLoop();
};

FPGAListener::FPGAListener( FPGAThread * thread, const bool onwsThread )
{
   union { FPGAThread * p; intptr_t i; } u = { thread };
   // Set the own status
   u.i |= int( onwsThread );
   _fpgaThread = u.p;
}

FPGAListener::~FPGAListener()
{
   union { BaseThread * p; intptr_t i; } u = { _fpgaThread };
   bool deleteThread = (u.i & 1);

   /*
   * The FPGAThread is only associated to the FPGAListener
   * The thread must be deleted before deleting the listener
   */
   if ( deleteThread ) {
      BaseThread * self = myThread;
      BaseThread * fpga = getFPGAThread();
      _fpgaThread = NULL;

      // Simulate be the BaseThread that is being deleted
      myThread = fpga;
      fpga->leaveTeam();
      fpga->join();

      // Restore the real thread identity and delete the FPGAThread
      myThread = self;
      delete fpga;
   }
}

inline FPGAThread * FPGAListener::getFPGAThread()
{
   union { FPGAThread * p; intptr_t i; } u = { _fpgaThread };
   // Clear the own status if set
   u.i &= ((~(intptr_t)0) << 1);
   return u.p;
}

void FPGAListener::callback( BaseThread* self )
{
   FPGAThread * thread = getFPGAThread();
   ensure( thread != NULL, "FPGAThread pointer in the FPGAListener cannot be NULL" );
   if (!thread->_lock.tryAcquire()) return;
   myThread = thread;

   int maxPendingWD = FPGAConfig::getMaxPendingWD();
   int finishBurst = FPGAConfig::getFinishWDBurst();
   for (;;) {
      if ( thread->getPendingWDs() > maxPendingWD ) {
          thread->finishPendingWD( finishBurst );
      }

      if ( !self->isRunning() ) break;
      //get next WD
      WD *wd = FPGAWorker::getFPGAWD( thread );
      if ( wd ) {
         Scheduler::prePreOutlineWork(wd);
         if ( Scheduler::tryPreOutlineWork(wd) ) {
            thread->runningOn()->preOutlineWorkDependent( *wd );
         }
         //TODO: may need to increment copies version number here
         if ( wd->isInputDataReady() ) {
            Scheduler::outlineWork( thread, wd );
         } else {
            //do whatever is needed if input is not ready
            //wait or whatever, for instance, sync needed copies
         }
         //add to the list of pending WD
         wd->submitOutputCopies();
         thread->addPendingWD( wd );

         //Scheduler::postOutlineWork( wd, false, thread ); <--moved to fpga thread
      } else {
         thread->finishPendingWD( finishBurst );
         break;
      }
   }

   myThread = self;
   thread->_lock.release();
}

void FPGAListener::FPGAWorkerLoop() {
   fatal( "This method (FPGAListener::FPGAWorkerLoop) never should be called!" );
}

} /* namespace ext */
} /* namespace nanos */
