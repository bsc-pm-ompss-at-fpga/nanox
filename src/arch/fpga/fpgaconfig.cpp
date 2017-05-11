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
//TODO set sensible defaults (disabling transfers when necessary, etc.)
unsigned int FPGAConfig::_burst = 8;
int FPGAConfig::_maxTransfers = 32;
int FPGAConfig::_idleSyncBurst = 4;
int FPGAConfig::_fpgaFreq = 100; //default to 100MHz
bool FPGAConfig::_hybridWorker = false;
int FPGAConfig::_maxPendingWD = 8;
int FPGAConfig::_finishWDBurst = 4;
bool FPGAConfig::_idleCallback = true;
std::size_t FPGAConfig::_allocatorPoolSize = 64;
std::string * FPGAConfig::_configFile = NULL;
FPGATypesMap * FPGAConfig::_accTypesMap = NULL;

void FPGAConfig::prepare( Config &config )
{
   config.setOptionsSection( "FPGA Arch", "FPGA spefific options" );

   config.registerConfigOption( "fpga-enable", NEW Config::FlagOption( _enableFPGA ),
                                "Enable the support for FPGA accelerators" );
   config.registerEnvOption( "fpga-enable", "NX_FPGA_ENABLE" );
   config.registerArgOption( "fpga-enable", "fpga-enable" );

   config.registerConfigOption( "fpga-disable", NEW Config::FlagOption( _forceDisableFPGA ),
                                "Disable the support for FPGA accelerators" );
   config.registerEnvOption( "fpga-disable", "NX_FPGA_DISABLE" );
   config.registerArgOption( "fpga-disable", "fpga-disable" );

   config.registerConfigOption( "num-fpga" , NEW Config::IntegerVar( _numAccelerators ),
      "Defines de number of FPGA acceleratos to use (default: number of accelerators detected in system)" );
   config.registerEnvOption( "num-fpga", "NX_FPGA_NUM" );
   config.registerArgOption( "num-fpga", "fpga-num" );

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

   config.registerConfigOption( "fpga_freq", NEW Config::IntegerVar( _fpgaFreq ),
                                "FPGA accelerator clock frequency in MHz" );
   config.registerEnvOption( "fpga_freq", "NX_FPGA_FREQ" );
   config.registerArgOption( "fpga_freq", "nx-fpga-freq" );

   config.registerConfigOption( "fpga_hybrid_worker", NEW Config::FlagOption( _hybridWorker ),
                                "Allow FPGA helper thread to run smp tasks" );
   config.registerEnvOption( "fpga_hybrid_worker", "NX_FPGA_HYBRID_WORKER" );
   config.registerArgOption( "fpga_hybrid_worker", "fpga-hybrid-worker" );

   config.registerConfigOption( "fpga_max_pending_tasks", NEW Config::IntegerVar( _maxPendingWD ),
      "Number of tasks allowed to be pending finalization for an fpga accelerator" );
   config.registerEnvOption( "fpga_max_pending_tasks", "NX_FPGA_MAX_PENDING_TASKS" );
   config.registerArgOption( "fpga_max_pending_tasks", "fpga-max-pending-tasks" );

   config.registerConfigOption( "fpga_finish_task_busrt", NEW Config::IntegerVar( _finishWDBurst ),
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

   _accTypesMap = new FPGATypesMap();
   _configFile = new std::string();
   config.registerConfigOption( "fpga_acc_types_map", NEW Config::StringVar( *_configFile ),
      "List with the number of accelerators for each type. Default is [fpga_num] which means that all accelerators have the same type" );
   config.registerEnvOption( "fpga_acc_types_map", "NX_FPGA_CONFIG_FILE" );
}

void FPGAConfig::apply()
{
   verbose0( "Initializing FPGA support component" );

   //Auto-enable support if Mercurium requires it
   _enableFPGA = _enableFPGA || nanos_needs_fpga_fun;

   if ( _forceDisableFPGA || !_enableFPGA || _numAccelerators == 0 ||
        _numAcceleratorsSystem <= 0 || ( _numFPGAThreads == 0 && !_idleCallback ) )
   {
      // The current configuration disables the FPGA support
      if ( nanos_needs_fpga_fun ) {
         warning0( " FPGA tasks were compiled and FPGA was disabled, execution could have " <<
                   "unexpected behavior and can even hang, check configuration parameters" );
      }
      _enableFPGA = false;
      _numAccelerators = 0;
      _numFPGAThreads = 0;
      _idleCallback = false;
   } else if ( _numAccelerators < 0 || _numAccelerators > _numAcceleratorsSystem ) {
      // The number of accelerators available in the system has to be used
      if ( _numAccelerators > _numAcceleratorsSystem ) {
         warning0( "The number of FPGA accelerators is larger than the accelerators in the system."
            << " Using " << _numAcceleratorsSystem << " accelerators." );

      }
      _numAccelerators = _numAcceleratorsSystem;
   }

   if ( _numFPGAThreads < 0 ) {
      //warning0( "Number of fpga threads cannot be negative. Using one thread per accelerator" );
      _numFPGAThreads = _numAccelerators;
   } else if ( _numFPGAThreads > _numAccelerators ) {
      warning0( "Number of FPGA helpers is greater than the number of FPGA accelerators. "
               << "Using one thread per accelerator" );
      _numFPGAThreads = _numAccelerators;
   }
   _idleSyncBurst = ( _idleSyncBurst < 0 ) ? _burst : _idleSyncBurst;

   if ( _enableFPGA && !_configFile->compare( "" ) ) {
      // Get the config file name using the executable filename
      // http://www.cplusplus.com/reference/string/string/find_last_of/
      std::string argv0 = std::string( OS::getArg(0) );

      // Look in the application binary folder
      *_configFile = argv0 + ".nanox.config";
      std::ifstream test( _configFile->c_str() );
      if ( !test.is_open() ) {
         // Look in the execution folder
         std::size_t found = argv0.find_last_of("/\\");
         if ( found == std::string::npos ) {
            // Something went wrong (argv0 does not contain any slash)
            fatal0( "FPGA support requires reading a '.nanox.config' file to initialize the accelerator types."
                     << " However, NX_FPGA_CONFIG_FILE was not defined and runtime was not able to build the"
                     << " filename based on the application binary." );
         }
         *_configFile = argv0.substr( found + 1 ) + ".nanox.config";
      }
   }

   // Generate the FPGA accelerators types mask
   if ( _enableFPGA ) {
      std::ifstream cnfFile( _configFile->c_str() );
      if ( cnfFile.is_open() ) {
         std::string line;
         std::getline( cnfFile, line ); // First line is the headers line (ignore it)
         while ( std::getline( cnfFile, line ) ) {
            // Each line contains [acc_id, num_instances, acc_name]
            std::istringstream iss( line );
            FPGADeviceType type;
            size_t count;
            std::string name;
            if ( !( iss >> type >> count >> name ) ) {
               fatal0( "Invalid line reading file " << *_configFile << ". Wrong line is:\t" << line );
            }
            _accTypesMap->insert( std::make_pair( type, count ) );
         }
         if ( _accTypesMap->empty() ) {
            warning0( "Configuration file '" << *_configFile << "' is empty (no accelerator types read)."
                      << " Assuming that all accelerators (" << _numAccelerators << ") are of type '0'" );
            _accTypesMap->insert( std::make_pair( FPGADeviceType( 0 ), _numAccelerators ) );
         }
      } else {
         warning0( "Cannot open file '" << *_configFile << "' to build the map of accelerators types."
                   << " Assuming that all accelerators (" << _numAccelerators << ") are of type '0'" );
         _accTypesMap->insert( std::make_pair( FPGADeviceType( 0 ), _numAccelerators ) );
      }
   }
}

void FPGAConfig::setFPGASystemCount ( int numFPGAs )
{
   _numAcceleratorsSystem = numFPGAs;
}

bool FPGAConfig::mayBeEnabled ()
{
   return ( _enableFPGA || nanos_needs_fpga_fun ) && !_forceDisableFPGA && _numAccelerators != 0;
}

} // namespace ext
} // namespace nanos
