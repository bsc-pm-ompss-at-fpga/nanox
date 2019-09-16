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

#ifndef _NANOS_FPGA_LIBXTASKS_WRAPPER
#define _NANOS_FPGA_LIBXTASKS_WRAPPER

#include "libxtasks.h"
#include "libxtasks_version.h"

//! Check that libxtasks version is compatible
#define LIBXTASKS_MIN_MAJOR 7
#define LIBXTASKS_MIN_MINOR 3
#if !defined(LIBXTASKS_VERSION_MAJOR) || !defined(LIBXTASKS_VERSION_MINOR) || \
    LIBXTASKS_VERSION_MAJOR < LIBXTASKS_MIN_MAJOR || \
    (LIBXTASKS_VERSION_MAJOR == LIBXTASKS_MIN_MAJOR && LIBXTASKS_VERSION_MINOR < LIBXTASKS_MIN_MINOR)
# error Installed libxtasks is not supported (use >= 7.3)
#endif

#endif //_NANOS_FPGA_PROCESSOR_INFO
