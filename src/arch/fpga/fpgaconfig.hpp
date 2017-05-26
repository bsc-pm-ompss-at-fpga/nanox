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

#ifndef _NANOS_FPGA_CFG
#define _NANOS_FPGA_CFG
#include "config.hpp"

#include "system_decl.hpp"
#include "fpgadevice_fwd.hpp"
#include "compatibility.hpp"

namespace nanos {
namespace ext {

      //! \brief Map with pairs {FPGADeviceType, num_instances}
      typedef TR1::unordered_map<FPGADeviceType, size_t> FPGATypesMap;

      class FPGAConfig
      {
            friend class FPGAPlugin;
         private:
            static bool                      _enableFPGA; //! Enable all CUDA support
            static bool                      _forceDisableFPGA; //! Force disable all CUDA support

            static int                       _numAccelerators; //! Number of accelerators used in the execution
            static int                       _numAcceleratorsSystem; //! Number of accelerators detected in the system
            static int                       _numFPGAThreads; //! Number of FPGA helper threads
            static unsigned int              _burst;
            static int                       _maxTransfers;
            static int                       _idleSyncBurst;
            static bool                      _syncTransfers;
            static int                       _fpgaFreq;
            static bool                      _hybridWorker;
            static int                       _maxPendingWD;
            static int                       _finishWDBurst;
            static bool                      _idleCallback;
            static std::size_t               _allocatorPoolSize;
            static std::string              *_configFile; //! Path of FPGA configuration file (used to generate _accTypesMap)
            static FPGATypesMap             *_accTypesMap;

            /*! Parses the FPGA user options */
            static void prepare ( Config &config );

            /*! Applies the configuration options
             *  NOTE: Should be called after call 'setFPGASystemCount'
             */
            static void apply ( void );

         public:
            static void printConfiguration( void );
            static int getFPGACount ( void ) { return _numAccelerators; }
            static inline unsigned int getBurst() { return _burst; }
            static inline unsigned int getMaxTransfers() { return _maxTransfers; }
            static inline int getAccPerThread() { return _numAccelerators/_numFPGAThreads; }
            static inline int getNumFPGAThreads() { return _numFPGAThreads; }

            //! \brief Returns if the FPGA support is enabled
            static inline bool isEnabled() { return _enableFPGA; }

            /*! \brief Returns if the FPGA support may be enabled after call apply.
             *         This method can be called before the apply (and setFPGASystemCount) to Check
                       if the support is expected to be enabled or not.
             */
            static bool mayBeEnabled();

            //should be areSyncTransfersDisabled() but is for consistency with other bool getters
            static inline int getIdleSyncBurst() { return _idleSyncBurst; }
            static bool getSyncTransfersEnabled() { return _syncTransfers; }

            //! \brief Returns cycle time in ns
            static unsigned int getCycleTime() {
                return 1000/_fpgaFreq; //_fpgaFreq is in MHz
            }
            static bool getHybridWorkerEnabled() { return _hybridWorker; }
            static int getMaxPendingWD() { return  _maxPendingWD; }
            static int getFinishWDBurst() { return _finishWDBurst; }
            static bool getIdleCallbackEnabled() { return _idleCallback; }

            //! \brief Returns FPGA Allocator size in MB
            static std::size_t getAllocatorPoolSize() { return _allocatorPoolSize; }

            //! \brief Returns a map with the accelerators types and number of instances
            static FPGATypesMap& getAccTypesMap() { return *_accTypesMap; }

            //! \brief Sets the number of FPGAs and return the old value
            static void setFPGASystemCount ( int numFPGAs );
      };
       //create instrumentation macros (as gpu) to make code cleaner
#define NANOS_FPGA_CREATE_RUNTIME_EVENT(x)    NANOS_INSTRUMENT( \
		sys.getInstrumentation()->raiseOpenBurstEvent (    \
         sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey( "in-xdma" ), (x) ); )

#define NANOS_FPGA_CLOSE_RUNTIME_EVENT       NANOS_INSTRUMENT( \
      sys.getInstrumentation()->raiseCloseBurstEvent (   \
         sys.getInstrumentation()->getInstrumentationDictionary()->getEventKey( "in-xdma" ), 0 ); )

      typedef enum {
         NANOS_FPGA_NULL_EVENT = 0,      // 0
         NANOS_FPGA_OPEN_EVENT,
         NANOS_FPGA_CLOSE_EVENT,
         NANOS_FPGA_REQ_CHANNEL_EVENT,
         NANOS_FPGA_REL_CHANNEL_EVENT,
         NANOS_FPGA_SUBMIT_IN_DMA_EVENT,    // 5
         NANOS_FPGA_SUBMIT_OUT_DMA_EVENT,
         NANOS_FPGA_WAIT_INPUT_DMA_EVENT,
         NANOS_FPGA_WAIT_OUTPUT_DMA_EVENT
      }in_xdma_event_value;

} // namespace ext
} // namespace nanos
#endif
