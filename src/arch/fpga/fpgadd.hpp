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

      class FPGADD : public DD
      {

         private:
            int            _accNum; //! Accelerator that will run the task
            static std::vector < FPGADevice * > *_accDevices;

         public:
            // constructors
            FPGADD( work_fct w , int accNum ) :
                DD( (accNum < 0) ? (*_accDevices)[ 0 ] : (*_accDevices)[accNum], w ),
                _accNum( accNum )
            {
               ensure( getDevice() != NULL,
                       "Trying to use an unexisting FPGA Accelerator." );
            }

            // copy constructors
            FPGADD( const FPGADD &dd ) : DD( dd ), _accNum( dd._accNum ){}

            // assignment operator
            const FPGADD & operator= ( const FPGADD &wd );

            // destructor
            virtual ~FPGADD() { }

            static void init();

            virtual void lazyInit ( WD &wd, bool isUserLevelThread, WD *previous ) { }
            virtual size_t size ( void ) { return sizeof( FPGADD ); }
            virtual FPGADD *copyTo ( void *toAddr );
            virtual FPGADD *clone () const { return NEW FPGADD ( *this ); }
            virtual bool isCompatible ( const Device &arch );

            //virtual bool isCompatibleWithPE ( const ProcessingElement *pe=NULL );

            static void addAccDevice( FPGADevice *dev ) {
                _accDevices->push_back( dev );
            }
            static std::vector< FPGADevice* > *getAccDevices() {
               return _accDevices;
            }
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
