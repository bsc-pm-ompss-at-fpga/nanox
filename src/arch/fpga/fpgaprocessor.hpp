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
#include "compatibility.hpp"
#include "copydescriptor_decl.hpp"
#include "queue_decl.hpp"

#include "fpgadevice.hpp"
#include "fpgaconfig.hpp"
#include "cachedaccelerator.hpp"
#include "fpgapinnedallocator.hpp"

namespace nanos {
namespace ext {

//As in gpu, we could keep track of copied data

      //forward declaration of transfer list
      class FPGAMemoryTransferList;
      class FPGAProcessor: public ProcessingElement
      {
         public:
            class FPGAProcessorInfo;
            typedef Queue< WD * > FPGATasksQueue_t;

         private:
            typedef struct FPGATaskInfo_t {
               WD              *_wd;
               xdma_task_handle _handle;

               FPGATaskInfo_t( WD * const wd = NULL, xdma_task_handle h = 0 ) : _wd( wd ), _handle( h ) {}
            } FPGATaskInfo_t;

            int                           _accelId;            //!< Unique FPGA Accelerator identifier
            FPGAProcessorInfo            *_fpgaProcessorInfo;  //!< Accelerator information
            Queue< FPGATaskInfo_t >       _pendingTasks;       //!< Tasks in the accelerator (running)
            FPGATasksQueue_t              _readyTasks;         //!< Tasks that are ready but are waiting for device memory
            FPGATasksQueue_t              _waitInTasks;        //!< Tasks that are ready but are waiting for input copies

            FPGAMemoryTransferList       *_inputTransfers;     //< List of in stream transfers
            FPGAMemoryTransferList       *_outputTransfers;    //< List of out stream transfers

#ifdef NANOS_INSTRUMENTATION_ENABLED
            DeviceInstrumentation * _devInstr;
            DeviceInstrumentation * _dmaInInstr;
            DeviceInstrumentation * _dmaOutInstr;
            DeviceInstrumentation * _submitInstrumentation;
#endif

            // AUX functions
            void waitAndFinishTask( FPGATaskInfo_t & task );
            xdma_task_handle createAndSubmitTask( WD &wd );
#ifdef NANOS_INSTRUMENTATION_ENABLED
            void readInstrCounters( FPGATaskInfo_t & task );
            xdma_instr_times * getInstrCounters( FPGATaskInfo_t & task );
#endif

         public:

            FPGAProcessor( int const accId, memory_space_id_t memSpaceId, Device const * arch );
            ~FPGAProcessor();

            inline FPGAProcessorInfo * getFPGAProcessorInfo() const {
               return _fpgaProcessorInfo;
            }
            inline FPGAMemoryTransferList * getInTransferList() const {
               return _inputTransfers;
            }
            inline FPGAMemoryTransferList * getOutTransferList() const {
               return _outputTransfers;
            }

            /*! \brief Initialize hardware:
             *   * Open device
             *   * Get channels
             */
            void init();

            /*! \brief Deinit hardware
             *      Close channels and device
             */
            void cleanUp();

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
            bool isGPU () const { return false; }
            //virtual void waitInputs(WorkDescriptor& wd);
            int getAccelId() const { return _accelId; }

            /// \brief Override (disable) getAddres as this device does not have a dedicated memory nor separated address space
            // This avoids accessing the cache to retrieve a (null) address
            virtual void* getAddress(WorkDescriptor &wd, uint64_t tag, nanos_sharing_t sharing ) {
               fatal0("Calls to FPGAProcessor::getAddress() don't have a defined behaviour");
               return NULL;
            }

            FPGAPinnedAllocator * getAllocator ( void );

            int getPendingWDs() const;
            void finishPendingWD( int numWD );
            void finishAllWD();

            FPGATasksQueue_t & getReadyTasks() { return _readyTasks; }
            FPGATasksQueue_t & getWaitInTasks() { return _waitInTasks; }

#ifdef NANOS_INSTRUMENTATION_ENABLED
            /*! Defines if the warning has already been shown
             */
            bool _dmaSubmitWarnShown;

            void setDeviceInstrumentation( DeviceInstrumentation * devInstr ) {
               _devInstr = devInstr;
            }
            void setDmaInstrumentation( DeviceInstrumentation *dmaIn,
                    DeviceInstrumentation *dmaOut ) {
               _dmaInInstr = dmaIn;
               _dmaOutInstr = dmaOut;
            }
            void setSubmitInstrumentation( DeviceInstrumentation * submitInstr ) {
               _submitInstrumentation = submitInstr;
            }

            DeviceInstrumentation *getDeviceInstrumentation() {
               return _devInstr;
            }
            DeviceInstrumentation *getDmaInInstrumentation() {
               return _dmaInInstr;
            }
            DeviceInstrumentation *getDmaOutInstrumentation() {
               return _dmaOutInstr;
            }
            DeviceInstrumentation *getSubmitInstrumentation() {
               return _submitInstrumentation;
            }
#endif

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
