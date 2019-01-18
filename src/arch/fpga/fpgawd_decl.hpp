/*************************************************************************************/
/*      Copyright 2019 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_FPGA_WD_DECL
#define _NANOS_FPGA_WD_DECL

#include "workdescriptor_decl.hpp"

namespace nanos {

   class FPGAWD : public WorkDescriptor
   {
      public:
         FPGAWD ( int ndevices, DeviceData **devs, size_t data_size = 0, size_t data_align = 1, void *wdata = 0,
                  size_t numCopies = 0, CopyData *copies = NULL, nanos_translate_args_t translate_args = NULL, const char *description = NULL );

         virtual void notifyParent();
   };

} // namespace nanos

#endif //_NANOS_FPGA_WD
