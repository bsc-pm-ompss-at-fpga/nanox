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

#ifndef _NANOS_FPGA_PROCESSOR
#define _NANOS_FPGA_PROCESSOR

#include "atomic.hpp"
#include "copydescriptor_decl.hpp"
#include "queue_decl.hpp"

#include "fpgadevice.hpp"
#include "fpgaconfig.hpp"
#include "cachedaccelerator.hpp"
#include "fpgapinnedallocator.hpp"
#include "fpgaprocessorinfo.hpp"
#include "fpgainstrumentation.hpp"
#include "libxtasks_wrapper.hpp"

namespace nanos {
namespace ext {

   class FPGAProcessor: public ProcessingElement
   {
      public:
         typedef Queue< WD * > FPGATasksQueue_t;

      private:
         Lock                          _execLock;           //!< Used to restrict the execution of tasks in this PE
         FPGAProcessorInfo             _fpgaProcessorInfo;  //!< Accelerator information
         Atomic<size_t>                _runningTasks;       //!< Tasks in the accelerator (running)
         FPGATasksQueue_t              _readyTasks;         //!< Tasks that are ready but are waiting for device memory
         FPGATasksQueue_t              _waitInTasks;        //!< Tasks that are ready but are waiting for input copies
#if defined(NANOS_DEBUG_ENABLED) || defined(NANOS_INSTRUMENTATION_ENABLED)
         Atomic<size_t>                _totalTasks;         //!< Total (acumulative) tasks executed in the accelerator
#endif

#ifdef NANOS_INSTRUMENTATION_ENABLED
         FPGAInstrumentation           _devInstr;
         static Atomic<size_t>         _totalRunningTasks;  //!< Global tasks counter between all processors
#endif

         // AUX functions
         void createTask( WD &wd, WD *parentWd );
         void submitTask( WD &wd );
      public:

         FPGAProcessor( FPGAProcessorInfo info, memory_space_id_t memSpaceId, Device const * arch );
         ~FPGAProcessor();

         inline FPGAProcessorInfo getFPGAProcessorInfo() const {
            return _fpgaProcessorInfo;
         }

         //Inherted from ProcessingElement
         WD & getWorkerWD () const;
         WD & getMasterWD () const;

         virtual WD & getMultiWorkerWD( DD::work_fct ) const {
            fatal( "getMasterWD(): FPGA processor is not allowed to create MultiThreads" );
         }

         BaseThread & createThread ( WorkDescriptor &wd, SMPMultiThread* parent );
         BaseThread & createMultiThread ( WorkDescriptor &wd, unsigned int numPEs, ProcessingElement **repPEs ) {
            fatal( "ClusterNode is not allowed to create FPGA MultiThreads" );
         }

         virtual bool hasSeparatedMemorySpace() const { return true; }
         bool supportsUserLevelThreads () const { return false; }
         FPGADeviceId getAccelId() const { return _fpgaProcessorInfo.getId(); }

         FPGAPinnedAllocator * getAllocator ( void );

         int getPendingWDs() const;

         FPGATasksQueue_t & getReadyTasks() { return _readyTasks; }
         FPGATasksQueue_t & getWaitInTasks() { return _waitInTasks; }

         void setTaskArg( WD &wd, size_t argIdx, bool isInput, bool isOutput, uint64_t argValue );

         virtual void switchHelperDependent( WD* oldWD, WD* newWD, void *arg ) {
            fatal("switchHelperDependent is not implemented in the FPGAProcessor");
         }
         virtual void exitHelperDependent( WD* oldWD, WD* newWD, void *arg ) {}
         virtual bool inlineWorkDependent (WD &work);
         virtual void switchTo( WD *work, SchedulerHelper *helper ) {}
         virtual void exitTo( WD *work, SchedulerHelper *helper ) {}
         virtual void outlineWorkDependent (WD &work);
         virtual void preOutlineWorkDependent (WD &work);
         bool tryPostOutlineTasks( size_t max = 9999 );

         virtual bool tryAcquireExecLock();
         virtual void releaseExecLock();
         bool isExecLockAcquired();

#if defined(NANOS_DEBUG_ENABLED) || defined(NANOS_INSTRUMENTATION_ENABLED)
         //! \breif Returns the number of tasks executed in the accelerator
         size_t getNumTasks() const {
            return _totalTasks.value();
         }
#endif
#ifdef NANOS_INSTRUMENTATION_ENABLED
         void handleInstrumentation();
#endif
   };

   inline FPGAPinnedAllocator * FPGAProcessor::getAllocator() {
      return ( FPGAPinnedAllocator * )( sys.getSeparateMemory(getMemorySpaceId()).getSpecificData() );
   }

   inline bool FPGAProcessor::isExecLockAcquired() { return _execLock.getState() == NANOS_LOCK_BUSY; }

   //! \brief Pointer to the vector of FPGA PEs
   extern std::vector< FPGAProcessor* > * fpgaPEs;

} // namespace ext
} // namespace nanos

#endif
