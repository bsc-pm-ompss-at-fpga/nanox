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

#include "libxdma.h"
#include "libxtasks.h"

#define NUM_STRING_LEN  16

namespace nanos {
namespace ext {

class FPGAPlugin : public ArchPlugin
{
   private:
      std::vector< FPGAProcessor* > *_fpgas;
      std::vector< FPGAThread* >    *_fpgaThreads;
      std::vector< FPGAListener* >   _fpgaListeners;
      FPGADeviceMap                  _fpgaDevices;
      SMPProcessor                  *_core;
      SMPMultiThread                *_fpgaHelper;

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
         //Check if the plugin has to be initialized
         if ( FPGAConfig::isDisabled() ) return;
         debug0( "FPGA Arch support may be enabled" );

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
         FPGADD::init( &_fpgaDevices );
         _fpgas = NEW std::vector< FPGAProcessor* >();
         _fpgas->reserve( FPGAConfig::getFPGACount() );
         _fpgaThreads = NEW std::vector< FPGAThread* >( FPGAConfig::getNumFPGAThreads(),( FPGAThread* )NULL) ;
         _core = NULL;
         _fpgaHelper = NULL;

         if ( FPGAConfig::isEnabled() ) {
            debug0( "FPGA Arch support enabled. Initializing structures..." );

            //Init the DMA lib before any operation using it is performed
            xdma_status sxd = xdmaOpen();
            if ( sxd != XDMA_SUCCESS ) {
               //Abort if dma library failed to initialize
               fatal0( "Error initializing DMA library, returned status: " << sxd );
            }

            //Init the instrumentation
            sxd = xdmaInitHWInstrumentation();
            if ( sxd != XDMA_SUCCESS ) {
               //NOTE: If the HW instrumentation initialization fails the execution must end.
               //      Current accelerators always generate the HW instrumentation information
               sxd = xdmaClose();
               if ( sxd != XDMA_SUCCESS ) {
                  warning0( "Error uninitializing xdma core library" );
               }
               fatal0( "Error initializing the instrumentation support in the DMA library." );
            }

            //Create the FPGAPinnedAllocator and set the global shared variable that points to it
            fpgaAllocator = NEW FPGAPinnedAllocator( FPGAConfig::getAllocatorPoolSize() );

            //Get the accelerators information array
            size_t count, maxFpgasCount = FPGAConfig::getFPGACount();
            xtasks_acc_handle accels[maxFpgasCount];
            sxt = xtasksGetAccs( maxFpgasCount, &accels[0], &count );
            ensure( count == maxFpgasCount, "Cannot retrieve accelerators information" );
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
               _fpgas->push_back( fpga );
            }

            if ( _fpgaThreads->size() > 0 ) {
               //Assuming 1 FPGA helper thread
               //TODO: Allow any number of fpga threads
               _core = sys.getSMPPlugin()->getLastFreeSMPProcessorAndReserve();
               if ( _core ) {
                  _core->setNumFutureThreads( 1 );
               } else {
                  _core = sys.getSMPPlugin()->getFirstSMPProcessor();
                  _core->setNumFutureThreads( _core->getNumFutureThreads() + 1 );
                  NANOS_INSTRUMENT( sys.getInstrumentation()->incrementMaxThreads(); );
               }
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
            int status;

            // Delete FPGADevices
            FPGADD::fini();
            for ( FPGADeviceMap::const_iterator it = _fpgaDevices.begin();
               it != _fpgaDevices.end(); ++it )
            {
               delete it->second;
            }

            delete fpgaAllocator;
            fpgaAllocator = NULL;

            for (size_t i = 0; i < _fpgaListeners.size(); ++i) {
               delete _fpgaListeners[i];
            }

            //NOTE: Do it only when NANOS_INSTRUMENTATION_ENABLED?
            //Finalize the HW instrumentation
            status = xdmaFiniHWInstrumentation();
            if (status) {
               warning("Error uninitializing the instrumentation support in the DMA library. Returned status: " << status);
            }

            /*
             * After the plugin is unloaded, no more operations regarding the DMA
             * library nor the FPGA device will be performed so it's time to close the dma lib
             */
            status = xdmaClose();
            if ( status ) {
               warning( "Error uninitializing xdma core library: " << status );
            }

            //Finalize the xTasks library
            xtasks_stat xst = xtasksFini();
            if ( xst != XTASKS_SUCCESS ) {
               warning( "Error uninitializing xTasks library, returned status: " << status );
            }
         }
      }

      virtual unsigned getNumHelperPEs() const {
         return ( _fpgaHelper != NULL );
      }

      virtual unsigned getNumPEs() const {
         return _fpgas->size();
      }

      virtual unsigned getNumThreads() const {
         return getNumWorkers() /* + getNumHelperThreads() */;
      }

      virtual unsigned getNumWorkers() const {
         return _fpgaThreads->size();
      }

      virtual void addPEs( PEMap &pes ) const {
         for ( std::vector<FPGAProcessor*>::const_iterator it = _fpgas->begin();
               it != _fpgas->end(); it++ )
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
         if ( !FPGAConfig::getIdleCallbackEnabled() ) return;
         for ( std::vector<FPGAProcessor*>::const_iterator it = _fpgas->begin();
               it != _fpgas->end(); it++ )
         {
            // Register Event Listener
            FPGAListener* l = new FPGAListener( *it );
            _fpgaListeners.push_back( l );
            sys.getEventDispatcher().addListenerAtIdle( *l );
         }
      }

      virtual void startWorkerThreads( std::map<unsigned int, BaseThread*> &workers ) {
         if ( !_core ) return;
         ensure( !_fpgas->empty(), "Starting one FPGA Support Thread but there are not accelerators" );

         // Starting the SMP (multi)Thread
         _fpgaHelper = dynamic_cast< ext::SMPMultiThread * >(
            &_core->startMultiWorker( _fpgas->size(), (ProcessingElement **) &(*_fpgas)[0],
            ( DD::work_fct )FPGAWorker::FPGAWorkerLoop )
         );

         // Register each sub-thread of Multithread
         for ( unsigned int i = 0; i< _fpgaHelper->getNumThreads(); i++) {
            BaseThread *thd = _fpgaHelper->getThreadVector()[ i ];
            workers.insert( std::make_pair( thd->getId(), thd ) );
         }
         //Push multithread into the team to let it steam tasks from other smp threads
         workers.insert( std::make_pair( _fpgaHelper->getId(), _fpgaHelper ) );
      }

      virtual ProcessingElement * createPE( unsigned id , unsigned uid ) {
         return NULL;
      }
};

}
}

DECLARE_PLUGIN("arch-fpga",nanos::ext::FPGAPlugin);
