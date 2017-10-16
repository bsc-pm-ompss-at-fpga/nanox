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

#include "eventdispatcher_decl.hpp"
#include "fpgaprocessor.hpp"
#include "fpgaconfig.hpp"
#include "fpgaworker.hpp"

namespace nanos {
namespace ext {

class FPGAListener : public EventListener {
   private:
      /*!
       * FPGAProcessor that has to be represented when the callback is executed
       * This is needed to allow the SMPThread get ready FPGA WDs
       */
      FPGAProcessor          *_fpgaPE;
      Atomic<int>             _count;     //!< Counter of threads in the listener instance

      /*!
       * Returns the pointer to the FPGAProcessor associated to this listener
       */
      FPGAProcessor * getFPGAProcessor();
   public:
      /*!
       * \brief Default constructor
       * \param [in] pe          Pointer to the PE that the callback looks work for
       * \param [in] ownsThread  Flag that defines if pe is exclusively for the listener and
       *                         must be deleted with the listener
       */
      FPGAListener( FPGAProcessor * pe, const bool ownsPE = false );
      ~FPGAListener();
      void callback( BaseThread * thread );
};

FPGAListener::FPGAListener( FPGAProcessor * pe, const bool onwsPE ) : _count( 0 )
{
   union { FPGAProcessor * p; intptr_t i; } u = { pe };
   // Set the own status
   u.i |= int( onwsPE );
   _fpgaPE = u.p;
}

FPGAListener::~FPGAListener()
{
   union { FPGAProcessor * p; intptr_t i; } u = { _fpgaPE };
   bool deletePE = (u.i & 1);

   /*
   * The FPGAProcessor is only associated to the FPGAListener
   * The PE must be deleted before deleting the listener
   */
   if ( deletePE ) {
      FPGAProcessor * pe = getFPGAProcessor();
      delete pe;
   }
}

inline FPGAProcessor * FPGAListener::getFPGAProcessor()
{
   union { FPGAProcessor * p; intptr_t i; } u = { _fpgaPE };
   // Clear the own status if set
   u.i &= ((~(intptr_t)0) << 1);
   return u.p;
}

} /* namespace ext */
} /* namespace nanos */
