/*************************************************************************************/
/*      Copyright 2010 Barcelona Supercomputing Center                               */
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

#include "fpgadd.hpp"
#include "fpgaprocessor.hpp"

using namespace nanos;
using namespace nanos::ext;

std::vector < FPGADevice * > *FPGADD::_accDevices;

void FPGADD::init() {
    _accDevices = NEW std::vector< FPGADevice * >;
}

FPGADD * FPGADD::copyTo ( void *toAddr )
{
   //Construct into a given address (toAddr) since we are copying
   //we are not allocating anithind, therefore, system allocator cannot be used here
   FPGADD *dd = new ( toAddr ) FPGADD( *this );
   return dd;
}


bool FPGADD::isCompatible ( const Device &arch ) {
   if ( _accNum == -1 ) return true;
   return DeviceData::isCompatible( arch );
}

