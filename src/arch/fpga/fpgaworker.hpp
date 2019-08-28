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

#ifndef _FPGA_WORKER_DECL
#define _FPGA_WORKER_DECL

#include "workdescriptor.hpp"
#include "eventdispatcher_decl.hpp"
#include "nanos-fpga.h"

namespace nanos {
namespace ext {

   class FPGAWorker {
      friend class FPGAPlugin;
      public:
         typedef struct FPGARegisteredTask {
            size_t                 numDevices;
            nanos_device_t *       devices;
            nanos_translate_args_t translate;
            std::string            description;

            FPGARegisteredTask(size_t _numDevices, nanos_device_t * _devices, nanos_translate_args_t _translate, std::string _description) {
               this->translate = _translate;
               this->numDevices = _numDevices;
               this->description = _description;

               //NOTE: Using nanos_fpga_args_t as it is the largest device argument struct
               size_t allocSize = sizeof( nanos_device_t )*this->numDevices + sizeof( nanos_fpga_args_t )*this->numDevices;
               this->devices = ( nanos_device_t * )( malloc( allocSize ) );
               ensure( this->devices != NULL, " Cannot allocate memory for FPGARegisteredTask structure" );
               std::memcpy( this->devices, _devices, sizeof(nanos_device_t)*this->numDevices );

               //Update the argument pointer of each device
               nanos_fpga_args_t *args = ( nanos_fpga_args_t * )( this->devices + this->numDevices );
               for ( size_t i = 0; i < this->numDevices; ++i ) {
                  this->devices[i].arg = ( void * )( args + i );
                  std::memcpy( this->devices[i].arg, _devices[i].arg, sizeof( nanos_fpga_args_t ) );
               }
            }

            ~FPGARegisteredTask() {
               free( this->devices );
            }
         } FPGARegisteredTask;

         typedef TR1::unordered_map<uint64_t, FPGARegisteredTask *> FPGARegisteredTasksMap;

         static FPGARegisteredTasksMap *_registeredTasks; //!< Map of registered tasks
         static EventListener          *_createWdListener; //!< Pointer to the listener to be registered

      protected:
         /*! \brief Initializes the FPGARegisteredTasksMap
          *         Must be called one time before the first access to _registeredTasks var
          */
         static void initRegisteredTasksMap( FPGARegisteredTasksMap * map, EventListener * listener ) {
            ensure ( _registeredTasks == NULL, " Double call to initRegisteredTasksMap" );
            _registeredTasks = map;
            _createWdListener = listener;
         }

         /*! \breif Removes references to the FPGARegisteredTasksMap
         */
         static void finiRegisteredTasksMap() {
            _registeredTasks = NULL;
            _createWdListener = NULL;
         }

      public:
         //We should add some methods for configuration
         static void FPGAWorkerLoop();
         static bool tryOutlineTask( BaseThread * thread );
         static void handleFPGACreatedTasks();
         static WD * getFPGAWD( BaseThread *thread );
      private:

   };

} // namespace ext
} // namespace nanos

#endif
