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

#include "libxtasks.h"

namespace nanos {
namespace ext {

   class FPGAProcessor: public ProcessingElement
   {
      public:
         typedef Queue< WD * > FPGATasksQueue_t;

      private:
         typedef struct FPGATaskInfo_t {
            WD                 *_wd;
            xtasks_task_handle _handle;

            FPGATaskInfo_t( WD * const wd = NULL, xtasks_task_handle h = 0 ) :
               _wd( wd ), _handle( h ) {}
         } FPGATaskInfo_t;

         FPGAProcessorInfo             _fpgaProcessorInfo;  //!< Accelerator information
         Queue< FPGATaskInfo_t >       _pendingTasks;       //!< Tasks in the accelerator (running)
         FPGATasksQueue_t              _readyTasks;         //!< Tasks that are ready but are waiting for device memory
         FPGATasksQueue_t              _waitInTasks;        //!< Tasks that are ready but are waiting for input copies

#ifdef NANOS_INSTRUMENTATION_ENABLED
         FPGAInstrumentation           _devInstr;
         FPGAInstrumentation           _dmaInInstr;
         FPGAInstrumentation           _dmaOutInstr;
         FPGAInstrumentation           _submitInstrumentation;
         bool                          _dmaSubmitWarnShown; //!< Defines if the warning has already been shown
#endif

         // AUX functions
         void waitAndFinishTask( FPGATaskInfo_t & task );
         xtasks_task_handle createAndSubmitTask( WD &wd );
#ifdef NANOS_INSTRUMENTATION_ENABLED
         void dmaSubmitStart( const WD *wd );
         void dmaSubmitEnd( const WD *wd );
         void readInstrCounters( FPGATaskInfo_t & task );
         xtasks_ins_times * getInstrCounters( FPGATaskInfo_t & task );
#endif

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
         void finishPendingWD( int numWD );
         void finishAllWD();

         FPGATasksQueue_t & getReadyTasks() { return _readyTasks; }
         FPGATasksQueue_t & getWaitInTasks() { return _waitInTasks; }

         virtual void switchHelperDependent( WD* oldWD, WD* newWD, void *arg ) {
            fatal("switchHelperDependent is not implemented in the FPGAProcessor");
         }
         virtual void exitHelperDependent( WD* oldWD, WD* newWD, void *arg ) {}
         virtual bool inlineWorkDependent (WD &work);
         virtual void switchTo( WD *work, SchedulerHelper *helper ) {}
         virtual void exitTo( WD *work, SchedulerHelper *helper ) {}
         virtual void outlineWorkDependent (WD &work);
         virtual void preOutlineWorkDependent (WD &work);

   };

   inline FPGAPinnedAllocator * FPGAProcessor::getAllocator() {
      return ( FPGAPinnedAllocator * )( sys.getSeparateMemory(getMemorySpaceId()).getSpecificData() );
   }

} // namespace ext
} // namespace nanos

#endif
