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

#include "plugin.hpp"
#include "archplugin.hpp"
#include "fpgaconfig.hpp"
#include "system_decl.hpp"
#include "fpgaprocessor.hpp"
#include "fpgathread.hpp"
#include "fpgadd.hpp"
#include "smpprocessor.hpp"
#include "instrumentationmodule_decl.hpp"
#include "fpgainstrumentation.hpp"
#include "fpgaworker.hpp"
#include "fpgalistener.hpp"
#include "fpgapinnedallocator.hpp"
#include "libxtasks_wrapper.hpp"

namespace nanos {
namespace ext {

class FPGAPlugin : public ArchPlugin
{
   private:
      std::vector< SMPMultiThread* > _helperThreads;
      std::vector< SMPProcessor* >   _helperCores;
      std::vector< FPGAListener* >   _fpgaListeners;
      FPGADeviceMap                  _fpgaDevices;
      std::string                    _executionSummary;
      FPGACreateWDListener           _createWDListener;
      FPGAWorker::FPGARegisteredTasksMap _registeredTasks;
#ifdef NANOS_INSTRUMENTATION_ENABLED
      FPGAInstrumentationListener    _instrumentationListener;
#endif //NANOS_INSTRUMENTATION_ENABLED

   public:
      FPGAPlugin() : ArchPlugin( "FPGA PE Plugin", 1 ) {}

      void config( Config& cfg )
      {
         FPGAConfig::prepare( cfg );
      }

