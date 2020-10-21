/*************************************************************************************/
/*      Copyright 2009-2019 Barcelona Supercomputing Center                          */
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

#ifndef _NANOS_FPGA_H
#define _NANOS_FPGA_H

#include "nanos-int.h"
#include "nanos_error.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    typedef struct {
        void (*outline) (void *);
        unsigned long long int type;
    } nanos_fpga_args_t;

    typedef struct {
        unsigned int type;
        bool check_free;
        bool lock_pe;
    } nanos_find_fpga_args_t;

    typedef enum {
        NANOS_COPY_HOST_TO_FPGA,
        NANOS_COPY_FPGA_TO_HOST
    } nanos_fpga_memcpy_kind_t;

    typedef enum {
        NANOS_ARGFLAG_DEP_OUT  = 0x08,
        NANOS_ARGFLAG_DEP_IN   = 0x04,
        NANOS_ARGFLAG_COPY_OUT = 0x02,
        NANOS_ARGFLAG_COPY_IN  = 0x01,
        NANOS_ARGFLAG_NONE     = 0x00
    } nanos_fpga_argflag_t;

    typedef struct __attribute__ ((__packed__)) {
        unsigned long long int address;
        unsigned char flags;
        unsigned char arg_idx;
        unsigned short _padding;
        unsigned int size;
        unsigned int offset;
        unsigned int accessed_length;
    } nanos_fpga_copyinfo_t;

    typedef void * nanos_fpga_task_t;

NANOS_API_DECL( void *, nanos_fpga_factory, ( void *args ) );
NANOS_API_DECL( void *, nanos_fpga_alloc_dma_mem, ( size_t len ) );
NANOS_API_DECL( void, nanos_fpga_free_dma_mem, ( void * address ) );
NANOS_API_DECL( nanos_err_t, nanos_find_fpga_pe, ( void *req, nanos_pe_t * pe ) );
NANOS_API_DECL( void *, nanos_fpga_get_phy_address, ( void * address ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_create_task, ( nanos_fpga_task_t *task, nanos_wd_t wd ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_create_periodic_task, ( nanos_fpga_task_t *task, nanos_wd_t wd, \
  const unsigned int period, const unsigned int num_reps ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_set_task_arg, ( nanos_fpga_task_t task, size_t argIdx, \
   bool isInput, bool isOutput, uint64_t argValue ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_submit_task, ( nanos_fpga_task_t task ) );
NANOS_API_DECL( void *, nanos_fpga_malloc, ( size_t len ) );
NANOS_API_DECL( void, nanos_fpga_free, ( void * fpgaPtr ) );
NANOS_API_DECL( void, nanos_fpga_memcpy, ( void * fpgaPtr, void * hostPtr, size_t len, \
   nanos_fpga_memcpy_kind_t kind ) );
NANOS_API_DECL( void, nanos_fpga_create_wd_async, ( const unsigned long long int type, \
  const unsigned char numArgs, const unsigned long long int * args, \
  const unsigned char numDeps, const unsigned long long int * deps, const unsigned char * depsFlags, \
  const unsigned char numCopies, const nanos_fpga_copyinfo_t * copies ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_register_wd_info, ( uint64_t type, size_t num_devices, \
  nanos_device_t * devices, nanos_translate_args_t translate ) );
NANOS_API_DECL( unsigned long long int, nanos_fpga_current_wd, ( void ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_wg_wait_completion, ( unsigned long long int uwg, unsigned char avoid_flush ) );
NANOS_API_DECL( unsigned long long int, nanos_fpga_get_time_cycle, ( void ) );
NANOS_API_DECL( unsigned long long int, nanos_fpga_get_time_us, ( void ) );

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_NANOS_FPGA_H
