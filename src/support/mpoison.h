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

#ifndef MPOISON_H
#define MPOISON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Initialize data structures used by mpoison
void mpoison_init ( void );

//! Clears data structures used by mpoison and waits the dedicated thread to finish
void mpoison_finalize ( void );

//! \brief Looks through /proc/pid/maps file to look for memory regions that can hold application data
void mpoison_scan ( void );

//! \brief Instead of scanning maps file, use user defined chunk list to inject failures.
void mpoison_declare_region ( uintptr_t addr, size_t size );

//! \brief Declare what is the region we would like faults to happen which typically is a small portion of the address space found by memory_scan.
void mpoison_declare_interested_memory_region (uintptr_t addr, size_t size);

//! \brief blocks the page starting at address page_addr. Returns 0 if no errors were found
int mpoison_block_page( uintptr_t);

//! \brief Unblocks the page starting at address page_addr. Returns 0 if no errors were found
int mpoison_unblock_page ( uintptr_t );

//! \brief Injects a bit flip in the address. The address is aligned.
int mpoison_inject_bit_flip_in_address( uintptr_t );

//! Makes the mpoison dedicated thread to stop randomized page blocking
void mpoison_stop ( void );

//! Makes the mpoison dedicated thread to stop randomized page blocking
void mpoison_continue ( void );

//! Makes the poison dedicated thread to begin randomized page blocking
void mpoison_start ( void );

void mpoison_delay_start ( unsigned long* );

void mpoison_set_fail( bool value );

void mpoison_do_fail();

int mpoison_should_fail();
#ifdef __cplusplus
}//extern C
#endif

#endif // MPOISON_H