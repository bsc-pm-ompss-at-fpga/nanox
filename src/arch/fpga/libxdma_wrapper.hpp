/*************************************************************************************/
/*      Copyright 2018 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_FPGA_LIBXDMA_WRAPPER
#define _NANOS_FPGA_LIBXDMA_WRAPPER

#include "libxdma.h"
#include "libxdma_version.h"

//! Check that libxdma version is compatible
#define LIBXDMA_MIN_MAJOR 1
#define LIBXDMA_MIN_MINOR 1
#if !defined(LIBXDMA_VERSION_MAJOR) || !defined(LIBXDMA_VERSION_MINOR) || \
    LIBXDMA_VERSION_MAJOR < LIBXDMA_MIN_MAJOR || \
    (LIBXDMA_VERSION_MAJOR == LIBXDMA_MIN_MAJOR && LIBXDMA_VERSION_MINOR < LIBXDMA_MIN_MINOR)
# error Installed libxdma is not supported (use >= 1.1)
#endif

#endif //_NANOS_FPGA_PROCESSOR_INFO
