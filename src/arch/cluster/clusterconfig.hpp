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
#include "system_decl.hpp"

namespace nanos {
namespace ext {

   class ClusterConfig
   {
      private:
         static bool            _hybridWorker;    /*! \brief Enable/disable hybrid cluster worker */
         static bool            _slaveNodeWorker; /*! \brief Enable/disable cluster worker in slave nodes */
         static int             _smpPresend;      /*! \brief Max. number of tasks sent to remote node without waiting */
         static int             _gpuPresend;      /*! \brief Max. number of tasks sent to remote node without waiting */
         static int             _oclPresend;      /*! \brief Max. number of tasks sent to remote node without waiting */
         static int             _fpgaPresend;     /*! \brief Max. number of tasks sent to remote node without waiting */
         static unsigned int    _maxArchId;       /*! \brief Max. cluster architecture identifier. Any id will be in [0, _maxArchId] */
         static bool            _sharedWorkerPE;  /*! \brief Enable/disable sharing PE for cluster worker */
         static int             _bindingWorker;   /*! \brief PE id where the cluster thread must run */
         static std::size_t     _nodeMem;         /*! \brief Memory pool size in each node */
         static bool            _allocFit;        /*! \brief Alloc objects in a fitted way */
         static bool            _unalignedNodeMem;    /*! \brief Allow unaligned memory allocations? */
         static System::CachePolicyType _cachePolicy; /*! \brief Cluster cache policy */
         static std::size_t     _gasnetSegmentSize;   /*! \brief ??? */

      public:
         /*! Parses the Cluster user options */
         static void prepare( Config &config );

         /*! Applies the configuration options
          */
         static void apply( void );

         static bool getHybridWorkerEnabled() { return _hybridWorker; }
         static bool getSlaveNodeWorkerEnabled() { return _slaveNodeWorker; }
         static int getSmpPresend() { return _smpPresend; }
         static int getGpuPresend() { return _gpuPresend; }
         static int getOclPresend() { return _oclPresend; }
         static int getFpgaPresend() { return _fpgaPresend; }
         static unsigned int getMaxClusterArchId() { return _maxArchId; }
         static void setMaxClusterArchId( unsigned int const num ) { _maxArchId = num; }
         static bool getSharedWorkerPeEnabled() { return _sharedWorkerPE; }
         static int getClusterWorkerBinding() { return _bindingWorker; }
         static std::size_t getNodeMem() { return _nodeMem; }
         static bool getAllocFitEnabled() { return _allocFit; }
         static bool getUnaligMemEnabled() { return _unalignedNodeMem; }
         static System::CachePolicyType getCachePolicy() { return _cachePolicy; }
         static std::size_t getGasnetSegmentSize() { return _gasnetSegmentSize; }
   };

} // namespace ext
} // namespace nanos
#endif // _NANOS_CLUSTER_CFG
