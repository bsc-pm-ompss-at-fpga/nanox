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

#include <stdexcept>
#include <fstream>
#include <map>
#include <list>
#include <random>

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>

#include "../core/schedule.hpp"
#include "../core/synchronizedcondition.hpp"
#include "vmentry.hpp"
#include "mempage.hpp"
#include "mpoison.hpp"

using namespace nanos;

size_t collect( );
bool specific_block( void* );
bool random_block( size_t );

pid_t pid;
unsigned long page_size;

static bool wait;
volatile bool stop;
std::map<size_t, vm::VMEntry> entries;
std::vector<const vm::MemPage*> pages;

EqualConditionChecker<bool> waitCond = EqualConditionChecker<bool>(&wait, false);


void mpoison_wait ( )
{
   Scheduler::waitOnCondition((GenericSyncCond*)&waitCond);
}

void mpoison_stop(){
     stop = true;
}

void mpoison_continue() {
   stop= false;
}

void mpoison_init ( )
{
   wait = true;
   stop = true;
}

size_t
collect()
{
  pid = getpid();
  page_size = sysconf(_SC_PAGESIZE);

  std::string path;
  size_t total = 0;

  entries.clear();

  std::ifstream maps("/proc/self/maps");
  if (!maps)
    fatal("Can't open 'maps' file");

  FILE *pagemap = fopen("/proc/self/pagemap", "rb");
  if (!pagemap)
    fatal("Can't open 'pagemap' file");

  vm::VMEntry vme; // entry in maps file
  while (maps >> vme)
    {
      // if a valid map entry was read...
      uint64_t pageNumber = vme.getStart() / page_size;
      uint64_t regionSize = vme.getEnd() - vme.getStart();
      uint64_t pagesInRegion = regionSize / page_size;

      vm::prot_t access = vme.getAccessRights();
      if ( access.r && 
           access.w && 
          !access.x && //only look for the frame if is R+W, not X 
          !vme.isSyscallArea()) 
        {
          // Advances file read position to the first page of the region
          fseek(pagemap, pageNumber * PM_ENTRY_BYTES, SEEK_SET);

          // Iterate over all the pages of the region
          for (uint p_index = 0; p_index < pagesInRegion; p_index++)
            {
              /* We must read the position before reading, as the pointer is
               * moved after the read operation
               */
              pageNumber = ftell(pagemap) / PM_ENTRY_BYTES;

              vm::pm_entry_t pagemap_entry;
              if (fread(&pagemap_entry, PM_ENTRY_BYTES, 1, pagemap) == 1) // if an entry was successfully read...
                {
                  vm::MemPage const &mp = vme.addPage(
                      vm::MemPage(pageNumber, pagemap_entry));
                  if (mp.isPresent())
                    {
                      pages.push_back(&mp);
                    }
                }
              else
                {
                  fatal("Error while reading /proc/self/pagemap");
                }
            }
          total += regionSize;
          entries.insert(std::pair<size_t, vm::VMEntry>(total, vme));
        }
    }

  return total;
}


bool
specific_block(void *addr)
{
 uintptr_t aligned_addr = ((uintptr_t) addr) & ~(page_size -1);
 return mprotect((void*)aligned_addr, page_size, PROT_NONE)==0;
}

bool
random_block(size_t range)
{
  static std::random_device rd;
  static std::mt19937 generator(rd());
  static std::uniform_int_distribution<int> distribution(0, pages.size());
  int dice_roll = distribution(generator); // generates random number between 0 and 'range'

  const vm::MemPage *corrupted_page = pages.at(dice_roll);
  void *addr = (void*) (corrupted_page->getPN() * page_size);
  debug("Mpoison: Blocking access to page 0x" << std::hex << corrupted_page->getPN()
      << ". Address: " << std::hex << addr);
  return mprotect(addr, page_size, PROT_NONE)==0;
}

void mpoison_run(void *arg)
{
  mpoison_init();
  bool success = false;

  size_t mem_size = collect();
  wait = false;
  debug("Done reading maps and pagemap files.");

  bool first_time = true;
  while( !getMyThreadSafe()->isRunning() ) {
    while(stop && getMyThreadSafe()->isRunning()) {}//wait until the other thread indicates 
                                                    //to continue the poisoning
    if(first_time || success){
      usleep(1000000);
      first_time = false;
    } else {
      debug("Block could not be potected");
    }

    if (mem_size > 0) {
      success = random_block(mem_size);
    }
  }
}

