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

#include "fpgalistener.hpp"
#include "queue.hpp"

using namespace nanos;
using namespace ext;

//TODO: Get the value from the FPGAConfig
unsigned int FPGAListener::_maxConcurrentThreads = 1;

void FPGAListener::callback( BaseThread* self )
{
   // Try to atomically reserve an slot
   if ( _count.fetchAndAdd() < _maxConcurrentThreads ) {
      PE * const selfPE = self->runningOn();
      WD * const selfWD = self->getCurrentWD();
      FPGAProcessor * const fpga = getFPGAProcessor();
      //verbose("FPGAListener::callback\t Thread " << self->getId() << " gets work for FPGA-PE (" << fpga << ")");

      //Simulate that the SMP thread runs on the FPGA PE
      self->setRunningOn( fpga );

      FPGAWorker::tryOutlineTask( self );

      //Restore the running PE of SMP Thread and the running WD (just in case)
      self->setCurrentWD( *selfWD );
      self->setRunningOn( selfPE );
   }
   --_count;
}