      /*!
       * \brief Initialize fpga device plugin.
       * Load config and initialize xilinx dma library
       */
      void init()
      {
         //Forward these initializations as they have to be done regardless fpga support is enabled.
         FPGADD::init( &_fpgaDevices );
         fpgaPEs = NEW std::vector< FPGAProcessor * >();
         FPGAWorker::initRegisteredTasksMap( &_registeredTasks, &_createWDListener );

         //Check if the plugin has to be initialized
         if ( FPGAConfig::isDisabled() ) {
            debug0( "FPGA Arch support not needed or disabled. Skipping initialization" );
            return;
         }

         //Init the xTasks library
         xtasks_stat sxt = xtasksInit();
         if ( sxt != XTASKS_SUCCESS ) {
            fatal0( "Error initializing xTasks library, returned status: " << sxt );
         }

         //Check the number of accelerators in the system
         size_t numAccel;
         sxt = xtasksGetNumAccs( &numAccel );
         if ( sxt != XTASKS_SUCCESS ) {
            xtasksFini();
            fatal0( "Error getting the number of accelerators in the system, returned status: " << sxt );
         }
         FPGAConfig::setFPGASystemCount( numAccel );
         //NOTE: Apply the config now because before FPGAConfig doesn't know the number of accelerators in the system
         FPGAConfig::apply();

         //Initialize some variables
         fpgaPEs->reserve( FPGAConfig::getFPGACount() );
         _helperThreads.reserve( FPGAConfig::getNumFPGAThreads() );
         _helperCores.reserve( FPGAConfig::getNumFPGAThreads() );

         if ( FPGAConfig::isEnabled() ) {
            debug0( "FPGA Arch support required. Initializing structures..." );

#if NANOS_INSTRUMENTATION_ENABLED
            //Init the instrumentation
            sxt = xtasksInitHWIns( FPGAConfig::getNumInstrEvents() );
            if ( sxt != XTASKS_SUCCESS ) {
               FPGAConfig::forceDisableInstr();
               warning0( " Error initializing the FPGA instrumentation support (status: " << sxt << ")." <<
                  ( sxt == XTASKS_ENOENTRY ? " Check if xdma_instr device exist in the system." : "" ) <<
                  ( sxt == XTASKS_EFILE ? " Current user cannot access xdma_instr device." : "" )
               );
               warning0( " Disabling all events generated using the FPGA instrumentation timer." );
            }
#endif //NANOS_INSTRUMENTATION_ENABLED

            //Create the FPGAPinnedAllocator and set the global shared variable that points to it
            fpgaAllocator = NEW FPGAPinnedAllocator();

            //Get the accelerators information array
            size_t count, maxFpgasCount = FPGAConfig::getFPGACount();
            xtasks_acc_handle accels[maxFpgasCount];
            sxt = xtasksGetAccs( maxFpgasCount, &accels[0], &count );
            ensure( count == maxFpgasCount, " Cannot retrieve accelerators information" );
            if ( sxt != XTASKS_SUCCESS ) {
               fatal0( "Error getting accelerators information, returned status" << sxt );
            }

            //Create the FPGAProcessors and FPGADevices
            memory_space_id_t memSpaceId = -1;
            FPGADevice * fpgaDevice = NULL;
            for ( size_t fpgasCount = 0; fpgasCount < maxFpgasCount; ++fpgasCount ) {
               FPGAProcessorInfo info( accels[fpgasCount] );
               FPGADeviceType fpgaType = info.getType();

               //Check the accelerator type
               if ( fpgaDevice == NULL || fpgaType != fpgaDevice->getFPGAType() ) {
                  fpgaDevice = NEW FPGADevice( fpgaType );
                  _fpgaDevices.insert( std::make_pair( fpgaType, fpgaDevice ) );

                  if ( fpgasCount == 0 ) {
                     /* NOTE: In order to create the FPGAProcessor the memSpaceId is needed and this one
                      *       wants one Device to performe the copies operations.
                      *       However, the FPGADevice will delegate the copies, allocations, etc. to the
                      *       FPGAPinnedAllocator. So, the used FPGADevice doesn't matter.
                      */
                     memSpaceId = sys.addSeparateMemoryAddressSpace( *fpgaDevice, true, 0 );
                     SeparateMemoryAddressSpace &fpgaAddressSpace = sys.getSeparateMemory( memSpaceId );
                     fpgaAddressSpace.setAcceleratorNumber( sys.getNewAcceleratorId() );
                     fpgaAddressSpace.setNodeNumber( 0 ); //there is only 1 node on this machine
                     fpgaAddressSpace.setSpecificData( fpgaAllocator );
                  }
               }

               debug0( "New FPGAProcessor created with id: " << info.getId() << ", memSpaceId: " <<
                       memSpaceId << ", fpgaType: " << fpgaType );
               FPGAProcessor *fpga = NEW FPGAProcessor( info, memSpaceId, fpgaDevice );
               fpgaPEs->push_back( fpga );
            }

            //Reserve some SMP cores to run the helper threads
            for ( int i = 0; i < FPGAConfig::getNumFPGAThreads(); ++i ) {
               SMPProcessor * core = sys.getSMPPlugin()->getLastFreeSMPProcessorAndReserve();
               if ( core != NULL ) {
                  core->setNumFutureThreads( 1 );
               } else {
                  core = sys.getSMPPlugin()->getLastSMPProcessor();
                  core->setNumFutureThreads( core->getNumFutureThreads() + 1 );
               }
               _helperCores.push_back( core );
            }
         } else { //!FPGAConfig::isEnabled()
            sxt = xtasksFini();
            if ( sxt != XTASKS_SUCCESS ) {
               warning0( "Error uninitializing xTasks library, returned status: " << sxt );
            }
         }
      }
      /*!
       * \brief Finalize plugin and close dma library.
       */
      void finalize() {
         if ( FPGAConfig::isEnabled() ) { //cleanup only if we have initialized
            xtasks_stat sxt;

            // Generate the execution summary before deleting the information
            //NOTE: It will be later retrieved using the getExecutionSummary
            generateExecutionSummary();

            // Join and delete FPGA Helper threads
            //NOTE: As they are in the workers list, they are deleted during the System::finish()

            // Delete FPGA Processors
            //NOTE: As they are in the PEs map, they are deleted during the System::finish()
            delete fpgaPEs;
            fpgaPEs = NULL;

            // Delete FPGADevices
            FPGAWorker::finiRegisteredTasksMap();
            FPGADD::fini();
            for ( FPGADeviceMap::const_iterator it = _fpgaDevices.begin();
               it != _fpgaDevices.end(); ++it )
            {
               delete it->second;
            }

            // Delete FPGA Allocator
            delete fpgaAllocator;
            fpgaAllocator = NULL;

            // Delete FPGA Idle Callbacks
            for (size_t i = 0; i < _fpgaListeners.size(); ++i) {
               delete _fpgaListeners[i];
            }

#if NANOS_INSTRUMENTATION_ENABLED
            if ( !FPGAConfig::isInstrDisabled() ) {
               //Finalize the HW instrumentation
               sxt = xtasksFiniHWIns();
               if (sxt != XTASKS_SUCCESS) {
                  warning( " Error uninitializing the instrumentation support in the xTasks library" <<
                     " (status: " << sxt << ")" );
               }
            }
#endif //NANOS_INSTRUMENTATION_ENABLED

            //Finalize the xTasks library
            sxt = xtasksFini();
            if ( sxt != XTASKS_SUCCESS ) {
               warning( "Error uninitializing xTasks library, returned status: " << sxt );
            }
         }
      }

