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

#include "fpgaconfig.hpp"
#include "plugin.hpp"
// We need to include system.hpp (to use verbose0(msg)), as debug.hpp does not include it
#include "system.hpp"
#include "os.hpp"

#include <fstream>

//This symbol is used to detect that a specific feature of OmpSs is used in an application
// (i.e. Mercurium explicitly defines one of these symbols if they are used)
extern "C"
{
   __attribute__((weak)) void nanos_needs_fpga_fun(void);
}

namespace nanos {
namespace ext {

bool FPGAConfig::_enableFPGA = false;
bool FPGAConfig::_forceDisableFPGA = false;
int  FPGAConfig::_numAccelerators = -1;
int  FPGAConfig::_numAcceleratorsSystem = -1;
int  FPGAConfig::_numFPGAThreads = -1;
bool FPGAConfig::_hybridWorker = true;
int FPGAConfig::_maxPendingWD = 4;
int FPGAConfig::_finishWDBurst = 8;
bool FPGAConfig::_idleCallback = true;
bool FPGAConfig::_idleCreateCallback = false;
bool FPGAConfig::_disableIdleCreateCallback = false;
bool FPGAConfig::_createCallbackRegistered = false;
int FPGAConfig::_maxThreadsIdleCallback = 1;
std::size_t FPGAConfig::_allocatorPoolSize = 0;
std::size_t FPGAConfig::_allocAlign = 16;
#ifdef NANOS_INSTRUMENTATION_ENABLED
   bool FPGAConfig::_disableInst = false;
   size_t FPGAConfig::_numEvents = 4096/24; //Number of events that fit in a page
   bool FPGAConfig::_insCallback = true;
#endif //NANOS_INSTRUMENTATION_ENABLED

void FPGAConfig::prepare( Config &config )
{
   config.setOptionsSection( "FPGA Arch", "FPGA spefific options" );

   config.registerConfigOption( "fpga-enable", NEW Config::FlagOption( _enableFPGA ),
                                "Enable the support for FPGA accelerators and allocator" );
   config.registerEnvOption( "fpga-enable", "NX_FPGA_ENABLE" );
   config.registerArgOption( "fpga-enable", "fpga-enable" );

   config.registerConfigOption( "fpga-disable", NEW Config::FlagOption( _forceDisableFPGA ),
                                "Disable the support for FPGA accelerators and allocator" );
   config.registerEnvOption( "fpga-disable", "NX_FPGA_DISABLE" );
   config.registerArgOption( "fpga-disable", "fpga-disable" );

   config.registerConfigOption( "num-fpga" , NEW Config::IntegerVar( _numAccelerators ),
      "Defines de number of FPGA acceleratos to use (def: #accels from libxtasks)" );
   config.registerEnvOption( "num-fpga", "NX_FPGA_NUM" );
   config.registerArgOption( "num-fpga", "fpga-num" );

   config.registerConfigOption( "fpga_helper_threads", NEW Config::IntegerVar( _numFPGAThreads ),
      "Defines de number of helper threads managing fpga accelerators (def: 1)");
   config.registerEnvOption( "fpga_helper_threads", "NX_FPGA_HELPER_THREADS" );
   config.registerArgOption( "fpga_helper_threads", "fpga-helper-threads" );

   config.registerConfigOption( "fpga_hybrid_worker", NEW Config::FlagOption( _hybridWorker ),
                                "Allow FPGA helper thread to run smp tasks (def: enabled)" );
   config.registerEnvOption( "fpga_hybrid_worker", "NX_FPGA_HYBRID_WORKER" );
   config.registerArgOption( "fpga_hybrid_worker", "fpga-hybrid-worker" );

   config.registerConfigOption( "fpga_max_pending_tasks", NEW Config::IntegerVar( _maxPendingWD ),
      "Number of tasks allowed to be pending finalization for an fpga accelerator (def: 4)" );
   config.registerEnvOption( "fpga_max_pending_tasks", "NX_FPGA_MAX_PENDING_TASKS" );
   config.registerArgOption( "fpga_max_pending_tasks", "fpga-max-pending-tasks" );

   config.registerConfigOption( "fpga_finish_task_busrt", NEW Config::IntegerVar( _finishWDBurst ),
      "Max number of tasks to be finalized in a burst when limit is reached (def: 8)" );
   config.registerEnvOption( "fpga_finish_task_busrt", "NX_FPGA_FINISH_TASK_BURST" );
   config.registerArgOption( "fpga_finish_task_busrt", "fpga-finish-task-burst" );

   config.registerConfigOption( "fpga_idle_callback", NEW Config::FlagOption( _idleCallback ),
      "Perform fpga operations using the IDLE event callback of Event Dispatcher (def: enabled)" );
   config.registerArgOption( "fpga_idle_callback", "fpga-idle-callback" );

   config.registerConfigOption( "fpga_create_callback", NEW Config::FlagOption( _idleCreateCallback ),
      "Register the task creation callback during the plugin initialization (def: false - automatically enabled when needed)" );
   config.registerArgOption( "fpga_create_callback", "fpga-create-callback" );

   config.registerConfigOption( "fpga_create_callback_disable", NEW Config::FlagOption( _disableIdleCreateCallback ),
      "Disable the registration of the task creation callback to handle task creation from the FPGA (def: false)" );
   config.registerArgOption( "fpga_create_callback_disable", "fpga-create-callback-disable" );

   config.registerConfigOption( "fpga_max_threads_callback", NEW Config::IntegerVar( _maxThreadsIdleCallback ),
      "Max. number of threads concurrently working in the FPGA IDLE callback (def: 1)" );
   config.registerEnvOption( "fpga_max_threads_callback", "NX_FPGA_MAX_THREADS_CALLBACK" );
   config.registerArgOption( "fpga_max_threads_callback", "fpga-max-threads-callback" );

   config.registerConfigOption( "fpga_alloc_pool_size", NEW Config::SizeVar( _allocatorPoolSize ),
      "FPGA device memory pool size (def: 512MB)" );
   config.registerEnvOption( "fpga_alloc_pool_size", "NX_FPGA_ALLOC_POOL_SIZE" );
   config.registerArgOption( "fpga_alloc_pool_size", "fpga-alloc-pool-size" );

   config.registerConfigOption( "fpga_alloc_align", NEW Config::SizeVar( _allocAlign ),
         "FPGA allocation alignment (def: 16)" );
   config.registerEnvOption( "fpga_alloc_align", "NX_FPGA_ALLOC_ALIGN" );
   config.registerArgOption( "fpga_alloc_align", "fpga-alloc-align" );

#ifdef NANOS_INSTRUMENTATION_ENABLED
   config.registerConfigOption( "fpga_max_instr_events", NEW Config::SizeVar( _numEvents ),
         "Maximum number of events to be saved from a FPGA task (def: 170)");
   config.registerEnvOption( "fpga_max_instr_events", "FPGA_MAX_INSTR_EVENTS" );
   config.registerArgOption( "fpga_max_instr_events", "fpga-max-instr-events" );

   config.registerConfigOption( "fpga_ins_callback", NEW Config::FlagOption( _insCallback ),
      "Handle the FPGA instrumentation using the IDLE event callback of Event Dispatcher (def: enabled)" );
   config.registerArgOption( "fpga_ins_callback", "fpga-instrumentation-callback" );
#endif //NANOS_INSTRUMENTATION_ENABLED
}

void FPGAConfig::apply()
{
   bool userWantsFpga = _enableFPGA;

   // Enable support if Mercurium or user requires it
   _enableFPGA = userWantsFpga || nanos_needs_fpga_fun;

   if ( _forceDisableFPGA || !_enableFPGA || _numAccelerators == 0 ||
      ( _numAcceleratorsSystem <= 0 && !userWantsFpga ) ||
      ( _numFPGAThreads == 0 && !_idleCallback && !userWantsFpga ) )
   {
      // The current configuration disables the FPGA support
      if ( nanos_needs_fpga_fun ) {
         warning0( " FPGA tasks were compiled and FPGA was disabled, execution could have " <<
                   "unexpected behaviour and can even hang, check configuration parameters" );
      }
      _enableFPGA = false;
      _numAccelerators = 0;
      _numFPGAThreads = 0;
      _idleCallback = false;
   } else if ( _numAccelerators < 0 || _numAccelerators > _numAcceleratorsSystem ) {
      // The number of accelerators available in the system has to be used
      if ( _numAccelerators > _numAcceleratorsSystem ) {
         warning0( " The number of FPGA accelerators is larger than the accelerators in the system."
            << " Using " << _numAcceleratorsSystem << " accelerators." );

      }
      _numAccelerators = _numAcceleratorsSystem;
   }

   if ( _numFPGAThreads < 0 ) {
      _numFPGAThreads = 1;
   } else if ( _numFPGAThreads > _numAccelerators ) {
      warning0( " Number of FPGA helper threads is larger than the number of FPGA accelerators." );
      //         << "Using one thread per accelerator (" << _numAccelerators << ")" );
      //_numFPGAThreads = _numAccelerators;
   }

   if ( _idleCreateCallback && _disableIdleCreateCallback ) {
      warning0( " The FPGA task creation callback will not be registered during plugin initialization because " <<
                " the disable task creation callback flag present." );
      _idleCreateCallback = false;
   }

   if ( _numFPGAThreads > 0 && !_idleCallback && _hybridWorker ) {
      warning0( " The use of FPGA idle callback is disabled, execution could have unexpected " <<
                " behaviour and can ever hang if there is task nesting." );
   }
}

void FPGAConfig::setFPGASystemCount ( int numFPGAs )
{
   _numAcceleratorsSystem = numFPGAs;
}

bool FPGAConfig::isDisabled ()
{
   return _forceDisableFPGA || !( _enableFPGA || nanos_needs_fpga_fun );
}

void FPGAConfig::setIdleCreateCallbackRegistered ()
{
   _createCallbackRegistered = true;
}

#ifdef NANOS_INSTRUMENTATION_ENABLED
void FPGAConfig::forceDisableInstr ()
{
   _disableInst = true;
}
#endif //NANOS_INSTRUMENTATION_ENABLED

} // namespace ext
} // namespace nanos
