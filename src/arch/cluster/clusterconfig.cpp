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

namespace nanos {
namespace ext {

bool ClusterConfig::_hybridWorker = false;
int ClusterConfig::_smpPresend = 1;
int ClusterConfig::_gpuPresend = 1;
int ClusterConfig::_oclPresend = 1;
int ClusterConfig::_fpgaPresend = 1;

void ClusterConfig::prepare( Config &cfg )
{
   //cfg.setOptionsSection( "Cluster Arch", "Cluster spefific options" );

   cfg.registerConfigOption( "cluster_hybrid_worker", NEW Config::FlagOption( _hybridWorker ),
      "Allow Cluster helper thread to run SMP tasks when IDLE (def: disabled)" );
   cfg.registerArgOption( "cluster_hybrid_worker", "cluster-hybrid-worker" );

   cfg.registerConfigOption ( "cluster-smp-presend", NEW Config::IntegerVar( _smpPresend ),
      "Number of Tasks (SMP arch) to be sent to a remote node without waiting any completion." );
   cfg.registerArgOption ( "cluster-smp-presend", "cluster-smp-presend" );
   cfg.registerEnvOption ( "cluster-smp-presend", "NX_CLUSTER_SMP_PRESEND" );

   cfg.registerConfigOption ( "cluster-gpu-presend", NEW Config::IntegerVar( _gpuPresend ),
      "Number of Tasks (GPU arch) to be sent to a remote node without waiting any completetion." );
   cfg.registerArgOption ( "cluster-gpu-presend", "cluster-gpu-presend" );
   cfg.registerEnvOption ( "cluster-gpu-presend", "NX_CLUSTER_GPU_PRESEND" );

   cfg.registerConfigOption ( "cluster-ocl-presend", NEW Config::IntegerVar( _oclPresend ),
      "Number of Tasks (OpenCL arch) to be sent to a remote node without waiting any completion." );
   cfg.registerArgOption ( "cluster-ocl-presend", "cluster-ocl-presend" );
   cfg.registerEnvOption ( "cluster-ocl-presend", "NX_CLUSTER_OCL_PRESEND" );

   cfg.registerConfigOption ( "cluster-fpga-presend", NEW Config::IntegerVar( _fpgaPresend ),
      "Number of Tasks (FPGA arch) to be sent to a remote node without waiting any completion." );
   cfg.registerArgOption ( "cluster-fpga-presend", "cluster-fpga-presend" );
   cfg.registerEnvOption ( "cluster-fpga-presend", "NX_CLUSTER_FPGA_PRESEND" );
}

void ClusterConfig::apply() { }

} // namespace ext
} // namespace nanos
