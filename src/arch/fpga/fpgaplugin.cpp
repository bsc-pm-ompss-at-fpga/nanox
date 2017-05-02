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

#ifdef NANOS_INSTRUMENTATION_ENABLED
      void registerDeviceInstrumentation( FPGAProcessor *fpga, int i ) {
          unsigned int id;
          //FIXME: assign proper IDs to deviceinstrumentation
          //for (int i=0; i<accNum; i++) {
             char devNum[NUM_STRING_LEN];
             sprintf(devNum, "%d", i);
             id = sys.getNumInstrumentAccelerators();
             FPGAInstrumentation *instr = new FPGAInstrumentation(
                   std::string( "FPGA accelerator" ) + devNum );
             instr->setId( id );
             sys.addDeviceInstrumentation( instr );

             id = sys.getNumInstrumentAccelerators();
             FPGAInstrumentation *dmaInInstr = new FPGAInstrumentation(
                   std::string( "DMA in" ) + devNum );
             dmaInInstr->setId( id );
             sys.addDeviceInstrumentation( dmaInInstr );

             id = sys.getNumInstrumentAccelerators();
             FPGAInstrumentation *dmaOutInstr = new FPGAInstrumentation(
                   std::string( "DMA out" ) + devNum );
             dmaOutInstr->setId( id );
             sys.addDeviceInstrumentation( dmaOutInstr );

             id = sys.getNumInstrumentAccelerators();
             FPGAInstrumentation *submitInstr = new FPGAInstrumentation(
                   std::string( "DMA submit" ) + devNum );
             submitInstr->setId( id );
             sys.addDeviceInstrumentation( submitInstr );

             instr->init();
             //sys.getInstrumentation()->registerInstrumentDevice( instr );
             fpga->setDeviceInstrumentation( instr );
             fpga->setDmaInstrumentation( dmaInInstr, dmaOutInstr );
             fpga->setSubmitInstrumentation( submitInstr );
          //}
      }
#endif

      /*!
       * \brief Initialize fpga device plugin.
       * Load config and initialize xilinx dma library
       */
      void init()
      {
         /*
          * Initialize dma library here if fpga support may be required
          * We must init the DMA lib before any operation using it is performed
          */
         int xdmaOpened = false;
         if ( FPGAConfig::mayBeEnabled() ) {
            debug0( "xilinx dma initialization" );
            //Instrumentation has not been initialized yet so we cannot trace things yet
            int status = xdmaOpen();
            //Abort if dma library failed to initialize
            //Otherwise this will cause problems (segfaults/hangs) later on the execution
            if (status)
               fatal0( "Error initializing DMA library: Returned status: " << status );
            xdmaOpened = true;

            //Check the number of accelerators in the system
            int numAccel;
            status = xdmaGetNumDevices( &numAccel );
            if ( status != XDMA_SUCCESS ) {
               warning0( "Error getting the number of accelerators in the system: Returned status: " << status );
            } else {
               FPGAConfig::setFPGASystemCount( numAccel );
            }
         }

         //NOTE: Apply the config now because before FPGAConfig doesn't know the number of accelerators in the system
         FPGAConfig::apply();
         FPGADD::init( &_fpgaDevices );
         _fpgas = NEW std::vector< FPGAProcessor* >( FPGAConfig::getFPGACount(),( FPGAProcessor* )NULL) ;
         _fpgaThreads = NEW std::vector< FPGAThread* >( FPGAConfig::getNumFPGAThreads(),( FPGAThread* )NULL) ;
         _core = NULL;
         _fpgaHelper = NULL;

         if ( FPGAConfig::isEnabled() ) {
            //NOTE: Do it only when NANOS_INSTRUMENTATION_ENABLED?
            //Init the instrumentation
            int status = xdmaInitHWInstrumentation();
            if (status) {
               fatal0("Error initializing the instrumentation support in the DMA library. Returned status: " << status);
            }

            // Create the FPGAPinnedAllocator and set the global shared variable that points to it
            fpgaAllocator = NEW FPGAPinnedAllocator( FPGAConfig::getAllocatorPoolSize()*1024*1024 /* MB -> bytes */ );

            memory_space_id_t memSpaceId = -1;
            unsigned int fpgasCount = 0;
            for ( FPGATypesMap::const_iterator typesIter = FPGAConfig::getAccTypesMap().begin();
                  typesIter != FPGAConfig::getAccTypesMap().end(); ++typesIter )
            {
               // At least one iteration will be done because FPGAConfig ensures that the map is not empty
               FPGADeviceType fpgaType = ( *typesIter ).first;
               size_t numAccels = ( *typesIter ).second;

               // Create the FPGA device
               FPGADevice * fpgaDevice = NEW FPGADevice( fpgaType );
               _fpgaDevices.insert( std::make_pair(fpgaType, fpgaDevice) );
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

               // Create the FPGA accelerators for the current type
               for ( size_t i = 0; i < numAccels && fpgasCount < _fpgas->size(); ++i ) {
                  debug0( "New FPGAProcessor created with id: " << fpgasCount << ", memSpaceId: " <<
                          memSpaceId << ", fpgaType: " << fpgaType );

                  FPGAProcessor *fpga = NEW FPGAProcessor( fpgasCount, memSpaceId, fpgaDevice );
                  (*_fpgas)[fpgasCount] = fpga;
#ifdef NANOS_INSTRUMENTATION_ENABLED
                  //Register device in the instrumentation system
                  registerDeviceInstrumentation( fpga, fpgasCount );
#endif
                  ++fpgasCount;
               }
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
            if ( xdmaOpened ) {
               int status;
               debug0( "Xilinx close dma" );
               status = xdmaClose();
               if ( status ) {
                  warning( "Error de-initializing xdma core library: " << status );
               }
            }
         }

      }
      /*!
       * \brief Finalize plugin and close dma library.
       */
      void finalize() {
         if ( FPGAConfig::isEnabled() ) { //cleanup only if we have initialized
            int status;

            // Run FPGAProcessor cleanup. Maybe it won't run any thread and cleanup must be run in any case
            for ( unsigned int i = 0; i < _fpgas->size(); i++ ) {
               (*_fpgas)[i]->cleanUp();
            }

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

            /*
             * After the plugin is unloaded, no more operations regarding the DMA
             * library nor the FPGA device will be performed so it's time to close the dma lib
             */
            debug0( "Xilinx close dma" );
            status = xdmaClose();
            if ( status ) {
               warning( "Error de-initializing xdma core library: " << status );
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
            /*
             * Initialize device. Maybe it won't run any thread and must be initialized in any case
             * NOTE: This cannot be done during the FPGAPlugin init call because the instrumentation
             *       is not available
             */
            ( *it )->init();
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
