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

#ifndef _NANOS_CLUSTER_CFG
#define _NANOS_CLUSTER_CFG
#include "config.hpp"

namespace nanos {
namespace ext {

   class ClusterConfig
   {
      private:
         static bool            _hybridWorker;   /*! \brief Enable/disable hybrid cluster worker */

      public:
         /*! Parses the Cluster user options */
         static void prepare ( Config &config );

         /*! Applies the configuration options
          */
         static void apply ( void );

         static bool getHybridWorkerEnabled() { return _hybridWorker; }
   };

} // namespace ext
} // namespace nanos
#endif // _NANOS_CLUSTER_CFG
