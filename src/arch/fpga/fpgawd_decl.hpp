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

#ifndef _NANOS_FPGA_WD_DECL
#define _NANOS_FPGA_WD_DECL

#include "workdescriptor_decl.hpp"

namespace nanos {
namespace ext {

   class FPGAWD : public WorkDescriptor
   {
      private:
         // \brief Stores the addresses of copies received from the FPGA. Used to copy the data back
         std::vector<uint64_t> _origFpgaCopiesAddrs;

      public:
         FPGAWD ( int ndevices, DeviceData **devs, size_t data_size = 0, size_t data_align = 1, void *wdata = 0,
                  size_t numCopies = 0, CopyData *copies = NULL, nanos_translate_args_t translate_args = NULL, const char *description = NULL );

         void setOrigFpgaCopyAddr(const size_t idx, const uint64_t addr) { _origFpgaCopiesAddrs[idx] = addr; }

         virtual void notifyParent();
   };

} // namespace ext
} // namespace nanos

#endif //_NANOS_FPGA_WD
