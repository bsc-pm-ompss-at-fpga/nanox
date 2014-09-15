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

void mpoison_delay_start ( unsigned* useconds ) {
   if( sys.isPoisoningEnabled() ) {
      debug("Resiliency: MPoison: Creating mpoison thread");
      pthread_create(&tid, NULL, nanos::vm::mpoison_run, (void*)useconds);
   }
}

void mpoison_start ( ) {
   static unsigned delay = sys.getMPoisonRate();
   mpoison_delay_start( &delay );
}

void mpoison_init ( )
{
   using namespace nanos::vm;

   if ( sys.isPoisoningEnabled() ) {
      mp_mgr = new MPoisonManager( sys.getMPoisonSeed());

      stop = false;
      run = true;
   }
}

void
mpoison_finalize ( )
{
   if ( sys.isPoisoningEnabled() ) {
      run = false;
      void *ret;

      if( tid != 0 ) {
         pthread_join(tid, &ret);
         delete mp_mgr;

         tid = 0;
         mp_mgr = NULL;
      }
   }
}

void mpoison_user_defined ( size_t len, chunk_t data_chunks[len] )
{
   using namespace nanos::vm;
   if( sys.isPoisoningEnabled() ) {
      for( size_t i = 0; i < len; i++ ) {
         uintptr_t addr = data_chunks[i].addr & ~(page_size-1);
         size_t region_size = (data_chunks[i].addr + data_chunks[i].size) & ~(page_size-1)// end of region
                              - addr;// region aligned beginning
         if( region_size < data_chunks[i].size )
            region_size += page_size; 
         mp_mgr->addAllocation( addr, region_size );
      }
   }
}

void mpoison_scan ()
{
   using namespace nanos::vm;

   if ( sys.isPoisoningEnabled() ) {
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
}

}// extern C

