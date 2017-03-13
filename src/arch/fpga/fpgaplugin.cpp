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
#include "eventdispatcher_decl.hpp"

#include "libxdma.h"

#define NUM_STRING_LEN  16

namespace nanos {
namespace ext {

class FPGAListener : public EventListener {
   private:
      FPGAThread* _fpgaThread;
   public:
     FPGAListener( FPGAThread* thread ) : _fpgaThread( thread ) {}
     ~FPGAListener() {}
     void callback( BaseThread* thread );
};

void FPGAListener::callback( BaseThread* thread )
{ 
   int maxPendingWD = FPGAConfig::getMaxPendingWD();
   int finishBurst = FPGAConfig::getFinishWDBurst();
   for (;;){
      //check if we have reached maximum pending WD
      //  finalize one (or some of them)
      //FPGAThread *myThread = (FPGAThread*)getMyThreadSafe();

      if ( _fpgaThread->getPendingWDs() > maxPendingWD ) {
          _fpgaThread->finishPendingWD( finishBurst );
      }

      if ( !thread->isRunning() ) break;
      //get next WD
      WD *wd = FPGAWorker::getFPGAWD( _fpgaThread );
      if ( wd ) {
         Scheduler::prePreOutlineWork(wd);
         if ( Scheduler::tryPreOutlineWork(wd) ) {
            _fpgaThread->preOutlineWorkDependent( *wd );
         }
         //TODO: may need to increment copies version number here
         if ( wd->isInputDataReady() ) {
            Scheduler::outlineWork( _fpgaThread, wd );
         } else {
            //do whatever is needed if input is not ready
            //wait or whatever, for instance, sync needed copies
         }
         //add to the list of pending WD
         wd->submitOutputCopies();
         _fpgaThread->addPendingWD( wd );

         //Scheduler::postOutlineWork( wd, false, myThread ); <--moved to fpga thread
      } else {
         break;
      }
   }
}

class FPGAPlugin : public ArchPlugin
{
   private:
      std::vector< FPGAProcessor* > *_fpgas;
      std::vector< FPGAThread* >    *_fpgaThreads;
      std::vector< FPGAListener* >   _fpgaListeners;
      std::vector< std::string >     _devNames;
      SMPProcessor                  *_core;
      SMPMultiThread                *_fpgaHelper;

   public:
      FPGAPlugin() : ArchPlugin( "FPGA PE Plugin", 1 ) {}

      void config( Config& cfg )
      {
         FPGAConfig::prepare( cfg );
      }

#ifdef NANOS_INSTRUMENTATION_ENABLED
      void registerDeviceInstrumentation( FPGAProcessor *fpga, int accNum ) {
          unsigned int id;
          //FIXME: assign proper IDs to deviceinstrumentation
          for (int i=0; i<accNum; i++) {
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
          }
      }
#endif

