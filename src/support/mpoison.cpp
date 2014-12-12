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
#include "debug.hpp"
#include "system.hpp"
#include "vmentry.hpp"

#include <cerrno>
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
   static float rate = sys.getMPoisonRate();
   if( rate <= 0.001f ) // discard really low values and/or negative values
      return NULL;

   useconds_t time_between_failures = 1000000 / rate; // in useconds

   unsigned long *delay_start = (unsigned long*) arg;
   usleep( *delay_start );

   run &= sys.getMPoisonAmount() !=0; // if we must not inject errors, just skip...

   while( run ) {

      while( stop && run ) {}// wait until the other thread indicates 
                             // to continue the poisoning
      mp_mgr->blockPage();
      usleep( time_between_failures );

      // It is intended to make an infinite loop if MPoisonAmount < 0
      if( sys.getMPoisonAmount() < 0 || sys.getMPoisonAmount() > sys.getExceptionStats().getInjectedErrors() ) {
         sys.getExceptionStats().incrInjectedErrors();
      } else {
         run = false;
      }
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
   if ( mp_mgr->unblockPage(page_addr) == 0 ) {
      debug("Resiliency: Page restored! Address: 0x", std::hex, page_addr);
      return 0;
   } else {    
      warning("Resiliency: Error while restoring page. "
              "Address: 0x", std::hex, page_addr,
              " Reason: ", strerror(errno) );
      return -1;
   }
}

void mpoison_delay_start ( unsigned long* useconds ) {
   if( sys.isPoisoningEnabled() ) {
      warning("Memory fault injection: Creating injection thread");
      pthread_create(&tid, NULL, nanos::vm::mpoison_run, (void*)useconds);
   }
}

void mpoison_start ( ) {
   static unsigned long delay = sys.getMPoisonRate() < 0.001f ? 0: 1000000 / sys.getMPoisonRate();
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

void mpoison_finalize ( )
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

void mpoison_declare_region ( uintptr_t addr, size_t size )
{
   using namespace nanos::vm;
   if( sys.isPoisoningEnabled() ) {

      uintptr_t aligned_addr = addr & ~(page_size-1);
      size_t aligned_size =( (addr + size) & ~(page_size-1) ) // end of region
                           - addr;// region aligned beginning

      if( addr != aligned_addr || size != aligned_size )
         warning0( "Memory error injection: Adding a memory chunk that is not aligned "
                   "(either starting address or length) to a memory page. "
                   "This can make the execution unstable." );

      ensure0( aligned_size > 0,
               "Error injection memory chunk is null. "
               "Make sure base address and size are properly aligned."
             );

      // If aligned_size < size we will still use aligned_size, as it is dangerous to inject
      // errors in pages that can contain other data structures that are not taken in
      // account by the user.

      mp_mgr->addAllocation( aligned_addr, aligned_size );
   }
}

void mpoison_scan ()
{
   using namespace nanos::vm;

   if ( sys.isPoisoningEnabled() ) {
      std::string path;
 
      std::ifstream maps("/proc/self/maps");
      if (!maps)
        fatal("Memory error injection: Can't open 'maps' file");
 
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

