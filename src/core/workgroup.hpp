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

#ifndef _NANOS_WORK_GROUP
#define _NANOS_WORK_GROUP

#include <vector>
#include "atomic.hpp"
#include "dependenciesdomain.hpp"

namespace nanos
{

   class WorkGroup : public DependableObject
   {

      private:
         static Atomic<int> _atomicSeed;

         // FIX-ME: vector is not a safe-class here
         typedef std::vector<WorkGroup *> WGList;

         WGList         _partOf;
         int            _id;
         Atomic<int>    _components;
         Atomic<int>    _phaseCounter;
         DependenciesDomain _depsDomain;

         void addToGroup ( WorkGroup &parent );
         void exitWork ( WorkGroup &work );

         WorkGroup( const WorkGroup &wg );
         const WorkGroup & operator= ( const WorkGroup &wg );

      public:
         // constructors
         WorkGroup() : DependableObject(),_id( _atomicSeed++ ),_components( 0 ), _phaseCounter( 0 ),_depsDomain() {  }

         // destructor
         virtual ~WorkGroup();

         void addWork( WorkGroup &wg );
         void sync();
         void waitCompletation();
         void done();
         int getId() const { return _id; }
         virtual void dependenciesSatisfied();

         void submitWithDependencies(WorkGroup &wg, int numDeps, Dependency* deps)
         {
            _depsDomain.submitDependableObject(wg, numDeps, deps);
         }

         void workFinished(WorkGroup &wg)
         {
            _depsDomain.finished( wg );
         }
   };

   typedef WorkGroup WG;

};

#endif

