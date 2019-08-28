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

#include "eventdispatcher_decl.hpp"
#include "fpgaprocessor.hpp"
#include "fpgaconfig.hpp"
#include "fpgaworker.hpp"
#include "compatibility.hpp"

#include "nanos-fpga.h"

namespace nanos {
namespace ext {

class FPGAListener : public EventListener
{
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

class FPGACreateWDListener : public EventListener
{
   private:
      Atomic<int>             _count;     //!< Counter of threads in the listener instance

   public:
      FPGACreateWDListener();
      ~FPGACreateWDListener();
      void callback( BaseThread * thread );
};

#ifdef NANOS_INSTRUMENTATION_ENABLED
class FPGAInstrumentationListener : public EventListener
{
   public:
      typedef std::vector<FPGAProcessor *> FPGAPEsVector;

   private:
      FPGAPEsVector         *_fpgas;      //!< Vector of FPGAProcessors to handle
      Atomic<int>            _count;      //!< Counter of threads in the listener instance

   public:
      /*!
       * \brief Default constructor
       */
      FPGAInstrumentationListener();
      ~FPGAInstrumentationListener();
      /*!
       * \brief Setter of FPGA PEs vector
       * \param [in] pe          Pointer to the vector of PEs that the callback handles
       */
      void setFPGAPEsVector( FPGAPEsVector *pes );
      void callback( BaseThread * thread );
};
#endif //NANOS_INSTRUMENTATION_ENABLED

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

FPGACreateWDListener::FPGACreateWDListener() : _count( 0 )
{}

FPGACreateWDListener::~FPGACreateWDListener()
{}

#ifdef NANOS_INSTRUMENTATION_ENABLED
FPGAInstrumentationListener::FPGAInstrumentationListener() : _fpgas( NULL ), _count( 0 )
{}

FPGAInstrumentationListener::~FPGAInstrumentationListener()
{}

void FPGAInstrumentationListener::setFPGAPEsVector( std::vector<FPGAProcessor *> *pes ) {
   _fpgas = pes;
}
#endif //NANOS_INSTRUMENTATION_ENABLED

} /* namespace ext */
} /* namespace nanos */
