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

#ifndef _NANOS_FPGA_DD
#define _NANOS_FPGA_DD

#include "fpgadevice.hpp"
#include "workdescriptor.hpp"

namespace nanos {
namespace ext {

   const FPGADevice &getFPGADevice(int i);

   class FPGADD : public DD
   {
      friend class FPGAPlugin;
      protected:
         //! \breif Map with the available accelerators types in the system
         static FPGADeviceMap     *_accDevices;

         /*! \brief Initializes the FPGADeviceMap to allow FPGADD creation
          *         Must be called one time before the first FPGADD creation
          */
         static void init( FPGADeviceMap * map ) {
            ensure ( _accDevices == NULL, "Double initialization of FPGADD static members" );
            _accDevices = map;
         }

         /*! \breif Removes references to the FPGADeviceMap
         */
         static void fini() {
            _accDevices = NULL;
         }

      public:
         static const FPGADevice &getNthDevice (unsigned int i)
         {
            FPGADeviceMap aux = *_accDevices;
            ensure( i<aux.size() , "Trying to use an unexisting FPGA Accelerator type. There's not enough number of types available." );

            unsigned int j=0;
            FPGADeviceMap::iterator it;
            for ( j=0, it = aux.begin(); j<i && it != aux.end(); j++, it++ );
            return *(it->second);
         }         

         // constructors
         FPGADD( work_fct w , FPGADeviceType const t ) : DD( (*_accDevices)[t], w ) {
#ifdef NANOS_DEBUG_ENABLED
            if( getDevice() == NULL ) {
               warning( "Creating a FPGADD with an unexisting FPGA Accelerator type: " << t );
            }
#endif
         }

         // copy constructors
         FPGADD( const FPGADD &dd ) : DD( dd ) { }

         // assignment operator
         const FPGADD & operator= ( const FPGADD &wd );

         // destructor
         virtual ~FPGADD() { }

         virtual void lazyInit ( WD &wd, bool isUserLevelThread, WD *previous ) { }
         virtual size_t size ( void ) { return sizeof( FPGADD ); }
         virtual FPGADD *copyTo ( void *toAddr );
         virtual FPGADD *clone () const { return NEW FPGADD ( *this ); }
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