      virtual unsigned getNumHelperPEs() const {
         return _helperCores.size();
      }

      virtual unsigned getNumPEs() const {
         return fpgaPEs->size();
      }

      virtual unsigned getNumThreads() const {
         return getNumWorkers() /* + getNumHelperThreads() */;
      }

      virtual unsigned getNumWorkers() const {
         return _helperThreads.size();
      }

      virtual void addPEs( PEMap &pes ) const {
         for ( std::vector<FPGAProcessor*>::const_iterator it = fpgaPEs->begin();
               it != fpgaPEs->end(); it++ )
         {
            pes.insert( std::make_pair( (*it)->getId(), *it) );
         }
      }

      virtual void addDevices( DeviceList &devices ) const {
         for ( FPGADeviceMap::const_iterator it = _fpgaDevices.begin();
               it != _fpgaDevices.end(); ++it )
         {
            devices.insert( it->second );
         }
      }

      virtual void startSupportThreads () {
         //Register the regular callback
         if ( FPGAConfig::getIdleCallbackEnabled() ) {
            for ( std::vector<FPGAProcessor*>::const_iterator it = fpgaPEs->begin();
                  it != fpgaPEs->end(); it++ )
            {
               FPGAListener* l = new FPGAListener( *it );
               _fpgaListeners.push_back( l );
               sys.getEventDispatcher().addListenerAtIdle( *l );
            }
         }

         //Register the creation callback
         if ( nanos::ext::FPGAConfig::getIdleCreateCallbackEnabled() ) {
            sys.getEventDispatcher().addListenerAtIdle( _createWDListener );
            nanos::ext::FPGAConfig::setIdleCreateCallbackRegistered();
         }

#ifdef NANOS_INSTRUMENTATION_ENABLED
         //Register the instrumentation callback
         if ( nanos::ext::FPGAConfig::getInstrumentationCallbackEnabled() ) {
            _instrumentationListener.setFPGAPEsVector( fpgaPEs );
            sys.getEventDispatcher().addListenerAtIdle( _instrumentationListener );
         }
#endif //NANOS_INSTRUMENTATION_ENABLED
      }

      virtual void startWorkerThreads( std::map<unsigned int, BaseThread*> &workers ) {
         for ( std::vector<SMPProcessor*>::const_iterator it = _helperCores.begin();
               it != _helperCores.end(); it++ )
         {
            // Starting the SMP (multi)Thread
            //NOTE: Assuming that all threads send to all accelerators
            SMPMultiThread * fpgaHelper = dynamic_cast< ext::SMPMultiThread * >(
               &(*it)->startMultiWorker( fpgaPEs->size(), (ProcessingElement **) &fpgaPEs->at(0),
               ( DD::work_fct )FPGAWorker::FPGAWorkerLoop )
            );
            debug0( "New FPGA Helper Thread created with id: " << fpgaHelper->getId() <<
               ", in SMP processor: " << ( *it )->getId() );
            _helperThreads.push_back( fpgaHelper );

            //Push multithread into the team to let it steam tasks from other smp threads
            //When the parent thread enters in a team, all sub-threads also enter the team
            workers.insert( std::make_pair( fpgaHelper->getId(), fpgaHelper ) );
         }
      }

      virtual ProcessingElement * createPE( unsigned id , unsigned uid ) {
         return NULL;
      }

      void generateExecutionSummary() {
#if defined(NANOS_DEBUG_ENABLED) || defined(NANOS_INSTRUMENTATION_ENABLED)
         for ( std::vector<FPGAProcessor*>::const_iterator it = fpgaPEs->begin();
               it != fpgaPEs->end(); it++ )
         {
            FPGAProcessor* f = *it;
            FPGAProcessorInfo info = f->getFPGAProcessorInfo();
            _executionSummary += "=== FPGA " + toString( info.getId() ) + " [type: " +  toString( info.getType() ) + "][freq: ";
            _executionSummary += toString( info.getFreq() ) + "] executed " + toString( f->getNumTasks() ) + " tasks\n";
         }
#endif
      }

      virtual std::string getExecutionSummary() const {
         return _executionSummary;
      }
};

}
}

DECLARE_PLUGIN("arch-fpga",nanos::ext::FPGAPlugin);
