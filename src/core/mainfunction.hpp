/*************************************************************************************/
/*      Copyright 2009-2016 Barcelona Supercomputing Center                          */
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

#ifndef MAIN_FUNCTION_HPP
#define MAIN_FUNCTION_HPP

#include "mainfunction_fwd.hpp"

#include "instrumentation.hpp"
#include "system.hpp"

#include <string>
#include <sstream>

namespace nanos {
namespace instrumentation {

class MainFunctionEvent {
   private:
      InstrumentationDictionary& _instrumentationDict;
      nanos_event_value_t        _eventValue;
      nanos_event_key_t          _eventKey;

      std::string generateEventName( const char* filename, int line );

      std::string generateEventDescription( const char* filename, int line );

   public:
      MainFunctionEvent( memory::Address functionAddress, const char* filename, int line ) :
         _instrumentationDict( *sys.getInstrumentation()->getInstrumentationDictionary() ),
         _eventValue( static_cast<nanos_event_value_t>(functionAddress.value()) ),
         _eventKey( _instrumentationDict.getEventKey("user-funct-location") )
      {
         _instrumentationDict.registerEventValue(
            /* key */ "user-funct-location",
            /* value */ generateEventName(filename,line),
            /* val */ _eventValue,
            /* description */ generateEventDescription(filename,line),
            /* abort_when_registered */ true
            );
         sys.getInstrumentation()->raiseOpenBurstEvent( _eventKey, _eventValue );
      }

      ~MainFunctionEvent()
      {
         sys.getInstrumentation()->raiseCloseBurstEvent( _eventKey, _eventValue );
      }
};

std::string MainFunctionEvent::generateEventName( const char* filename, int line )
{
   std::stringstream ss;
   ss << "main@" << filename << "@" << line << "@FUNCTION";
   return ss.str();
}

std::string MainFunctionEvent::generateEventDescription( const char* filename, int line )
{
   std::stringstream ss;
   ss << "int main(int, char**)@" << filename << "@" << line << "@FUNCTION";
   return ss.str();
}

} // namespace instrumentation
} // namespace nanos

#endif // MAIN_FUNCTION_HPP
