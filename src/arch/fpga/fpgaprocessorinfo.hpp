/*************************************************************************************/
/*      Copyright 2017 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_FPGA_PROCESSOR_INFO
#define _NANOS_FPGA_PROCESSOR_INFO

#include "debug.hpp"
#include "libxtasks.h"

namespace nanos {
namespace ext {

   typedef xtasks_acc_type FPGADeviceType;
   typedef xtasks_acc_id   FPGADeviceId;

   /*!
    * The only purpose of this class is to wrap some device dependent info dependeing
    * on an external library ir order to keep the system as clean as possible
    * (not having to include/define xtasks types in system, etc)
    */
   class FPGAProcessorInfo
   {
      private:
         xtasks_acc_handle _handle;   /// Low level accelerator handle
         xtasks_acc_info   _info;

      public:
         FPGAProcessorInfo( xtasks_acc_handle handle ) :
            _handle( handle )
         {
#if NANOS_DEBUG_ENABLED
            xtasks_stat sxt = xtasksGetAccInfo( _handle, &_info );
            ensure0( sxt == XTASKS_SUCCESS, "Cannot retrieve accelerator information" );
#else
            xtasksGetAccInfo( _handle, &_info );
#endif
         }

         xtasks_acc_handle getHandle() const {
            return _handle;
         }

         FPGADeviceType getType() const {
            return _info.type;
         }

         FPGADeviceId getId() const {
            return _info.id;
         }
   };
} // namespace ext
} // namespace nanos

#endif
