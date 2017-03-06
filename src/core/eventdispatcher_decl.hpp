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

#ifndef _EVENTDISPATCHER_DECL_HPP
#define _EVENTDISPATCHER_DECL_HPP

#include "list_decl.hpp"
#include "basethread_decl.hpp"

namespace nanos {

   /*! \brief Common interface to implement Event Listeners
    *         The subclasses only have to implement the callback(BaseThread*) method
    */
   class EventListener {
      public:
         EventListener() {}
         virtual ~EventListener() {}
         virtual void callback( BaseThread* thread ) = 0;
   };

   class EventDispatcher {
      private:
         typedef List<EventListener*> ListenersList;
         ListenersList     _atIdleList;

      public:
         /*! \brief Type of events that the EventDispatcher handles */
         typedef enum { AT_IDLE } EventType;

         EventDispatcher() {}
         ~EventDispatcher();

         /*! \brief Registers a new Event Listener for an Event Type
          *  \param [in]  type  EventType that the listener is interested in
          *  \param [in]  obj   EventListener that must be called when an event happens
          *  \return            Returns true if the Event Listener is successfuly registered,
          *                     false otherwise
          */
         bool addListener( EventType& type, EventListener& obj );

         /*! \brief Registers a new Event Listener for a the AT_IDLE event
          *  \param [in]  obj   EventListener that must be called when an event happens
          *  \return            Returns true if the Event Listener is successfuly registered,
          *                     false otherwise
          */
         bool addListenerAtIdle( EventListener& obj );

         /*! \brief Function that the threads must call when an AT_IDLE event happen
          */
         void atIdle();
   };

} // namespace nanos

#endif // _EVENTDISPATCHER_DECL_HPP
