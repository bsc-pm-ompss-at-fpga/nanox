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

#ifndef _NANOS_FPGA_CFG
#define _NANOS_FPGA_CFG
#include "config.hpp"

#include "system_decl.hpp"
#include "compatibility.hpp"

namespace nanos {
namespace ext {

   class FPGAConfig
   {
      friend class FPGAPlugin;
      private:
         static bool                      _enableFPGA; //! Enable all FPGA support
         static bool                      _forceDisableFPGA; //! Force disable all FPGA support
#ifdef NANOS_INSTRUMENTATION_ENABLED
         static bool                      _disableInst; //! Disable FPGA instrumentation using HW timer
         static size_t                    _numEvents; //! Max fpga events
         static bool                      _insCallback; //! Idle callback for FPGA instrumentation handling
#endif //NANOS_INSTRUMENTATION_ENABLED

         static int                       _numAccelerators; //! Number of accelerators used in the execution
         static int                       _numAcceleratorsSystem; //! Number of accelerators detected in the system
         static int                       _numFPGAThreads; //! Number of FPGA helper threads
         static bool                      _hybridWorker;
         static int                       _maxPendingWD;
         static int                       _finishWDBurst;
         static bool                      _idleCallback;
         static bool                      _idleCreateCallback; //! Idle callback for fpga creation enable
         static bool                      _createCallbackRegistered; //! Idle callback for fpga creation has been already registered
         static bool                      _disableIdleCreateCallback; //! Must Idle callback for fpga creation be disabled
         static int                       _maxThreadsIdleCallback;
         static std::size_t               _allocatorPoolSize;
         static std::size_t               _allocAlign;

         /*! Parses the FPGA user options */
         static void prepare ( Config &config );

         /*! Applies the configuration options
          *  NOTE: Should be called after call 'setFPGASystemCount'
          */
         static void apply ( void );

      public:
         static void printConfiguration( void );
         static int getFPGACount ( void ) { return _numAccelerators; }
         static inline int getAccPerThread() { return _numAccelerators/_numFPGAThreads; }
         static inline int getNumFPGAThreads() { return _numFPGAThreads; }

         /*! \brief Returns if the FPGA support is enabled.
          *         NOTE: It may be enabled in the apply() method
          */
         static inline bool isEnabled() { return _enableFPGA; }

         /*! \brief Returns if the FPGA support is disabled and won't be enabled in apply()
          */
         static bool isDisabled();
         static bool getHybridWorkerEnabled() { return _hybridWorker; }
         static int getMaxPendingWD() { return  _maxPendingWD; }
         static int getFinishWDBurst() { return _finishWDBurst; }
         static bool getIdleCallbackEnabled() { return _idleCallback; }
         static bool getIdleCreateCallbackEnabled() { return _idleCreateCallback; }
         static bool isIdleCreateCallbackRegistered() { return _createCallbackRegistered; }
         static bool forceDisableIdleCreateCallback() { return _disableIdleCreateCallback; }
         static int getMaxThreadsIdleCallback() { return _maxThreadsIdleCallback; }

         //! \brief Returns FPGA Allocator size requested by user in bytes
         static std::size_t getAllocatorPoolSize() { return _allocatorPoolSize; }
         static std::size_t getDefaultAllocatorPoolSize() { return 512*1024*1024; }

         //! \brief Sets the number of FPGAs
         static void setFPGASystemCount ( int numFPGAs );

         //! \brief Set that the fpga callback for task creation has been registered
         static void setIdleCreateCallbackRegistered();

         static int getAllocAlign() { return _allocAlign; }

#ifdef NANOS_INSTRUMENTATION_ENABLED
         //! \brief Returns if the instrumentation using the HW timer is disabled
         static bool isInstrDisabled() { return _disableInst; }

         //! \brief Set the disable instrumentation flag to true
         static void forceDisableInstr();

         //! \brief Retuns number of maximum instrumentation events to be read from fpga
         static int getNumInstrEvents() { return _numEvents; }

         //! \brief Retuns whether the FPGA instrumentation callback must be enabled or not
         static bool getInstrumentationCallbackEnabled() { return _insCallback; }
#endif //NANOS_INSTRUMENTATION_ENABLED
   };

} // namespace ext
} // namespace nanos
#endif
