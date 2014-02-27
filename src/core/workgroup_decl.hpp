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

#ifndef _NANOS_WORK_GROUP_DECL_H
#define _NANOS_WORK_GROUP_DECL_H

#include <vector>
#include "atomic_decl.hpp"
#include "dependenciesdomain_decl.hpp"
#include "synchronizedcondition_decl.hpp"

namespace nanos
{
  /* \class WorkGroup
   *
   */
   class WorkGroup
   {
      private:
         typedef std::vector<WorkGroup *> WGList; // FIXME: vector is not a safe-class here

      private:
         WGList         _partOf; // other than parent
         int            _id;
         Atomic<int>    _components;

         SingleSyncCond<EqualConditionChecker<int> > _syncCond;

         WorkGroup     *_parent; // most WG will only have one parent

      private:
         void addToGroup ( WorkGroup &parent );

         /*! \brief WorkGroup copy assignment operator (private)
          */
         const WorkGroup & operator= ( const WorkGroup &wg );

      public:
         void exitWork ( WorkGroup &work );

         /*! \brief WorkGroup default constructor
          */
         WorkGroup();

         /*! \brief WorkGroup copy constructor
          */
         WorkGroup( const WorkGroup &wg );

         /*! \brief WorkGroup destructor 
          */
         virtual ~WorkGroup();

         void addWork( WorkGroup &wg );
         virtual void waitCompletion( bool avoidFlush = false );
         virtual void init();
         virtual void done();
         virtual void clear();
         WorkGroup * getWGParent( void ) const;
         int getId() const { return _id; }

   };

   typedef WorkGroup WG;

};

#endif

