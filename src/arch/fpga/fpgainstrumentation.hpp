/*************************************************************************************/
/*      Copyright 2017 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_FPGA_INSTRUMENTATION
#define _NANOS_FPGA_INSTRUMENTATION

#include "instrumentation.hpp"
#include "fpgaprocessorinfo.hpp"

namespace nanos {
namespace ext {
   class FPGAInstrumentation : public DeviceInstrumentation {
      private:
         std::string               _deviceType; //< Name to be shown in the trace
         FPGAProcessorInfo const  *_deviceInfo; //< Reference to FPGA dependent information
      public:
         FPGAInstrumentation() : DeviceInstrumentation(), _deviceType( "NULL" ), _deviceInfo( NULL ) { }

         FPGAInstrumentation( int id, std::string deviceType, FPGAProcessorInfo const * const fpgaInfo ) :
            DeviceInstrumentation( id ), _deviceType( deviceType ), _deviceInfo( fpgaInfo ) { }

         virtual void init() {}
         //! \breif Returns the device time in cycles
         virtual unsigned long long int getDeviceTime();
         //! \brief Translates a raw device time in cycles to ns
         virtual unsigned long long int translateDeviceTime( unsigned long long int );
         virtual void startDeviceTrace() {}
         virtual void pauseDeviceTrace( bool pause ) {}
         virtual void stopDeviceTrace() {}
         //! \brief Returns the device name
         virtual const char* getDeviceType() { return _deviceType.c_str(); }
   };
} //namespace ext
} //namespace nanos

#endif /* _NANOS_FPGA_INSTRUMENTATION */
