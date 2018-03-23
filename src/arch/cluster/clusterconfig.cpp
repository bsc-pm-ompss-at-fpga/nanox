/*************************************************************************************/
/*      Copyright 2018 Barcelona Supercomputing Center                               */
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

#include "clusterconfig.hpp"
#include "config.hpp"

#if defined(__SIZEOF_SIZE_T__)
#  if  __SIZEOF_SIZE_T__ == 8
#    define DEFAULT_NODE_MEM  (0x80000000ULL) //2Gb
#    define MAX_NODE_MEM     (0x542000000ULL)
#  elif __SIZEOF_SIZE_T__ == 4
#    define DEFAULT_NODE_MEM (0x40000000UL)
#    define MAX_NODE_MEM     (0x40000000UL)
#  else
#    error "Weird. Only 8 and 4 are supported for sizeof(size_t)"
#  endif
#else
#  error "I need to know the size of a size_t"
#endif

namespace nanos {
namespace ext {

bool ClusterConfig::_hybridWorker = false;
bool ClusterConfig::_hybridWorkerPlus = false;
bool ClusterConfig::_slaveNodeWorker = false;
int ClusterConfig::_smpPresend = 1;
int ClusterConfig::_gpuPresend = 1;
int ClusterConfig::_oclPresend = 1;
int ClusterConfig::_fpgaPresend = 1;
unsigned int ClusterConfig::_maxArchId = 3; //< 0: SMP, 1: CUDA, 2: OCL, 3: FPGA
bool ClusterConfig::_sharedWorkerPE = false;
int ClusterConfig::_bindingStart = -1;
int ClusterConfig::_bindingStride = 1;
std::size_t ClusterConfig::_nodeMem = DEFAULT_NODE_MEM;
bool ClusterConfig::_allocFit = false;
bool ClusterConfig::_unalignedNodeMem = false;
System::CachePolicyType ClusterConfig::_cachePolicy = System::DEFAULT;
std::size_t ClusterConfig::_gasnetSegmentSize = 0;
std::list<unsigned int> * ClusterConfig::_numWorkers = NULL;

void ClusterConfig::prepare( Config &cfg )
{
   cfg.setOptionsSection( "Cluster Arch", "Cluster specific options" );

   cfg.registerConfigOption( "cluster_hybrid_worker", NEW Config::FlagOption( _hybridWorker ),
      "Allow Cluster helper thread to run SMP tasks when IDLE (def: disabled)" );
   cfg.registerArgOption( "cluster_hybrid_worker", "cluster-hybrid-worker" );

   cfg.registerConfigOption( "cluster_hybrid_plus", NEW Config::FlagOption( _hybridWorkerPlus ),
      "Enable call to Event Dispatcher when the cluster helper thread is IDLE (def: disabled). It will also enable '--cluster-hybrid-worker'" );
   cfg.registerArgOption( "cluster_hybrid_plus", "cluster-hybrid-worker-plus" );

   cfg.registerConfigOption( "cluster_slave_worker", NEW Config::FlagOption( _slaveNodeWorker ),
      "Enable Cluster helper thread in slave Nodes (def: disabled)" );
   cfg.registerArgOption( "cluster_slave_worker", "cluster-slave-worker" );

   cfg.registerConfigOption( "cluster_smp_presend", NEW Config::IntegerVar( _smpPresend ),
      "Number of Tasks (SMP arch) to be sent to a remote node without waiting any completion." );
   cfg.registerArgOption( "cluster_smp_presend", "cluster-smp-presend" );
   cfg.registerEnvOption( "cluster_smp_presend", "NX_CLUSTER_SMP_PRESEND" );

   cfg.registerConfigOption( "cluster_gpu_presend", NEW Config::IntegerVar( _gpuPresend ),
      "Number of Tasks (GPU arch) to be sent to a remote node without waiting any completetion." );
   cfg.registerArgOption( "cluster_gpu_presend", "cluster-gpu-presend" );
   cfg.registerEnvOption( "cluster_gpu_presend", "NX_CLUSTER_GPU_PRESEND" );

   cfg.registerConfigOption( "cluster_ocl_presend", NEW Config::IntegerVar( _oclPresend ),
      "Number of Tasks (OpenCL arch) to be sent to a remote node without waiting any completion." );
   cfg.registerArgOption( "cluster_ocl_presend", "cluster-ocl-presend" );
   cfg.registerEnvOption( "cluster_ocl_presend", "NX_CLUSTER_OCL_PRESEND" );

   cfg.registerConfigOption( "cluster_fpga_presend", NEW Config::IntegerVar( _fpgaPresend ),
      "Number of Tasks (FPGA arch) to be sent to a remote node without waiting any completion." );
   cfg.registerArgOption( "cluster_fpga_presend", "cluster-fpga-presend" );
   cfg.registerEnvOption( "cluster_fpga_presend", "NX_CLUSTER_FPGA_PRESEND" );

   cfg.registerConfigOption( "cluster_shared_pe", NEW Config::FlagOption( _sharedWorkerPE ),
      "Allow the cluster thread to share CPU with other threads (def: disabled)" );
   cfg.registerArgOption( "cluster_shared_pe", "cluster-allow-shared-thread" );
   cfg.registerEnvOption( "cluster_shared_pe", "NX_CLUSTER_ALLOW_SHARED_THREAD" );

   cfg.registerConfigOption( "cluster_worker_binding", NEW Config::IntegerVar( _bindingStart ),
      "PE id where the 1st cluster worker thread must run." );
   cfg.registerArgOption( "cluster_worker_binding", "cluster-worker-binding" );
   cfg.registerEnvOption( "cluster_worker_binding", "NX_CLUSTER_WORKER_BINDING" );

   cfg.registerConfigOption( "cluster_binding_stride", NEW Config::IntegerVar( _bindingStride ),
      "Stride between cluster threads. Ignored if no 'cluster-worker-binding' provided (def: 1)." );
   cfg.registerArgOption( "cluster_binding_stride", "cluster-worker-binding-stride" );
   cfg.registerEnvOption( "cluster_binding_stride", "NX_CLUSTER_WORKER_BINDING_STRIDE" );

   /* Cluster: memory size to be allocated on remote nodes */
   cfg.registerConfigOption( "node_memory", NEW Config::SizeVar( _nodeMem ),
      "Sets the memory size that will be used on each node to send and receive data (def: ~21GB)" );
   cfg.registerArgOption( "node_memory", "cluster-node-memory" );
   cfg.registerEnvOption( "node_memory", "NX_CLUSTER_NODE_MEMORY" );

   cfg.registerAlias( "node_memory", "node_memory_mpi",
      "Alias to cluster-node-memory option" );
   cfg.registerArgOption( "node_memory_mpi", "cluster-node-memory-mpi" );
   cfg.registerEnvOption( "node_memory_mpi", "NX_CLUSTER_NODE_MEMORY_MPI" );

   cfg.registerConfigOption( "cluster_alloc_fit", NEW Config::FlagOption( _allocFit ),
      "Allocate full objects (def: false)" );
   cfg.registerArgOption( "cluster_alloc_fit", "cluster-alloc-fit" );

   System::CachePolicyConfig *cachePolicyCfg = NEW System::CachePolicyConfig( _cachePolicy );
   cachePolicyCfg->addOption("wt", System::WRITE_THROUGH );
   cachePolicyCfg->addOption("wb", System::WRITE_BACK );
   cachePolicyCfg->addOption("no", System::NONE );
   cfg.registerConfigOption( "cluster_cache_policy", cachePolicyCfg,
      "Defines the cache policy for Cluster architectures: write-through / write-back (wb by default)" );
   cfg.registerEnvOption( "cluster_cache_policy", "NX_CLUSTER_CACHE_POLICY" );
   cfg.registerArgOption( "cluster_cache_policy", "cluster-cache-policy" );

   cfg.registerConfigOption( "cluster_unaligned_node_memory", NEW Config::FlagOption( _unalignedNodeMem ),
      "Do not align node memory (def: false)" );
   cfg.registerArgOption( "cluster_unaligned_node_memory", "cluster-unaligned-node-memory" );
   cfg.registerEnvOption( "cluster_unaligned_node_memory", "NX_CLUSTER_UNALIGNED_NODE_MEMORY" );

   cfg.registerConfigOption( "gasnet_segment", NEW Config::SizeVar( _gasnetSegmentSize ),
      "GASNet segment size (def: 0)" );
   cfg.registerArgOption( "gasnet_segment", "gasnet-segment-size" );
   cfg.registerEnvOption( "gasnet_segment", "NX_GASNET_SEGMENT_SIZE" );

   _numWorkers = new std::list<unsigned int>();
   cfg.registerConfigOption( "cluster_num_workers", NEW Config::UintVarList( *_numWorkers ),
      "Defines the number of cluster helper threads in each node. If only one value is provided, all nodes use it (def: 1)" );
   cfg.registerArgOption( "cluster_num_workers", "cluster-helper-threads" );
   cfg.registerEnvOption( "cluster_num_workers", "NX_CLUSTER_HELPER_THREADS" );
}

void ClusterConfig::apply() {
   //Also enable hybrid worker if hybrid worker plus is enabled
   _hybridWorker |= _hybridWorkerPlus;
}

} // namespace ext
} // namespace nanos

#undef DEFAULT_NODE_MEM
#undef MAX_NODE_MEM
