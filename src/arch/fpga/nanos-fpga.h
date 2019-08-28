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

#ifndef _NANOS_FPGA_H
#define _NANOS_FPGA_H

#include "nanos-int.h"
#include "nanos_error.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    typedef struct {
        void (*outline) (void *);
        unsigned int type;
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
        uint64_t address;
        uint8_t  flags;
        uint8_t  arg_idx;
        uint16_t _padding;
        uint32_t size;
        uint32_t offset;
        uint32_t accessed_length;
    } nanos_fpga_copyinfo_t;

    enum {
       NANOS_FPGA_ARCH_SMP  = 0x800000,
       NANOS_FPGA_ARCH_FPGA = 0x400000
    };

NANOS_API_DECL( void *, nanos_fpga_factory, ( void *args ) );
NANOS_API_DECL( void *, nanos_fpga_alloc_dma_mem, ( size_t len) );
NANOS_API_DECL( void, nanos_fpga_free_dma_mem, ( void * address ) );
NANOS_API_DECL( nanos_err_t, nanos_find_fpga_pe, ( void *req, nanos_pe_t * pe ) );
NANOS_API_DECL( void *, nanos_fpga_get_phy_address, ( void * address ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_set_task_arg, ( nanos_wd_t wd, size_t argIdx, \
   bool isInput, bool isOutput, uint64_t argValue ) );
NANOS_API_DECL( void *, nanos_fpga_malloc, ( size_t len ) );
NANOS_API_DECL( void, nanos_fpga_free, ( void * fpgaPtr ) );
NANOS_API_DECL( void, nanos_fpga_memcpy, ( void * fpgaPtr, void * hostPtr, size_t len, \
   nanos_fpga_memcpy_kind_t kind ) );
NANOS_API_DECL( void, nanos_fpga_create_wd_async, ( uint32_t archMask, uint64_t type, \
  uint8_t numArgs, uint64_t * args, uint8_t numDeps, uint64_t * deps, uint8_t * depsFlags, \
  uint8_t numCopies, nanos_fpga_copyinfo_t * copies ) );
NANOS_API_DECL( nanos_err_t, nanos_fpga_register_wd_info, ( uint64_t type, size_t num_devices, \
  nanos_device_t * devices, nanos_translate_args_t translate ) );

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_NANOS_FPGA_H
