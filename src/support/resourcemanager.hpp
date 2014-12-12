/*************************************************************************************/
/*      Copyright 2013 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_RESOURCEMANAGER
#define _NANOS_RESOURCEMANAGER

// Sleep time in ns between each sched_yield
#define NANOS_RM_YIELD_SLEEP_NS 20000

namespace nanos {
namespace ResourceManager {
   void init( void );
   void finalize( void );
   void acquireResourcesIfNeeded( void );
   void releaseCpu( void );
   void returnClaimedCpus( void );
   void returnMyCpuIfClaimed( void );
   void waitForCpuAvailability( void );
   bool lastActiveThread( void );
   bool canUntieMaster( void );
}}

#endif /* _NANOS_RESOURCEMANAGER */
