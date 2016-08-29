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
         private:
            //! FPGA device ID
            Atomic<int> _fpgaDevice;
            static Lock _initLock;  ///Initialization lock (may only be needed if we dynamically spawn fpga helper threads)
            FPGAProcessorInfo *_fpgaProcessorInfo;
            static int _accelSeed;  ///Keeps track of the created accelerators
            int _accelBase;         ///Base of the range of assigned accelerators
            SMPProcessor *_core;

            FPGAMemoryTransferList *_inputTransfers;
            FPGAMemoryTransferList *_outputTransfers;

            static FPGAPinnedAllocator _allocator;
            std::map <WD*, xdma_task_handle> _pendingTasks;

#ifdef NANOS_INSTRUMENTATION_ENABLED
            DeviceInstrumentation * _devInstr;
            DeviceInstrumentation * _dmaInInstr;
            DeviceInstrumentation * _dmaOutInstr;
            DeviceInstrumentation * _submitInstrumentation;
#endif

         public:

            FPGAProcessor(const Device *arch, memory_space_id_t memSpaceId);
            ~FPGAProcessor();

            FPGAProcessorInfo* getFPGAProcessorInfo() const {
               return _fpgaProcessorInfo;
            }
            FPGAMemoryTransferList *getInTransferList() const {
               return _inputTransfers;
            }
            FPGAMemoryTransferList* getOutTransferList() const {
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

            bool supportsUserLevelThreads () const { return false; }
            bool isGPU () const { return false; }
            //virtual void waitInputs(WorkDescriptor& wd);
            int getAccelBase() const { return _accelBase; }

            /// \brief Override (disable) getAddres as this device does not have a dedicated memory nor separated address space
            // This avoids accessing the cache to retrieve a (null) address
            virtual void* getAddress(WorkDescriptor &wd, uint64_t tag, nanos_sharing_t sharing ) {return NULL;}

            //BaseThread &startFPGAThread();
            static  FPGAPinnedAllocator& getPinnedAllocator() { return _allocator; }

            void createAndSubmitTask( WD &wd );
            void waitTask( WD *wd );
            void deleteTask( WD *wd );
            xdma_instr_times * getInstrCounters( WD *wd );


#ifdef NANOS_INSTRUMENTATION_ENABLED
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

      };
} // namespace ext
} // namespace nanos

#endif
