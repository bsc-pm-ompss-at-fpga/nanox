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

#ifndef _NANOS_FPGA_H
#define _NANOS_FPGA_H

#include "nanos-int.h"
#include "nanos_error.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    typedef struct {
        void (*outline) (void *);
        unsigned int acc_num;
    } nanos_fpga_args_t;

    typedef struct {
        unsigned int acc_num;
        bool check_free;
        bool lock_pe;
    } nanos_find_fpga_args_t;

    typedef uint64_t nanos_fpga_alloc_t;

    typedef enum {
        NANOS_COPY_HOST_TO_FPGA,
        NANOS_COPY_FPGA_TO_HOST
    } nanos_fpga_memcpy_kind_t;

NANOS_API_DECL( void *, nanos_fpga_factory, ( void *args ) );
NANOS_API_DECL( void *, nanos_fpga_alloc_dma_mem, ( size_t len) );
NANOS_API_DECL( void, nanos_fpga_free_dma_mem, ( void * address ) );
NANOS_API_DECL( nanos_err_t, nanos_find_fpga_pe, ( void *req, nanos_pe_t * pe ) );
NANOS_API_DECL( void *, nanos_fpga_get_phy_address, ( void * address ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_set_task_arg, ( nanos_wd_t wd, size_t argIdx, bool isInput, bool isOutput, uint64_t argValue ) );
NANOS_API_DECL( nanos_fpga_alloc_t, nanos_fpga_malloc, ( size_t len ) );
NANOS_API_DECL( void, nanos_fpga_free, ( nanos_fpga_alloc_t handle ) );
NANOS_API_DECL( void, nanos_fpga_memcpy, ( nanos_fpga_alloc_t handle, size_t offset, void * ptr, size_t len, nanos_fpga_memcpy_kind_t kind ) );

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_NANOS_FPGA_H
