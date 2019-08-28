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

#ifndef _NANOS_FPGA_DD
#define _NANOS_FPGA_DD

#include "fpgadevice.hpp"
#include "workdescriptor.hpp"

namespace nanos {
namespace ext {

   class FPGADD : public DD
   {
      friend class FPGAPlugin;
      protected:
         //! \breif Task handle from xTasks library for the WD
         void const               *_xtasksHandle;

         //! \breif Map with the available accelerators types in the system
         static FPGADeviceMap     *_accDevices;

         /*! \brief Initializes the FPGADeviceMap to allow FPGADD creation
          *         Must be called one time before the first FPGADD creation
          */
         static void init( FPGADeviceMap * map ) {
            ensure ( _accDevices == NULL, " Double initialization of FPGADD static members" );
            _accDevices = map;
         }

         /*! \breif Removes references to the FPGADeviceMap
         */
         static void fini() {
            _accDevices = NULL;
         }

      public:
         // constructors
         FPGADD( work_fct w , FPGADeviceType const t ) : DD( (*_accDevices)[t], w ), _xtasksHandle( NULL ) {
#ifdef NANOS_DEBUG_ENABLED
            if( getDevice() == NULL ) {
               warning( "Creating a FPGADD with an unexisting FPGA Accelerator type: " << t );
            }
#endif
         }

         // copy constructors
         FPGADD( const FPGADD &dd ) : DD( dd ), _xtasksHandle( dd._xtasksHandle ) { }

         // assignment operator
         const FPGADD & operator= ( const FPGADD &wd );

         // destructor
         virtual ~FPGADD() { }

         virtual void lazyInit ( WD &wd, bool isUserLevelThread, WD *previous ) { }
         virtual size_t size ( void ) { return sizeof( FPGADD ); }
         virtual FPGADD *copyTo ( void *toAddr );
         virtual FPGADD *clone () const { return NEW FPGADD ( *this ); }

         // getter and setter for the xTasks handle
         void setHandle ( void const * handle ) { _xtasksHandle = handle; }
         void const * getHandle () { return _xtasksHandle; }

         static unsigned int getNumDevices() { return _accDevices->size(); }
         static FPGADeviceMap::iterator const getDevicesMapBegin () { return _accDevices->begin(); }
         static FPGADeviceMap::iterator const getDevicesMapEnd () { return _accDevices->end(); }
   };

   inline const FPGADD & FPGADD::operator= ( const FPGADD &dd )
   {
      // self-assignment: ok
      if ( &dd == this ) return *this;

      DD::operator= ( dd );

      return *this;
   }

} // namespace ext
} // namespace nanos

#endif
