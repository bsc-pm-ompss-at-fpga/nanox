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

#ifndef CLUSTERPLUGIN_DECL_H
#define CLUSTERPLUGIN_DECL_H

#include "plugin.hpp"
#include "system_decl.hpp"
#include "clusternode_decl.hpp"
#include "gasnetapi_fwd.hpp"

namespace nanos {
namespace ext {

//! \brief ClusterListener to be registered in the EventDispatcher
class ClusterListener : public EventListener {
   private:
      //Disable copy constructor
      ClusterListener( const ClusterListener &ref ) { }

   public:
      //Default constructor and destructor
      ClusterListener() { }
      ~ClusterListener() { }

      //! \brief Callback executed when a worker become idle
      void callback( BaseThread * thread ) {
         thread->processTransfers();
      }
};

class ClusterPlugin : public ArchPlugin
{
      GASNetAPI                          *_gasnetApi;

      unsigned int                        _numPinnedSegments;
      void                              **_pinnedSegmentAddrList;
      std::size_t                        *_pinnedSegmentLenList;
      unsigned int                        _extraPEsCount;
      std::string                         _conduit;
      std::vector<ext::ClusterNode *>    *_remoteNodes;
      std::vector<ext::SMPProcessor *>    _cpus;
      std::vector<ext::SMPMultiThread *>  _clusterThreads;
      ClusterListener                     _clusterListener; /*! \brief Cluster listener for atIdle events */

   public:
      ClusterPlugin();
      virtual void config( Config& cfg );
      virtual void init();

      RemoteWorkDescriptor * getRemoteWorkDescriptor( unsigned int nodeId, int archId );

      virtual void startSupportThreads();
      virtual void startWorkerThreads( std::map<unsigned int, BaseThread *> &workers);
      virtual void finalize();

      virtual ProcessingElement * createPE( unsigned id, unsigned uid );
      virtual unsigned getNumThreads() const;
      void addPEs( PEMap &pes ) const;
      virtual void addDevices( DeviceList &devices ) const {}
      virtual unsigned int getNumPEs() const;
      virtual unsigned int getMaxPEs() const;
      virtual unsigned int getNumWorkers() const;
};

} // namespace ext
} // namespace nanos


#endif /* CLUSTERPLUGIN_DECL_H */