      /*!
       * \brief Initialize fpga device plugin.
       * Load config and initialize xilinx dma library
       */
      void init()
      {
         /*
          * Initialize dma library here if fpga device is not disabled
          * We must init the DMA lib before any operation using it is performed
          */
         int xdmaOpened = false;
         if ( !FPGAConfig::isDisabled() ) {
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
         FPGADD::init();
         _fpgas = NEW std::vector< FPGAProcessor* >( FPGAConfig::getFPGACount(),( FPGAProcessor* )NULL) ;
         _fpgaThreads = NEW std::vector< FPGAThread* >( FPGAConfig::getNumFPGAThreads(),( FPGAThread* )NULL) ;
         _core = NULL;
         _fpgaHelper = NULL;

         if ( !FPGAConfig::isDisabled() ) {
            //NOTE: Do it only when NANOS_INSTRUMENTATION_ENABLED?
            //Init the instrumentation
            int status = xdmaInitHWInstrumentation();
            if (status) {
               fatal0("Error initializing the instrumentation support in the DMA library. Returned status: " << status);
            }

            //Check the cache policy
            if ( sys.getRegionCachePolicyStr().compare( "fpga" ) != 0 ) {
               if ( sys.getRegionCachePolicyStr().compare( "" ) != 0 ) {
                  warning0( "Switching the cache-policy from '" << sys.getRegionCachePolicyStr() << "' to 'fpga'" );
               } else {
                  debug0( "Setting the cache-policy option to 'fpga'" );
               }
               sys.setRegionCachePolicyStr( "fpga" );
            }

            //Accelerator setup
            for ( unsigned int i=0; i < _fpgas->size(); i++) {
               std::stringstream name;
               name << "FPGA acc " << i;
               _devNames.push_back( name.str() );
               FPGADevice *device = NEW FPGADevice( _devNames.back().c_str() );
               memory_space_id_t memSpaceId = sys.addSeparateMemoryAddressSpace(
                     *device, true, 0 );
               SeparateMemoryAddressSpace &fpgaAddressSpace = sys.getSeparateMemory( memSpaceId );
               fpgaAddressSpace.setAcceleratorNumber( sys.getNewAcceleratorId() );
               fpgaAddressSpace.setNodeNumber( 0 ); //there is only 1 node on this machine

               FPGADD::addAccDevice( device );
               FPGAProcessor *fpga = NEW FPGAProcessor( device, memSpaceId );
               (*_fpgas)[i] = fpga;
#ifdef NANOS_INSTRUMENTATION_ENABLED
               //Register device in the instrumentation system
               registerDeviceInstrumentation( fpga,
                     _fpgas->size()/FPGAConfig::getNumFPGAThreads() );
#endif
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
         } else { //!FPGAConfig::isDisabled()
            //NOTE: Add a fake device to allow FPGADD instantiation
            FPGADD::addAccDevice( NULL );
            
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
         /*
          * After the plugin is unloaded, no more operations regarding the DMA
          * library nor the FPGA device will be performed so it's time to close the dma lib
          */
         if ( !FPGAConfig::isDisabled() ) { //cleanup only if we have initialized
            int status;
            debug0( "Xilinx close dma" );
            status = xdmaClose();
            if ( status ) {
               warning( "Error de-initializing xdma core library: " << status );
            }

            for (size_t i = 0; i < _fpgaListeners.size(); ++i) {
               delete _fpgaListeners[i];
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

      virtual void addPEs( std::map< unsigned int,  ProcessingElement*> &pes ) const {
          for ( std::vector<FPGAProcessor*>::const_iterator it = _fpgas->begin();
                  it != _fpgas->end(); it++ )
          {
              pes.insert( std::make_pair( (*it)->getId(), *it) );
          }
      }

      virtual void addDevices( DeviceList &devices ) const {
         if ( !_fpgas->empty() ) {
            //Insert device type.
            //Any position in _fpgas will work as we only need the device type
            std::vector<const Device*> const &pe_archs = ( *_fpgas->begin() )->getDeviceTypes();
            for ( std::vector<const Device *>::const_iterator it = pe_archs.begin();
                  it != pe_archs.end(); it++ ) {
               devices.insert( *it );
            }
         }
      }

      virtual void startSupportThreads () {
         if ( !_core || _fpgas->empty() ) return;
         _fpgaHelper = dynamic_cast< ext::SMPMultiThread * >(
            &_core->startMultiWorker( _fpgas->size(), (ProcessingElement **) &(*_fpgas)[0],
            ( DD::work_fct )FPGAWorker::FPGAWorkerLoop )
         );
      }

      virtual void startWorkerThreads( std::map<unsigned int, BaseThread*> &workers ) {
         if ( !_fpgaHelper ) return;
         for ( unsigned int i=0; i< _fpgaHelper->getNumThreads(); i++) {
            BaseThread *thd = _fpgaHelper->getThreadVector()[ i ];
            workers.insert( std::make_pair( thd->getId(), thd ) );

            // Register Event Listener
            FPGAListener* l = new FPGAListener( (FPGAThread*)(thd) );
            _fpgaListeners.push_back( l );
            sys.getEventDispatcher().addListenerAtIdle( *l );
         }
         //Push multithread into the team to let ir steam tasks from other smp threads
         workers.insert( std::make_pair( _fpgaHelper->getId(), _fpgaHelper ) );
      }

      virtual ProcessingElement * createPE( unsigned id , unsigned uid) {
         return NULL;
      }
};

}
}

DECLARE_PLUGIN("arch-fpga",nanos::ext::FPGAPlugin);
