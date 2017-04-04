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

#include "fpgaconfig.hpp"
#include "plugin.hpp"
// We need to include system.hpp (to use verbose0(msg)), as debug.hpp does not include it
#include "system.hpp"

namespace nanos
{
   namespace ext
   {
      int  FPGAConfig::_numAccelerators = -1;
      int  FPGAConfig::_numAcceleratorsSystem = -1;
      int  FPGAConfig::_numFPGAThreads = -1;
      bool FPGAConfig::_disableFPGA = false;
      Lock FPGAConfig::_dmaLock;
      Atomic <int> FPGAConfig::_accelID(0);
      //TODO set sensible defaults (disabling transfers when necessary, etc.)
      unsigned int FPGAConfig::_burst = 8;
      int FPGAConfig::_maxTransfers = 32;
      int FPGAConfig::_idleSyncBurst = 4;
      bool FPGAConfig::_syncTransfers = false;
      int FPGAConfig::_fpgaFreq = 100; //default to 100MHz
      bool FPGAConfig::_hybridWorker = false;
      int FPGAConfig::_maxPendingWD = 8;
      int FPGAConfig::_finishWDBurst = 4;
      bool FPGAConfig::_idleCallback = true;
      std::size_t FPGAConfig::_allocatorPoolSize = 64;

      void FPGAConfig::prepare( Config &config )
      {
         config.setOptionsSection( "FPGA Arch", "FPGA spefific options" );
         config.registerConfigOption( "num-fpga" , NEW Config::IntegerVar( _numAccelerators ),
                                      "Defines de number of FPGA acceleratos to use (defaults to one)" );
         config.registerEnvOption( "num-fpga", "NX_FPGA_NUM" );
         config.registerArgOption( "num-fpga", "fpga-num" );

         config.registerConfigOption( "disable-fpga", NEW Config::FlagOption( _disableFPGA ),
                                      "Disable the use of FPGA accelerators" );
         config.registerEnvOption( "disable-fpga", "NX_DISABLE_FPGA" );
         config.registerArgOption( "disable-fpga", "disable-fpga" );

         config.registerConfigOption( "fpga-burst", NEW Config::UintVar( _burst ),
                 "Defines the number of transfers fo be waited in a row when the maximum active transfer is reached (-1 acts as unlimited)");
         config.registerEnvOption( "fpga-burst", "NX_FPGA_BURST" );
         config.registerArgOption( "fpga-burst", "fpga-burst" );

         config.registerConfigOption( "fpga_helper_threads", NEW Config::IntegerVar( _numFPGAThreads ),
                 "Defines de number of helper threads managing fpga accelerators");
         config.registerEnvOption( "fpga_helper_threads", "NX_FPGA_HELPER_THREADS" );
         config.registerArgOption( "fpga_helper_threads", "fpga-helper-threads" );

         config.registerConfigOption( "fpga_max_transfers", NEW Config::IntegerVar( _maxTransfers ),
                 "Defines the maximum number of active transfers per dma accelerator channel (-1 behaves as unlimited)" );
         config.registerEnvOption( "fpga_max_transfers", "NX_FPGA_MAX_TRANSFERS" );
         config.registerArgOption( "fpga_max_transfers", "fpga-max-transfers" );

         config.registerConfigOption( "fpga_idle_sync_burst", NEW Config::IntegerVar( _idleSyncBurst ),
               "Number of transfers synchronized when calling thread's idle" );
         config.registerEnvOption( "fpga_idle_sync_burst", "NX_FPGA_IDLE_SYNC_BURST" );
         config.registerArgOption( "fpga_idle_sync_burst", "fpga-idle-sync-burst" );

         config.registerConfigOption( "fpga_sync_transfers", NEW Config::FlagOption( _syncTransfers ),
               "Perform fpga transfers synchronously" );
         config.registerEnvOption( "fpga_sync_transfers", "NX_FPGA_SYNC_TRANSFERS" );
         config.registerArgOption( "fpga_sync_transfers", "fpga-sync-transfers" );

         config.registerConfigOption( "fpga_freq", NEW Config::IntegerVar( _fpgaFreq ),
               "FPGA accelerator clock frequency in MHz" );
         config.registerEnvOption( "fpga_freq", "NX_FPGA_FREQ" );
         config.registerArgOption( "fpga_freq", "nx-fpga-freq" );

         config.registerConfigOption( "fpga_hybrid_worker",
               NEW Config::FlagOption( _hybridWorker ),
              "Allow FPGA helper thread to run smp tasks" );
         config.registerEnvOption( "fpga_hybrid_worker", "NX_FPGA_HYBRID_WORKER" );
         config.registerArgOption( "fpga_hybrid_worker", "fpga-hybrid-worker" );

         config.registerConfigOption( "fpga_max_pending_tasks",
                 NEW Config::IntegerVar( _maxPendingWD ),
                 "Number of tasks allowed to be pending finalization for an fpga accelerator" );
         config.registerEnvOption( "fpga_max_pending_tasks", "NX_FPGA_MAX_PENDING_TASKS" );
         config.registerArgOption( "fpga_max_pending_tasks", "fpga-max-pending-tasks" );

         config.registerConfigOption( "fpga_finish_task_busrt",
                 NEW Config::IntegerVar( _finishWDBurst ),
                 "Number of tasks to be finalized in a burst when limit is reached" );
         config.registerEnvOption( "fpga_finish_task_busrt", "NX_FPGA_FINISH_TASK_BURST" );
         config.registerArgOption( "fpga_finish_task_busrt", "fpga-finish-task-burst" );

         config.registerConfigOption( "fpga_idle_callback", NEW Config::FlagOption( _idleCallback ),
               "Perform fpga operations using the IDLE event callback of Event Dispatcher (def: enabled)" );
         config.registerArgOption( "fpga_idle_callback", "fpga-idle-callback" );

         config.registerConfigOption( "fpga_alloc_pool_size", NEW Config::SizeVar( _allocatorPoolSize ),
               "Size (in MB) of the memory pool for the FPGA Tasks data copies (def: 64)" );
         config.registerEnvOption( "fpga_alloc_pool_size", "NX_FPGA_ALLOC_POOL_SIZE" );
         config.registerArgOption( "fpga_alloc_pool_size", "fpga-alloc-pool-size" );
      }

