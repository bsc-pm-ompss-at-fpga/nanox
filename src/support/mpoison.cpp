/*************************************************************************************/
/*      Copyright 2014 Barcelona Supercomputing Center                               */
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

/*
 * Note: for documentation about /proc/$pid/maps file, see
 * https://www.kernel.org/doc/Documentation/filesystems/proc.txt
 */
#include "mpoison.hpp"
#include "mpoison.h"
#include "system.hpp"
#include "vmentry.hpp"

#include <unistd.h>

/* Variables used to manage and synchronize with mpoison thread */
pthread_t tid;
volatile bool started = false;
volatile bool stop = false;
volatile bool run = false;

nanos::vm::MPoisonManager *mp_mgr;

nanos::vm::MPoisonManager *nanos::vm::getMPoisonManager() {
   return mp_mgr;
}

void *nanos::vm::mpoison_run(void *arg)
{
   long *delay_start = (long*) arg;
   usleep( *delay_start );

   while( run ) {
      while(stop && run) {}// wait until the other thread indicates 
                           // to continue the poisoning
      mp_mgr->blockPage();
      usleep( sys.getMPoisonRate() );
   }

   return NULL;
}

extern "C" {

void mpoison_stop(){
   stop = true;
}

void mpoison_continue(){
   stop = false;
}

int mpoison_unblock_page( uintptr_t page_addr ) {
   return mp_mgr->unblockPage(page_addr);
}

void mpoison_delay_start ( long *useconds ) {
   debug("Resiliency: MPoison: Creating mpoison thread");
   pthread_create(&tid, NULL, nanos::vm::mpoison_run, (void*)useconds);
}

void mpoison_start ( ) {
   static long delay = 0;
   mpoison_delay_start(&delay);
}

void mpoison_init ( )
{
   using namespace nanos::vm;

   mp_mgr = new MPoisonManager( sys.getMPoisonSeed());

   stop = false;
   run = true;
}

void
mpoison_finalize ( )
{
   run = false;
   void *ret;
   pthread_join(tid, &ret);

   delete mp_mgr;
}

void mpoison_scan ()
{
   using namespace nanos::vm;

   std::string path;
 
   std::ifstream maps("/proc/self/maps");
   if (!maps)
     fatal("Mpoison: Can't open 'maps' file");
 
   nanos::vm::VMEntry vme; // entry in maps file
   while (maps >> vme)
   {
       // if a valid map entry was read...
       vm::prot_t access = vme.getAccessRights();
       if ( access.r && 
            access.w && 
           !access.x && //only look for the frame if is R+W, not X 
           !vme.isSyscallArea()) 
       {
           uintptr_t addr = vme.getStart() & ~(page_size-1);
           size_t region_size = vme.getEnd() - vme.getStart();
 
           mp_mgr->addAllocation( addr, region_size );
       }
   }
}

}// extern C

