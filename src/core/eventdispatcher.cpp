/*************************************************************************************/
/*      Copyright 2016 Barcelona Supercomputing Center                               */
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

#include "eventdispatcher_decl.hpp"
#include "list.hpp"

namespace nanos {

EventDispatcher::~EventDispatcher() {}

bool EventDispatcher::addListener( EventType& type, EventListener& obj )
{
   if ( type == AT_IDLE ) {
      return addListenerAtIdle( obj );
   }
   return false;
}

bool EventDispatcher::addListenerAtIdle( EventListener& obj )
{
   _atIdleList.push_front( &obj );
   //memoryFence();
   return true;
}

void EventDispatcher::atIdle()
{
   for ( ListenersList::iterator it = _atIdleList.begin(); it != _atIdleList.end(); ++it ) {
      ( *it )->callback( getMyThreadSafe() );
   }
}

} // namespace nanos
