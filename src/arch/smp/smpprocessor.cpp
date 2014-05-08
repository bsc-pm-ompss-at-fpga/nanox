/*************************************************************************************/
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

#include "smpprocessor.hpp"
#include "schedule.hpp"
#include "debug.hpp"
#include "config.hpp"
#include <iostream>

using namespace nanos;
using namespace nanos::ext;

bool SMPProcessor::_useUserThreads = true;
size_t SMPProcessor::_threadsStackSize = 0;
System::CachePolicyType SMPProcessor::_cachePolicy = System::DEFAULT;
size_t SMPProcessor::_cacheDefaultSize = 1048580;

SMPProcessor::SMPProcessor( int bindingId ) : PE( &SMP, NULL, sys.getRootMemorySpaceId() ), _bindingId( bindingId )  {}

void SMPProcessor::prepareConfig ( Config &config )
{
   config.registerConfigOption( "user-threads", NEW Config::FlagOption( _useUserThreads, false), "Disable use of user threads to implement workdescriptor" );
   config.registerArgOption( "user-threads", "disable-ut" );

   config.registerConfigOption ( "pthreads-stack-size", NEW Config::SizeVar( _threadsStackSize ), "Defines pthreads stack size" );
   config.registerArgOption( "pthreads-stack-size", "pthreads-stack-size" );

//#if SMP_NUMA
//   System::CachePolicyConfig *cachePolicyCfg = NEW System::CachePolicyConfig ( _cachePolicy );
//   cachePolicyCfg->addOption("wt", System::WRITE_THROUGH );
//   cachePolicyCfg->addOption("wb", System::WRITE_BACK );
//   cachePolicyCfg->addOption( "nocache", System::NONE );
//   config.registerConfigOption ( "numa-cache-policy", cachePolicyCfg, "Defines the cache policy for SMP_NUMA architectures: write-through / write-back (wt by default)" );
//   config.registerEnvOption ( "numa-cache-policy", "NX_NUMA_CACHE_POLICY" );
//   config.registerArgOption( "numa-cache-policy", "numa-cache-policy" );
//
//   config.registerConfigOption ( "numa-cache-size", NEW Config::SizeVar( _cacheDefaultSize ), "Defines size of the cache for SMP_NUMA architectures" );
//   config.registerArgOption( "numa-cache-size", "numa-cache-size" );
//
//   // Check if the use of caches has been disabled
//   if ( sys.isCacheEnabled() ) {
//      // Check if the cache policy for SMP_NUMA has been defined
//      if ( _cachePolicy == System::DEFAULT ) {
//         // The user has not defined a specific cache policy for SMP_NUMA,
//         // check if he has defined a global cache policy
//         _cachePolicy = sys.getCachePolicy();
//         if ( _cachePolicy == System::DEFAULT ) {
//            // There is no global cache policy specified, assign it the default value (write-through)
//            _cachePolicy = System::WRITE_THROUGH;
//         }
//      }
//   } else {
//      _cachePolicy = System::NONE;
//   }
//
//   configureCache( _cacheDefaultSize, _cachePolicy );
//
//#endif
}

WorkDescriptor & SMPProcessor::getWorkerWD () const
{
   SMPDD  *dd = NEW SMPDD( ( SMPDD::work_fct )Scheduler::workerLoop );
   DeviceData **dd_ptr = NEW DeviceData*[1];
   dd_ptr[0] = (DeviceData*)dd;

   WD *wd = NEW WD( 1, dd_ptr );

   return *wd;
}

WorkDescriptor & SMPProcessor::getMultiWorkerWD () const
{
#ifdef CLUSTER_DEV
   SMPDD * dd = NEW SMPDD( ( SMPDD::work_fct )Scheduler::workerClusterLoop );
#else
   SMPDD * dd = NEW SMPDD( ( SMPDD::work_fct )NULL); //this is an error if we are not runnig with cluster
#endif
   WD *wd = NEW WD( dd );
   return *wd;
}

WorkDescriptor & SMPProcessor::getMasterWD () const
{
   SMPDD  *dd = NEW SMPDD();
   DeviceData **dd_ptr = NEW DeviceData*[1];
   dd_ptr[0] = (DeviceData*)dd;

   WD * wd = NEW WD( 1, dd_ptr );

   return *wd;
}

BaseThread &SMPProcessor::createThread ( WorkDescriptor &helper, SMPMultiThread *parent )
{
   ensure( helper.canRunIn( SMP ),"Incompatible worker thread" );
   SMPThread &th = *NEW SMPThread( helper, this, this );
   th.stackSize( _threadsStackSize ).useUserThreads( _useUserThreads );

   return th;
}

BaseThread &SMPProcessor::createMultiThread ( WorkDescriptor &helper, unsigned int numPEs, PE **repPEs )
{
   ensure( helper.canRunIn( SMP ),"Incompatible worker thread" );
   SMPThread &th = *NEW SMPMultiThread( helper, this, numPEs, repPEs );
   th.stackSize(_threadsStackSize).useUserThreads(_useUserThreads);

   return th;
}

//void* SMPProcessor::getAddressDependent( uint64_t tag )
//{
//#ifdef CLUSTER_DEV
//   void * res;
//   res = (void *) tag;
//   fprintf(stderr, "WHAT DO NOW?!\n");
//   return res;
//#else
// return _cache.getAddress( tag );
//#endif
//}
//void* SMPProcessor::newGetAddressDependent( CopyData const &cd )
//{
// (void) cd;
// return (void*) 0xdeadebef;
//}
//bool SMPProcessor::supportsDirectTransfersWith(ProcessingElement const &pe ) const {
//   return false;
//}