      void FPGAConfig::apply()
      {
         verbose0( "Initializing FPGA support component" );

         if ( _disableFPGA ) {
            _numAccelerators = 0; //system won't instanciate accelerators if count=0
            _numFPGAThreads = 0;
         } else if ( _numAccelerators < 0 ) {
            /* if not given, assume we are using one accelerator
             * We should get the number of accelerators on the system
             * and use it as a default
             */
            _numAccelerators = _numAcceleratorsSystem < 0 ? 1 : _numAcceleratorsSystem;
         } else if ( _numAccelerators > _numAcceleratorsSystem ) {
            warning0( "The number of FPGA accelerators is larger than the accelerators in the system. Using "
                     << _numAcceleratorsSystem << " accelerators." );
            _numAccelerators = _numAcceleratorsSystem;
         }
         _disableFPGA = _numAccelerators == 0;

         if ( _numFPGAThreads < 0 ) {
            //warning0( "Number of fpga threads cannot be negative. Using one thread per accelerator" );
            _numFPGAThreads = _numAccelerators;
         } else if ( _numFPGAThreads > _numAccelerators ) {
            warning0( "Number of FPGA helpers is greater than the number of FPGA accelerators. "
                     << "Using one thread per accelerator" );
            _numFPGAThreads = _numAccelerators;
         } else if ( _numFPGAThreads == 0 && !_idleCallback ) {
            warning0( "No FPGA helper threads requested and IDLE callback feature is disabled. "
                     << "The execution may not finish." );
         }
         _idleSyncBurst = ( _idleSyncBurst < 0 ) ? _burst : _idleSyncBurst;

      }

      void FPGAConfig::setFPGASystemCount ( int numFPGAs )
      {
         _numAcceleratorsSystem = numFPGAs;
      }
   }
}
