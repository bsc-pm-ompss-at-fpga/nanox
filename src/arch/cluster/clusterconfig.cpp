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

void ClusterConfig::prepare( Config &config )
{
   //config.setOptionsSection( "Cluster Arch", "Cluster spefific options" );

   config.registerConfigOption( "cluster_hybrid_worker", NEW Config::FlagOption( _hybridWorker ),
                                "Allow Cluster helper thread to run SMP tasks when IDLE (def: disabled)" );
   config.registerArgOption( "cluster_hybrid_worker", "cluster-hybrid-worker" );
}

void ClusterConfig::apply() { }

} // namespace ext
} // namespace nanos
