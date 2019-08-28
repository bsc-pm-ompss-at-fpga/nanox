/*************************************************************************************/
/*      Copyright 2009-2018 Barcelona Supercomputing Center                          */
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
/*      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            */
/*************************************************************************************/

#include "fpgathread.hpp"
#include "fpgadd.hpp"
#include "fpgaworker.hpp"
#include "fpgaprocessor.hpp"
#include "fpgaprocessorinfo.hpp"
#include "instrumentation_decl.hpp"

using namespace nanos;
using namespace nanos::ext;

FPGAThread::FPGAThread( WD &wd, PE *pe, SMPMultiThread *parent ) :
   BaseThread( parent->getOsId(), wd, pe, parent)
{
   setCurrentWD( wd );
}

void FPGAThread::runDependent()
{
   verbose( "fpga run dependent" );
   WD &work = getThreadWD();
   setCurrentWD( work );
   SMPDD &dd = ( SMPDD & ) work.activateDevice( getSMPDevice() );
   dd.getWorkFct()( work.getData() );
}

void FPGAThread::yield()
{
   verbose("FPGA yield");
   //Synchronizing tasks here seems to yield slightly better performance
   static int const finishBurst = FPGAConfig::getFinishWDBurst();
   ((FPGAProcessor*)runningOn())->tryPostOutlineTasks( finishBurst );
}

void FPGAThread::idle( bool debug )
{
   //Sync tasks on idle
   static int const finishBurst = FPGAConfig::getFinishWDBurst();
   ((FPGAProcessor*)runningOn())->tryPostOutlineTasks( finishBurst );
}

BaseThread *FPGAThread::getNextThread()
{
   if ( getParent() != NULL ) {
      return getParent()->getNextThread();
   } else {
      return this;
   }
}
