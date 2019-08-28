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
#include "system.hpp"
#include "gasnetapi_decl.hpp"
#include "clusterplugin_decl.hpp"
#include "clusternode_decl.hpp"
#include "remoteworkdescriptor_decl.hpp"
#include "basethread.hpp"
#include "smpprocessor.hpp"
#include "clusterthread_decl.hpp"
#include "clusterconfig.hpp"

#ifdef OpenCL_DEV
#include "opencldd.hpp"
#endif
#ifdef GPU_DEV
#include "gpudd.hpp"
#endif
#ifdef FPGA_DEV
#include "fpgadd.hpp"
#endif

namespace nanos {
namespace ext {

ClusterPlugin::ClusterPlugin() : ArchPlugin( "Cluster PE Plugin", 1 ),
   _gasnetApi( NEW GASNetAPI() ), _numPinnedSegments( 0 ), _pinnedSegmentAddrList( NULL ),
   _pinnedSegmentLenList( NULL ), _extraPEsCount( 0 ), _conduit(""),
   _remoteNodes( NULL ), _cpus(), _clusterThreads(), _clusterListener() {
}

void ClusterPlugin::config( Config& cfg )
{
   ClusterConfig::prepare( cfg );
}

void ClusterPlugin::init()
{
   ClusterConfig::apply();

   _gasnetApi->initialize( sys.getNetwork() );
   //sys.getNetwork()->setAPI(_gasnetApi);
   _gasnetApi->setGASNetSegmentSize( ClusterConfig::getGasnetSegmentSize() );
   _gasnetApi->setUnalignedNodeMemory( ClusterConfig::getUnaligMemEnabled() );
   sys.getNetwork()->initialize( _gasnetApi );

   unsigned int nodes = _gasnetApi->getNumNodes();

   if ( nodes > 1 ) {
      if ( _gasnetApi->getNodeNum() == 0 ) {
         void *segmentAddr[ nodes ];
         sys.getNetwork()->mallocSlaves( &segmentAddr[ 1 ], ClusterConfig::getNodeMem() );
         segmentAddr[ 0 ] = NULL;

         ClusterNode::ClusterSupportedArchMap supported_archs;
         supported_archs[0] = &getSMPDevice();

         #ifdef GPU_DEV
         supported_archs[1] = &GPU;
         #endif
         #ifdef OpenCL_DEV
         supported_archs[2] = &OpenCLDev;
         #endif
         #ifdef FPGA_DEV
         unsigned int cnt = 3;
         for (FPGADeviceMap::iterator it = FPGADD::getDevicesMapBegin();
              it != FPGADD::getDevicesMapEnd(); ++it) {
            supported_archs[cnt] = it->second;
            ++cnt;
         }
         ClusterConfig::setMaxClusterArchId( cnt - 1 ); //< Update the max cluster arch id
         #endif

         const Device * supported_archs_array[supported_archs.size()];
         unsigned int arch_idx = 0;
         for ( ClusterNode::ClusterSupportedArchMap::const_iterator it = supported_archs.begin();
               it != supported_archs.end(); it++ ) {
            supported_archs_array[arch_idx] = it->second;
            arch_idx += 1;
         }

         _remoteNodes = NEW std::vector<nanos::ext::ClusterNode *>(nodes - 1, (nanos::ext::ClusterNode *) NULL);
         unsigned int node_index = 0;
         for ( unsigned int nodeC = 0; nodeC < nodes; nodeC++ ) {
            if ( nodeC != _gasnetApi->getNodeNum() ) {
               memory_space_id_t id = sys.addSeparateMemoryAddressSpace( ext::Cluster, !( ClusterConfig::getAllocFitEnabled() ), 0 );
               SeparateMemoryAddressSpace &nodeMemory = sys.getSeparateMemory( id );
               nodeMemory.setSpecificData( NEW SimpleAllocator( ( uintptr_t ) segmentAddr[ nodeC ], ClusterConfig::getNodeMem() ) );
               nodeMemory.setNodeNumber( nodeC );
               nanos::ext::ClusterNode *node = new nanos::ext::ClusterNode( nodeC, id, supported_archs, supported_archs_array );
               (*_remoteNodes)[ node_index ] = node;
               node_index += 1;
            }
         }
      }

      for ( unsigned int i = 0; i < ClusterConfig::getNumWorkersInNode( _gasnetApi->getNodeNum() ); ++i ) {
         ext::SMPProcessor *cpu = NULL;
         if ( ClusterConfig::getBindingStart() != -1 ) {
            //User wants the cluster thread to run in a specific CPU
            int cpuId = ( ClusterConfig::getBindingStart() + ClusterConfig::getBindingStride()*i )%sys.getSMPPlugin()->getCpuCount();
            cpu = sys.getSMPPlugin()->getSMPProcessorById( cpuId );
            if ( cpu->isReserved() && !ClusterConfig::getSharedWorkerPeEnabled() ) {
               fatal0( "Requested CPU for cluster thread is already reserved. Try use --cluster-allow-shared-thread or --binding-start" );
            }
         } else {
            cpu = sys.getSMPPlugin()->getLastFreeSMPProcessor();
            if ( cpu == NULL && ClusterConfig::getSharedWorkerPeEnabled() ) {
               //There is not a free CPU for cluster worker but it can run on a shared PE
               cpu = sys.getSMPPlugin()->getLastSMPProcessor();
               ensure0( cpu != NULL, "Unable to get a cpu to run the cluster thread." );
            } else if ( cpu == NULL ) {
               fatal0( "Unable to get a cpu to run the cluster thread. Try using --cluster-allow-shared-thread" );
            }
         }

         if ( !cpu->isReserved() ) {
            //The cpu is free and will be used to exclusively run the cluster worker
            cpu->reserve();
         }
         cpu->setNumFutureThreads( cpu->getNumFutureThreads() + 1 /* Cluster worker */ );
         _cpus.push_back( cpu );
      }

      //Register the EventListener in the EventDispatcher for atIdle events
      sys.getEventDispatcher().addListenerAtIdle( _clusterListener );
   }
}

//void ClusterPlugin::addPinnedSegments( unsigned int numSegments, void **segmentAddr, std::size_t *segmentSize ) {
//   unsigned int idx;
//   _numPinnedSegments = numSegments;
//   _pinnedSegmentAddrList = new void *[ numSegments ];
//   _pinnedSegmentLenList = new std::size_t[ numSegments ];
//
//   for ( idx = 0; idx < numSegments; idx += 1)
//   {
//      _pinnedSegmentAddrList[ idx ] = segmentAddr[ idx ];
//      _pinnedSegmentLenList[ idx ] = segmentSize[ idx ];
//   }
//}

// void * ClusterPlugin::getPinnedSegmentAddr( unsigned int idx ) const {
//    return _pinnedSegmentAddrList[ idx ];
// }
//
// std::size_t ClusterPlugin::getPinnedSegmentLen( unsigned int idx ) const {
//    return _pinnedSegmentLenList[ idx ];
// }

RemoteWorkDescriptor * ClusterPlugin::getRemoteWorkDescriptor( unsigned int nodeId, int archId ) {
   //NOTE Get the SMP core of one cluster worker or just one random SMP core. Reuse always the same core
   static ext::SMPProcessor *cpu = _cpus.empty() ? sys.getSMPPlugin()->getLastSMPProcessor() : _cpus[0];
   RemoteWorkDescriptor *rwd = NEW RemoteWorkDescriptor( nodeId );
   rwd->_mcontrol.preInit();
   rwd->_mcontrol.initialize( *cpu );
   return rwd;
}

ProcessingElement * ClusterPlugin::createPE( unsigned id, unsigned uid ){
   return NULL;
}

unsigned ClusterPlugin::getNumThreads() const {
   return 1;
}

void ClusterPlugin::startSupportThreads() {
   for ( unsigned int i = 0; i < _cpus.size(); ++i ) {
      ext::SMPProcessor *cpu = _cpus[i];
      if ( _gasnetApi->getNodeNum() == 0 ) {
         //Master node
         _clusterThreads.push_back( dynamic_cast<ext::SMPMultiThread *>( &cpu->startMultiWorker(
            _gasnetApi->getNumNodes() - 1,
            ( ProcessingElement ** ) &( *_remoteNodes )[0],
            ( DD::work_fct ) ClusterThread::workerClusterLoop
         ) ) );
      } else {
         //Slave nodes
         _clusterThreads.push_back( dynamic_cast<ext::SMPMultiThread *>( &cpu->startMultiWorker(
            0, NULL, ( DD::work_fct )ClusterThread::workerClusterLoop )
         ) );
         if ( sys.getPMInterface().getInternalDataSize() > 0 ) {
            _clusterThreads[i]->getThreadWD().setInternalData(
               NEW char[sys.getPMInterface().getInternalDataSize()]
            );
         }
      }
      debug0( "New Cluster Helper Thread created with id: " << _clusterThreads[i]->getId()
         << ", in SMP processor: " << cpu->getId() );
   }

   //NOTE The following code is executed here to ensure that the accelerators are already initialized.
   if ( _gasnetApi->getNodeNum() > 0 ) {
      //Do it only in the slave nodes
      if ( sys.getNumAccelerators() > 0 ) {
         sys.getNetwork()->enableCheckingForDataInOtherAddressSpaces();
      }

      int num_devices = ClusterConfig::getMaxClusterArchId() + 1;
      _gasnetApi->allocArchRWDs( 1, num_devices );
      for ( int j = 0; j < num_devices; j++ ) {
         _gasnetApi->_rwgs[0][j] = getRemoteWorkDescriptor( 0, j );
      }
   }
}

void ClusterPlugin::startWorkerThreads( std::map<unsigned int, BaseThread *> &workers ) {
   for ( unsigned int i = 0; i < _clusterThreads.size(); ++i ) {
      workers.insert( std::make_pair( _clusterThreads[i]->getId(), _clusterThreads[i] ) );
   }
}

void ClusterPlugin::finalize() {
   if ( _gasnetApi->getNodeNum() == 0 ) {
      //message0("Master: Created " << createdWds << " WDs.");
      //message0("Master: Failed to correctly schedule " << sys.getAffinityFailureCount() << " WDs.");
      int soft_inv = 0;
      int hard_inv = 0;
      unsigned int max_execd_wds = 0;
      if ( _remoteNodes ) {
         for ( unsigned int idx = 0; idx < _remoteNodes->size(); idx += 1 ) {
            soft_inv += sys.getSeparateMemory( (*_remoteNodes)[idx]->getMemorySpaceId() ).getSoftInvalidationCount();
            hard_inv += sys.getSeparateMemory( (*_remoteNodes)[idx]->getMemorySpaceId() ).getHardInvalidationCount();
            max_execd_wds = max_execd_wds >= (*_remoteNodes)[idx]->getExecutedWDs() ? max_execd_wds : (*_remoteNodes)[idx]->getExecutedWDs();
            //message("Memory space " << idx <<  " has performed " << _separateAddressSpaces[idx]->getSoftInvalidationCount() << " soft invalidations." );
            //message("Memory space " << idx <<  " has performed " << _separateAddressSpaces[idx]->getHardInvalidationCount() << " hard invalidations." );
         }
      }
      message0("Cluster Soft invalidations: " << soft_inv);
      message0("Cluster Hard invalidations: " << hard_inv);
      //if ( max_execd_wds > 0 ) {
      //   float balance = ( (float) createdWds) / ( (float)( max_execd_wds * (_separateMemorySpacesCount-1) ) );
      //   message0("Cluster Balance: " << balance );
      //}
   }
}


void ClusterPlugin::addPEs( PEMap &pes ) const {
   if ( _remoteNodes ) {
      std::vector<ClusterNode *>::const_iterator it = _remoteNodes->begin();
      //NOT ANYMORE: it++; //position 0 is null, node 0 does not have a ClusterNode object
      for (; it != _remoteNodes->end(); it++ ) {
         pes.insert( std::make_pair( (*it)->getId(), *it ) );
      }
   }
}

unsigned int ClusterPlugin::getNumPEs() const {
   if ( _remoteNodes ) {
      return _remoteNodes->size();
   } else {
      return 0;
   }
}

unsigned int ClusterPlugin::getMaxPEs() const {
   if ( _remoteNodes ) {
      return _remoteNodes->size();
   } else {
      return 0;
   }
}

unsigned int ClusterPlugin::getNumWorkers() const {
   return _clusterThreads.size();
}

}
}

DECLARE_PLUGIN("arch-cluster",nanos::ext::ClusterPlugin);
