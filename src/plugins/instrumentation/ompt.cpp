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

#include "plugin.hpp"
#include "system.hpp"
#include "instrumentation.hpp"
#include "instrumentationcontext_decl.hpp"
#include "ompt.h"

using namespace nanos;

extern "C" {

   namespace nanos {
      class InstrumentationOMPT;
      namespace ompt {
         Lock _lock;
         std::map<void *, int> map_parallel_id;
         ompt_parallel_id_t    count_parallel_id = 1;
      }
   }

   int ompt_initialize(
         ompt_function_lookup_t lookup,  /* function to look up OMPT API routines by name */
         const char *runtime_version,    /* OpenMP runtime version string */
         unsigned int ompt_version       /* integer that identifies the OMPT revision */
         );

   ompt_thread_id_t ompt_nanos_get_thread_id( void );

   int ompt_initialize( ompt_function_lookup_t lookup, const char *runtime_version, unsigned int ompt_version )
   {
      fatal0( "There is no OMPT compliant tool loaded\n" );
      return 0;
   }

   #define OMPT_NANOS_STATES 5
   ompt_state_t nanos_state_values[OMPT_NANOS_STATES] = { ompt_state_first, ompt_state_idle, ompt_state_work_serial, ompt_state_work_parallel, ompt_state_undefined };
   const char  *nanos_state_string[OMPT_NANOS_STATES] = { "First", "Idle", "Serial", "Parallel", "Undefined" };

   // Scheduler break point callback
   typedef void (*break_point_callback_t)( void );

   void breakPointCallBack(void);
   void breakPointCallBack(void) {
      // fprintf(stderr, "sched. step, thread %p\n", nanos::myThread);
      nanos::myThread->setSteps(1);
   }

   //! List of callback declarations
   ompt_parallel_begin_callback_t      ompt_nanos_event_parallel_begin = NULL;
   ompt_parallel_end_callback_t        ompt_nanos_event_parallel_end = NULL;
   ompt_task_begin_callback_t          ompt_nanos_event_task_begin = NULL;
   ompt_task_callback_t                ompt_nanos_event_task_end = NULL;
   ompt_parallel_callback_t            ompt_nanos_event_implicit_task_begin = NULL;
   ompt_parallel_callback_t            ompt_nanos_event_implicit_task_end = NULL;
   ompt_thread_type_callback_t         ompt_nanos_event_thread_begin = NULL;
   ompt_thread_type_callback_t         ompt_nanos_event_thread_end = NULL;
   ompt_control_callback_t             ompt_nanos_event_control = NULL;
   ompt_callback_t                     ompt_nanos_event_shutdown = NULL;
   ompt_task_pair_callback_t           ompt_nanos_event_task_switch = NULL;
   ompt_task_pair_callback_t           ompt_nanos_event_dependence = NULL;
   ompt_sync_callback_t                ompt_nanos_event_barrier_begin = NULL;
   ompt_parallel_callback_t            ompt_nanos_event_barrier_end = NULL;
   ompt_parallel_callback_t            ompt_nanos_event_taskwait_begin = NULL;
   ompt_parallel_callback_t            ompt_nanos_event_taskwait_end = NULL;


   int ompt_nanos_set_callback( ompt_event_t event, ompt_callback_t callback );
   int ompt_nanos_set_callback( ompt_event_t event, ompt_callback_t callback )
   {
      // Return values:
      // 0 callback registration error (e.g., callbacks cannot be registered at this time).
      // 1 event may occur; no callback is possible
      // 2 event will never occur in runtime
      // 3 event may occur; callback invoked when convenient
      // 4 event may occur; callback always invoked when event occurs

      switch ( event ) {
         case ompt_event_parallel_begin:
            ompt_nanos_event_parallel_begin = (ompt_parallel_begin_callback_t) callback;
            return 4;
         case ompt_event_parallel_end:
            ompt_nanos_event_parallel_end = (ompt_parallel_end_callback_t) callback;
            return 4;
         case ompt_event_task_begin:
            ompt_nanos_event_task_begin = (ompt_task_begin_callback_t) callback;
            return 4;
         case ompt_event_task_end:
            ompt_nanos_event_task_end = (ompt_task_callback_t) callback;
            return 4;
         case ompt_event_thread_begin:
            ompt_nanos_event_thread_begin = (ompt_thread_type_callback_t) callback;
            return 4;
         case ompt_event_thread_end:
            ompt_nanos_event_thread_end = (ompt_thread_type_callback_t) callback;
            return 4;
         case ompt_event_control:
            //FIXME Consider to instrument user control calls
            ompt_nanos_event_control = (ompt_control_callback_t) callback;
            return 1;
         case ompt_event_runtime_shutdown:
            ompt_nanos_event_shutdown = (ompt_callback_t) callback;
            return 4;
         case ompt_event_idle_begin:
         case ompt_event_idle_end:
            return 1;
         case ompt_event_wait_barrier_begin:
         case ompt_event_wait_barrier_end:
            return 1;
         case ompt_event_barrier_begin:
            ompt_nanos_event_barrier_begin = (ompt_sync_callback_t) callback;
            return 4;
         case ompt_event_barrier_end:
            ompt_nanos_event_barrier_end = (ompt_parallel_callback_t) callback;
            return 4;
         case ompt_event_implicit_task_begin:
            ompt_nanos_event_implicit_task_begin = (ompt_parallel_callback_t) callback;
            return 4;
         case ompt_event_implicit_task_end:
            ompt_nanos_event_implicit_task_end = (ompt_parallel_callback_t) callback;
            return 4;
         case ompt_event_wait_taskwait_begin:
         case ompt_event_wait_taskwait_end:
         case ompt_event_wait_taskgroup_begin:
         case ompt_event_wait_taskgroup_end:
         case ompt_event_release_lock:
         case ompt_event_release_nest_lock_last:
         case ompt_event_release_critical:
         case ompt_event_release_atomic:
         case ompt_event_release_ordered:
         case ompt_event_initial_task_begin:
         case ompt_event_initial_task_end:
            return 1;
         case ompt_event_task_switch:
            ompt_nanos_event_task_switch = (ompt_task_pair_callback_t) callback;
            return 4;
         case ompt_event_loop_begin:
         case ompt_event_loop_end:
         case ompt_event_sections_begin:
         case ompt_event_sections_end:
         case ompt_event_single_in_block_begin:
         case ompt_event_single_in_block_end:
         case ompt_event_single_others_begin:
         case ompt_event_single_others_end:
         case ompt_event_workshare_begin:
         case ompt_event_workshare_end:
         case ompt_event_master_begin:
         case ompt_event_master_end:
            return 1;
         case ompt_event_taskwait_begin:
            ompt_nanos_event_taskwait_begin = (ompt_parallel_callback_t) callback;
            return 4;
         case ompt_event_taskwait_end:
            ompt_nanos_event_taskwait_end = (ompt_parallel_callback_t) callback;
            return 4;
         case ompt_event_taskgroup_begin:
         case ompt_event_taskgroup_end:
         case ompt_event_release_nest_lock_prev:
         case ompt_event_wait_lock:
         case ompt_event_wait_nest_lock:
         case ompt_event_wait_critical:
         case ompt_event_wait_atomic:
         case ompt_event_wait_ordered:
         case ompt_event_acquired_lock:
         case ompt_event_acquired_nest_lock_first:
         case ompt_event_acquired_nest_lock_next:
         case ompt_event_acquired_critical:
         case ompt_event_acquired_atomic:
         case ompt_event_acquired_ordered:
            return 1;
         case ompt_event_task_dependence_pair:
            ompt_nanos_event_dependence = (ompt_task_pair_callback_t) callback;
            return 4;
         case ompt_event_task_dependences:
            return 1;
         default:
            warning0("Callback registration error: incorrect event id");
            return 0;
      }

   }

   int ompt_nanos_get_callback( ompt_event_t event, ompt_callback_t *callback );
   int ompt_nanos_get_callback( ompt_event_t event, ompt_callback_t *callback )
   {
      // FIXME: TBD
      return 0;
   }

   int ompt_nanos_enumerate_state( ompt_state_t current_state, ompt_state_t *next_state, const char **next_state_name );
   int ompt_nanos_enumerate_state( ompt_state_t current_state, ompt_state_t *next_state, const char **next_state_name )
   {
      int i;

      for ( i = 0; i < OMPT_NANOS_STATES; i++) {
         if ( nanos_state_values[i] == current_state ) break;
      }

      if ( ++i < OMPT_NANOS_STATES ) {
         *next_state = nanos_state_values[i];
         *next_state_name = nanos_state_string[i];
         return 1;
      } else {
         return 0;
      }
   }

   ompt_state_t ompt_nanos_get_state( ompt_wait_id_t *wait_id );
   ompt_state_t ompt_nanos_get_state( ompt_wait_id_t *wait_id )
   {
      //! \note This function must be ordered acording with state genericity
      //! specific states must be first to be detected, so the first state that fits
      //! with curret thread state must be returned.


      //! \note If thread state is idle, return idle
      if ( myThread->isIdle() ) return ompt_state_idle;

      //! \note If we consider OmpSs running always in parallel there is only when
      //! case in which we can run serially: first level team, running with one thread.
      ThreadTeam *tt = myThread->getTeam();
      if ( tt && tt->size() == 1 && tt->getLevel() == 0 )
         return ompt_state_work_serial;
      else if ( tt )
         return ompt_state_work_parallel;

      //! \note Otherwise return undefined
      return ompt_state_undefined;
   }

   void * ompt_nanos_get_idle_frame(void);
   void * ompt_nanos_get_idle_frame(void)
   {
      // FIXME: TBD
      return NULL;
   }

   ompt_parallel_id_t ompt_nanos_get_parallel_id( int ancestor_level );
   ompt_parallel_id_t ompt_nanos_get_parallel_id( int ancestor_level )
   {

      ompt_parallel_id_t rv = 0;

      ThreadTeam *tt = myThread->getTeam();
      while ( ancestor_level > 0 && tt != NULL ) {
         tt = tt->getParent();
         ancestor_level--;
      }
      if ( tt != NULL ) {

         nanos::ompt::_lock.acquire();
         if ( nanos::ompt::map_parallel_id.find((void *) tt ) != nanos::ompt::map_parallel_id.end() )
            rv = nanos::ompt::map_parallel_id[(void *) tt];
         else
            rv = (ompt_parallel_id_t) 0;
         nanos::ompt::_lock.release();
      }

      return rv;
   }

   int ompt_nanos_get_parallel_team_size( int ancestor_level );
   int ompt_nanos_get_parallel_team_size( int ancestor_level )
   {
      ThreadTeam *tt = myThread->getTeam();
      while ( ancestor_level > 0 && tt != NULL ) {
         tt = tt->getParent();
         ancestor_level--;
      }
      if ( tt ) return (int) tt->size();
      return (int) 0;
   }

   ompt_task_id_t ompt_nanos_get_task_id( int depth );
   ompt_task_id_t ompt_nanos_get_task_id( int depth )
   {
      WorkDescriptor *wd = myThread->getCurrentWD();
      while ( depth > 0 && wd != NULL ) {
         wd = wd->getParent();
         depth--;
      }
      if ( wd ) return (ompt_task_id_t) wd->getId();

      return (ompt_task_id_t) 0;
   }

   ompt_frame_t *ompt_nanos_get_task_frame( int depth );
   ompt_frame_t *ompt_nanos_get_task_frame( int depth )
   {
      // FIXME: TBD
      return NULL;
   }


   static ompt_target_id_t ompt_nanos_get_target_id( void );
   static ompt_target_id_t ompt_nanos_get_target_id( void )
   {
      return 0;
   }

   static int ompt_nanos_get_num_devices( void )
   {
      return sys.getNumInstrumentAccelerators();
   }

   //! Return the ID of the active device
   static int ompt_nanos_target_get_device_id( void )
   {
      return 0;
   }

   //Declarations to be used in lookup function
   int ompt_nanos_target_get_device_info( int32_t device_id, const char **type,
         ompt_target_device_t **device, ompt_function_lookup_t *lookup,
         const char *documentation );
   int ompt_nanos_target_start_trace( ompt_target_device_t *device,
         ompt_target_buffer_request_callback_t request,
         ompt_target_buffer_complete_callback_t );
   int ompt_nanos_target_advance_buffer_cursor( ompt_target_buffer_t *buffer,
         ompt_target_buffer_cursor_t current,
         ompt_target_buffer_cursor_t *next );
   ompt_interface_fn_t ompt_nanos_target_lookup ( const char *entry_point );
   ompt_interface_fn_t ompt_nanos_lookup ( const char *entry_point );

   ompt_interface_fn_t ompt_nanos_lookup ( const char *entry_point )
   {
      if ( strncmp( entry_point, "ompt_set_callback", strlen("ompt_set_callback") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_set_callback;
      if ( strncmp( entry_point, "ompt_get_callback", strlen("ompt_get_callback") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_callback;
      if ( strncmp( entry_point, "ompt_enumerate_state", strlen("ompt_enumerate_state") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_enumerate_state;
      if ( strncmp( entry_point, "ompt_get_thread_id", strlen("ompt_get_thread_id") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_thread_id;
      if ( strncmp( entry_point, "ompt_get_state", strlen("ompt_get_state") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_state;
      if ( strncmp( entry_point, "ompt_get_idle_frame", strlen("ompt_get_idle_frame") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_idle_frame;
      if ( strncmp( entry_point, "ompt_get_parallel_id", strlen("ompt_get_parallel_id") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_parallel_id;
      if ( strncmp( entry_point, "ompt_get_parallel_team_size", strlen("ompt_get_parallel_team_size") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_parallel_team_size;
      if ( strncmp( entry_point, "ompt_get_task_id", strlen("ompt_get_task_id") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_task_id;
      if ( strncmp( entry_point, "ompt_get_task_frame", strlen("ompt_get_task_frame") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_task_frame;

      //Target
      if ( strncmp( entry_point, "ompt_get_target_id", strlen("ompt_get_target_id") )  == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_target_id; //TODO
      if ( strncmp( entry_point, "ompt_get_num_devices", strlen("ompt_get_num_devices") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_get_num_devices;     //TODO
      if ( strncmp( entry_point, "ompt_target_get_device_id", strlen("ompt_target_get_device_id") ) == 0 )
         return (ompt_interface_fn_t ) ompt_nanos_target_get_device_id;       //TODO
      if ( strncmp( entry_point, "ompt_target_get_device_info", strlen("ompt_target_get_device_info") ) == 0 )
         return ( ompt_interface_fn_t ) ompt_nanos_target_get_device_info;    //TODO

      return (NULL);
   }


   static ompt_target_time_t ompt_nanos_target_get_time( ompt_target_device_t *device )
   {
      DeviceInstrumentation *devInstr = ( DeviceInstrumentation * ) device;
      return (ompt_target_time_t)devInstr->getDeviceTime();
   }

   static double ompt_nanos_target_translate_time( ompt_target_device_t *device,
         ompt_target_time_t time)
   {
      DeviceInstrumentation *devInstr = ( DeviceInstrumentation * ) device;
      return (double) devInstr->translateDeviceTime( time );
   }

   //! Enables or disables individual ompt events
   static int ompt_nanos_target_set_trace_ompt( ompt_target_device_t *device, _Bool enable,
         uint32_t flags )
   {
      //Current implementation will always enable all events
      return 0;   //Success
   }

   static int ompt_nanos_target_set_trace_native( ompt_target_device_t *device, _Bool enable,
         ompt_record_type_t rtype )
   {
      warning( "OMPT native instrumentation is currently not supported" );
      return -1; //Fail
   }


   static int ompt_nanos_target_pause_trace( ompt_target_device_t *device, _Bool begin_pause )
   {
      ( ( DeviceInstrumentation * )device )->pauseDeviceTrace( begin_pause );
      return 0;
   }

   int ompt_nanos_target_stop_trace ( ompt_target_device_t *device );


  static ompt_record_type_t ompt_nanos_target_buffer_get_record_type( ompt_target_buffer_t *buffer, ompt_target_buffer_cursor_t current)
  {
     //Only ompt records are supported now
     return ompt_record_ompt;
  }

  static ompt_record_ompt_t *ompt_nanos_target_buffer_get_record_ompt(
        ompt_target_buffer_t *buffer,
        ompt_target_buffer_cursor_t current )
  {
     ompt_record_ompt_t *record = ( ompt_record_ompt_t* )buffer;
     return &record[ current ];
  }

  static void *ompt_nanos_target_buffer_get_record_native( ompt_target_buffer_cursor_t *buffer,
        ompt_target_buffer_cursor_t current, ompt_target_id_t *host_op_id )
  {
     warning( "Trying to get OMPT native record" );
     return 0;
  }

  static ompt_record_native_abstract_t *ompt_nanos_target_buffer_get_record_native_abstract(
        void *native_record )
  {
     warning( "Trying to get OMPT abstract native record" );
     return ( ompt_record_native_abstract_t* )NULL;
  }

   /*!
    * Lookup function that will manage target (device) related functions
    * A single target lookup function is used for any device
    */
   ompt_interface_fn_t ompt_nanos_target_lookup ( const char *entry_point )
   {
      //TODO: Use a smarter way to look for a function
      //Trace control
      if ( strncmp( entry_point, "ompt_target_get_time", strlen("ompt_target_get_time") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_get_time;
      if ( strncmp( entry_point, "ompt_target_translate_time", strlen("ompt_target_translate_time") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_translate_time;
      if ( strncmp( entry_point, "ompt_target_set_trace_ompt", strlen("ompt_target_set_trace_ompt") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_set_trace_ompt;
      if ( strncmp( entry_point, "ompt_target_set_trace_native", strlen("ompt_target_set_trace_native") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_set_trace_native;
      if ( strncmp( entry_point, "ompt_target_start_trace", strlen("ompt_target_start_trace") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_start_trace;
      if ( strncmp( entry_point, "ompt_target_pause_trace", strlen("ompt_target_pause_trace") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_pause_trace;
      if ( strncmp( entry_point, "ompt_target_stop_trace", strlen("ompt_target_stop_trace") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_stop_trace;
      //Buffer control
      if ( strncmp( entry_point, "ompt_target_advance_buffer_cursor", strlen("ompt_target_advance_buffer_cursor") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_advance_buffer_cursor;
      if ( strncmp( entry_point, "ompt_target_buffer_get_record_type", strlen("ompt_target_buffer_get_record_type") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_buffer_get_record_type;
      if ( strncmp( entry_point, "ompt_target_buffer_get_record_ompt", strlen("ompt_target_buffer_get_record_ompt") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_buffer_get_record_ompt;
      if ( strncmp( entry_point, "ompt_target_buffer_get_record_native", strlen("ompt_target_buffer_get_record_native") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_buffer_get_record_native;
      if ( strncmp( entry_point, "ompt_target_buffer_get_record_native_abstract", strlen("ompt_target_buffer_get_record_native_abstract") ) == 0 )
         return (ompt_interface_fn_t) ompt_nanos_target_buffer_get_record_native_abstract;
      return NULL;
   }
}

namespace nanos
{
   class InstrumentationOMPT: public Instrumentation
   {
      private:
         ompt_task_id_t * _previousTask;
         int            * _threadActive;
         int              _numThreads;

         //target buffer callbacks
         //TODO these should be inside a vector, per accelerator
         ompt_target_buffer_request_callback_t     _requestBufferCallback;
         ompt_target_buffer_complete_callback_t    _completeBufferCallback;

         struct BufferInfo {
            Lock lock;
            Atomic<unsigned int> done;   ///< Counter of positions full filled in the buffer
            unsigned int begin;
            unsigned int current;        ///< Next position to be full filled
            size_t size;
            unsigned int records;
            ompt_record_ompt_t *buffer;
            BufferInfo() : begin( 0 ), current( 0 ), size( 0 ), records( 0 ),
               buffer( NULL ) { }
         };

         std::vector < BufferInfo* > _devEventBuffers;

      public:
         InstrumentationOMPT( ) : Instrumentation( *NEW InstrumentationContextDisabled()),
            _previousTask( NULL ), _threadActive( NULL ), _requestBufferCallback( NULL ),
            _completeBufferCallback( NULL ) {}
         ~InstrumentationOMPT() {
            for ( std::vector< BufferInfo* >::iterator it = _devEventBuffers.begin();
                  it != _devEventBuffers.end();
                  it++ ) {
               delete *it;
            }
         }

         static int getCurrentThreadId() {
            /*! NOTE: The thread ids may not be consecutive as each MultiThread uses several identifies.
                      However all sub-threads use the osId of their parent.
             */
            return myThread->getOsId();
         }

         void initialize( void )
         {
            ompt_initialize ( ompt_nanos_lookup, "Nanos++ 0.15a", 1);
            int nthreads = sys.getSMPPlugin()->getNumThreads();
            _numThreads = nthreads;
            _previousTask = ( ompt_task_id_t *) malloc ( nthreads * sizeof(ompt_task_id_t) );
            _threadActive = ( int *) malloc ( nthreads * sizeof(int) );
            for ( int i = 0; i < nthreads; i++ ){
               _previousTask[i] = (ompt_task_id_t) 0;
               _threadActive[i] = 0;
            }

            // initialize() cannot reference myThead object
            if (ompt_nanos_event_thread_begin) {
               ompt_nanos_event_thread_begin( (ompt_thread_type_t) ompt_thread_initial, (ompt_thread_id_t) 0);
            }
            _threadActive[0] = 1;
         }
         void finalize( void )
         {
            if (ompt_nanos_event_thread_end) {
               ompt_nanos_event_thread_end((ompt_thread_type_t) ompt_thread_initial, (ompt_thread_id_t) getCurrentThreadId());
            }
            if ( ompt_nanos_event_shutdown ) ompt_nanos_event_shutdown();
            free( _threadActive );
            free( _previousTask );
         }
         void disable( void ) {}
         void enable( void ) {}
         void addEventList ( unsigned int count, Event *events )
         {
            InstrumentationDictionary *iD = getInstrumentationDictionary();
            static const nanos_event_key_t create_wd_ptr = iD->getEventKey("create-wd-ptr");
            static const nanos_event_key_t set_num_threads = iD->getEventKey("set-num-threads");

            static const nanos_event_key_t api = iD->getEventKey("api");
            static const nanos_event_value_t api_create_team = iD->getEventValue("api","create_team");
            static const nanos_event_value_t api_end_team = iD->getEventValue("api","end_team");
            static const nanos_event_value_t api_barrier = iD->getEventValue("api","omp_barrier");
            static const nanos_event_value_t api_enter_team = iD->getEventValue("api","enter_team");
            static const nanos_event_value_t api_leave_team = iD->getEventValue("api","leave_team");
            static const nanos_event_value_t api_taskwait = iD->getEventValue("api","wg_wait_completion");

            static const nanos_event_key_t parallel_outline = iD->getEventKey("parallel-outline-fct");
            static const nanos_event_key_t team_info = iD->getEventKey("team-ptr");
            static const nanos_event_key_t dependence =  iD->getEventKey("dependence");
            //static const nanos_event_key_t dep_direction = iD->getEventKey("dep-direction");
            //static const nanos_event_key_t dep_address = iD->getEventKey("dep-address");

            unsigned int i;
            for( i=0; i<count; i++) {
               Event &e = events[i];
// XXX: debug information
#if 0
               int thid = nanos::myThread? getCurrentThreadId():0;
               fprintf(stderr,"NANOS++ [%d]: (%d/%d) event %ld value %lu\n",
                     thid,
                     (int)i+1,
                     (int)count,
                     (long) e.getKey(),
                     (unsigned long) e.getValue()
                     );
#endif
               switch ( e.getType() ) {
                  case NANOS_POINT:
                     if ( e.getKey( ) == create_wd_ptr && ompt_nanos_event_task_begin )
                     {
                        WorkDescriptor *wd = (WorkDescriptor *) e.getValue();
                        if ( !wd->isImplicit() ) {
                           //Add an event for each task implementation
                           for ( unsigned int j = 0; j < wd->getNumDevices(); j++) {
                              ompt_nanos_event_task_begin(
                                    (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId(),
                                    NULL,  // FIXME: task frame
                                    (ompt_task_id_t) wd->getId(),
                                    (void*)wd->getDevices()[j]->getWorkFct()
                                    );
                           }
                        }
                     }
                     if ( e.getKey() == dependence && ompt_nanos_event_dependence ) {
                        nanos_event_value_t dependence_value = e.getValue();
                        int sender_id = (int) ( dependence_value >> 32 ) & 0xFFFFFFFF;
                        int receiver_id = (int) ( dependence_value & 0xFFFFFFFF );

                        ompt_nanos_event_dependence(
                           (ompt_task_id_t) sender_id,
                           (ompt_task_id_t) receiver_id
                        );
                     }
                     break;
                  case NANOS_BURST_START:
                     if ( e.getKey( ) == api )
                     {
                        nanos_event_value_t val = e.getValue();

                        // getting current team id
                        ThreadTeam *tt = myThread->getTeam();
                        ompt_parallel_id_t team_id = 0;
                        if ( tt != NULL ) {
                           nanos::ompt::_lock.acquire();
                           if ( nanos::ompt::map_parallel_id.find((void *) tt ) != nanos::ompt::map_parallel_id.end() )
                              team_id = nanos::ompt::map_parallel_id[(void *) tt];
                           nanos::ompt::_lock.release();
                        }

                        if ( val == api_end_team && ompt_nanos_event_parallel_end ) {
                           void *team_ptr = NULL;
                           ompt_parallel_id_t p_id = 0;
                           while ( i < count ) {
                              Event &e1 = events[++i];
                              if ( e1.getKey() == team_info) {
                                 team_ptr = (void *) e1.getValue();
                                 nanos::ompt::_lock.acquire();
                                 if ( nanos::ompt::map_parallel_id.find((void *) team_ptr ) != nanos::ompt::map_parallel_id.end() ) {
                                    p_id = nanos::ompt::map_parallel_id[(void *) team_ptr];
                                 }
                                 nanos::ompt::_lock.release();
                                 break;
                              }
                           }
                           ompt_nanos_event_parallel_end (
                                 (ompt_parallel_id_t) p_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId(),
                                 ompt_invoker_runtime);
                        } else if ( val == api_barrier && ompt_nanos_event_barrier_begin ) {
                           ompt_nanos_event_barrier_begin (
                                 (ompt_parallel_id_t) team_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId(),
                                 NULL);
                        } else if ( val == api_leave_team && ompt_nanos_event_implicit_task_end ) {
                           ompt_nanos_event_implicit_task_end (
                                 (ompt_parallel_id_t) team_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId() );
                        } else if ( val == api_taskwait && ompt_nanos_event_taskwait_begin ) {
                           ompt_nanos_event_taskwait_begin (
                                 (ompt_parallel_id_t) team_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId() );
                        }

                     }
                     break;
                  case NANOS_BURST_END:
                     if ( e.getKey( ) == api )
                     {
                        nanos_event_value_t val = e.getValue();

                        // getting current team id
                        ThreadTeam *tt = myThread->getTeam();
                        ompt_parallel_id_t team_id = 0;
                        if ( tt != NULL ) {
                           nanos::ompt::_lock.acquire();
                           if ( nanos::ompt::map_parallel_id.find((void *) tt ) != nanos::ompt::map_parallel_id.end() )
                              team_id = nanos::ompt::map_parallel_id[(void *) tt];
                           nanos::ompt::_lock.release();
                        }

                        if ( val == api_barrier && ompt_nanos_event_barrier_end ) {
                           ompt_nanos_event_barrier_end (
                                 (ompt_parallel_id_t) team_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId() );
                        } else if ( val == api_create_team && ompt_nanos_event_parallel_begin ) {
                           uint32_t team_size = 0;
                           void *parallel_fct = NULL;
                           void *team_ptr = NULL;
                           ompt_parallel_id_t p_id = 0;

                           while ( i < count ) {
                              Event &e1 = events[++i];
                              if ( e1.getKey() == set_num_threads ) {
                                 team_size = (uint32_t) e1.getValue();
                              }
                              else if ( e1.getKey() == parallel_outline ) {
                                 parallel_fct = (void *) e1.getValue();
                              } else if ( e1.getKey() == team_info) {
                                 team_ptr = (void *) e1.getValue();
                                 nanos::ompt::_lock.acquire();
                                 if ( nanos::ompt::map_parallel_id.find((void *) team_ptr ) == nanos::ompt::map_parallel_id.end() ) {
                                    p_id = nanos::ompt::count_parallel_id++;
                                    nanos::ompt::map_parallel_id[(void *) team_ptr] = p_id;
                                 }
                                 nanos::ompt::_lock.release();
                                 break;
                              }
                           }

                           ompt_frame_t cb_frame;
                           cb_frame.exit_runtime_frame = NULL; // FIXME: frame data of parent task
                           cb_frame.reenter_runtime_frame = NULL; // FIXME: as ^^^

                           ompt_nanos_event_parallel_begin (
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId(),
                                 &cb_frame,
                                 (ompt_parallel_id_t) p_id,
                                 (uint32_t) team_size,
                                 (void *) parallel_fct,
                                 ompt_invoker_runtime
                                 );
                        } else if ( val == api_enter_team && ompt_nanos_event_implicit_task_begin ) {
                           ompt_nanos_event_implicit_task_begin (
                                 (ompt_parallel_id_t) team_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId() );
                        } else if ( val == api_taskwait && ompt_nanos_event_taskwait_end ) {
                           ompt_nanos_event_taskwait_end (
                                 (ompt_parallel_id_t) team_id,
                                 (ompt_task_id_t) nanos::myThread->getCurrentWD()->getId() );
                        }
                     }
                     break;
                  case NANOS_STATE_START:
                  case NANOS_STATE_END:
                  case NANOS_SUBSTATE_START:
                  case NANOS_SUBSTATE_END:
                  case NANOS_PTP_START:
                  case NANOS_PTP_END:
                  case EVENT_TYPES:
                     break;
                  default:
                     break;
               }
            }
         }
         void addResumeTask( WorkDescriptor &w )
         {
            if ( !ompt_nanos_event_task_switch ) return;

            ompt_task_id_t post = (ompt_task_id_t) w.getId();

            int thid = (int) getCurrentThreadId();
            ensure( thid < _numThreads, "Wrong thid in addResumeTask" );

            if (!_threadActive[thid]) return;

            ompt_task_id_t pre = (ompt_task_id_t) _previousTask[thid];

            ompt_nanos_event_task_switch ( pre, post );
         }
         void addSuspendTask( WorkDescriptor &w, bool last )
         {
            int thid = (int) getCurrentThreadId();
            ensure( thid < _numThreads, "Wrong thid in addSuspendTask" );

            if (last) _previousTask[thid] = (ompt_task_id_t) 0;
            else _previousTask[thid] = (ompt_task_id_t) w.getId();

            if (ompt_nanos_event_task_end && last) {
               ompt_nanos_event_task_end((ompt_task_id_t) w.getId());
            }
         }
         void threadStart( BaseThread &thread )
         {
            // Setting break point
            thread.setSteps (1);
            thread.setCallBack ( breakPointCallBack );

            int thid = getCurrentThreadId();

            if (ompt_nanos_event_thread_begin) {
               ompt_nanos_event_thread_begin( (ompt_thread_type_t) ompt_thread_worker, (ompt_thread_id_t) thid );
            }
            _threadActive[thid] = 1;
         }
         void threadFinish ( BaseThread &thread )
         {
            if (ompt_nanos_event_thread_end) {
               ompt_nanos_event_thread_end((ompt_thread_type_t) ompt_thread_worker, (ompt_thread_id_t) getCurrentThreadId());
            }
         }
         void incrementMaxThreads( void ) {
            _numThreads++;
            _previousTask = ( ompt_task_id_t* ) realloc( _previousTask, _numThreads*sizeof( ompt_task_id_t ) );
            _threadActive = ( int* ) realloc( _threadActive, _numThreads*sizeof( int ) );
         }

         void setRequestBufferCallback( ompt_target_buffer_request_callback_t callback )
         {
            _requestBufferCallback = callback;
         }

         void setCompleteBufferCallback ( ompt_target_buffer_complete_callback_t callback )
         {
            _completeBufferCallback = callback;
         }

         DeviceInstrumentation *getDeviceInstrumentation( int id )
         {
            return sys.getDeviceInstrumentation( id );
         }

         int advanceBuffer( ompt_target_buffer_t targetBuffer,
               ompt_target_buffer_cursor_t current,
               ompt_target_buffer_cursor_t *next )
         {
            //Get the right buffer
            //Use naive linear search, as we usually have a small number of accelerators
            //this will be enough, even faster than using a more complex structure
            BufferInfo *buffer = NULL;
            for ( std::vector< BufferInfo* >::iterator it = _devEventBuffers.begin();
                  it != _devEventBuffers.end();
                  it++ ) {
               if ( (*it)->buffer == ( ompt_record_ompt_t *)targetBuffer ) {
                  buffer = *it;
                  break;
               }
            }
            ensure( buffer != NULL,
                  "Could not get event buffer information for current buffer");
            *next = current + 1;
            if ( current >= buffer->records ) {
               return 1;
            } else {
               return 0;
            }
         }

         virtual void addDeviceEventList( const DeviceInstrumentation& ctx, unsigned int count, DeviceEvent *events )
         {
            int deviceId = ctx.getId();
            BufferInfo &buffer = *_devEventBuffers[ deviceId ];
            unsigned int eventIdx = 0; //< Index being handled of events array
            ompt_thread_id_t threadId = ( ompt_thread_id_t )( getCurrentThreadId() );

            while ( eventIdx < count ) {
               buffer.lock++; //Lock the buffer to get an empty slot

               //Request buffer if it's not initialized
               if ( buffer.buffer == NULL ) {
                  //buffer request
                  _requestBufferCallback( (ompt_target_buffer_t**)&buffer.buffer, &buffer.size );
                  buffer.done = 0;
                  buffer.begin = 0;
                  buffer.current = 0;
                  buffer.records = buffer.size / sizeof( ompt_record_ompt_t );
               }

               //Call to complete if no more records fit in current buffer
               if ( buffer.current == buffer.records ) {
                  //Wait until last writes end
                  while ( buffer.done.value() != buffer.records ) {}

                  //There is no space to save the event -> call completion
                  _completeBufferCallback( deviceId, ( ompt_target_buffer_t *) buffer.buffer,
                     buffer.size, buffer.begin, buffer.current );
                  //reset current pointer to recycle the buffer
                  buffer.done = buffer.begin;
                  buffer.current = buffer.begin;
               }
               unsigned int current = buffer.current; //< Initial position to write in the buffer
               //! NOTE: Each input event generate two OMPT events:
               //!        - NANOS_BURST_START: ompt_event_task_begin + ompt_event_task_switch
               //!        - NANOS_BURST_END: ompt_event_task_switch + ompt_event_task_end
               unsigned int countCurrent = std::min( ( count - eventIdx )*2, buffer.records - current );
               buffer.current += countCurrent;
               buffer.lock--; //Release the lock and use local variable

               for (unsigned int i = 0; i < countCurrent; i += 2) {
                  //add event to buffer
                  nanos_event_value_t eventValue = events[eventIdx].getValue();
                  nanos_event_time_t eventTime = events[eventIdx].getDeviceTime();
                  switch ( events[eventIdx].getType() ) {
                     case NANOS_BURST_START: {
                        //set the ompt_event_task_begin information
                        ompt_record_ompt_t *burstEvent = &buffer.buffer[ current + i ];
                        burstEvent->type = ompt_event_task_begin;
                        burstEvent->time = eventTime;
                        burstEvent->thread_id = threadId;
                        burstEvent->dev_task_id = eventValue; //< Event value should be the taskID
                        burstEvent->record.new_task.parent_task_id = 1; //wd->getParent()->getId();
                        burstEvent->record.new_task.parent_task_frame = NULL;
                        burstEvent->record.new_task.new_task_id = eventValue;
                        //NOTE: The codeptr_ofn field must be not NULL as Extrae stores a pair (task_id, codeptr) used to emit the task switches.
                        //      Those switches will be ignored if the codeptr is NULL
                        uintptr_t codeptr = ( uintptr_t )( 0xF0000000 + events[eventIdx].getKey() ); //wd->getActiveDevice().getWorkFct();
                        burstEvent->record.new_task.codeptr_ofn = ( void * )( codeptr );

                        //set the ompt_event_task_switch information
                        ompt_record_ompt_t *switchEvent = burstEvent + 1;
                        switchEvent->type = ompt_event_task_switch;
                        switchEvent->time = eventTime;
                        switchEvent->thread_id = threadId;
                        switchEvent->dev_task_id = eventValue; //< Event value should be the taskID
                        switchEvent->record.task_switch.first_task_id = 0;
                        switchEvent->record.task_switch.second_task_id = eventValue;
                        break;
                     } case NANOS_BURST_END: {
                        //set the ompt_event_task_switch information
                        ompt_record_ompt_t *switchEvent = &buffer.buffer[ current + i ];
                        switchEvent->type = ompt_event_task_switch;
                        switchEvent->time = eventTime;
                        switchEvent->thread_id = threadId;
                        switchEvent->dev_task_id = eventValue; //< Event value should be the taskID
                        switchEvent->record.task_switch.first_task_id = eventValue;
                        switchEvent->record.task_switch.second_task_id = 0;

                        //set the ompt_event_task_end information
                        ompt_record_ompt_t *burstEvent = switchEvent + 1;
                        burstEvent->type = ompt_event_task_end;
                        burstEvent->time = eventTime;
                        burstEvent->thread_id = threadId;
                        burstEvent->dev_task_id = eventValue;
                        burstEvent->record.task.task_id = eventValue;
                        break;
                     } default: {
                        warning( "Found an unsupported event type in InstrumentationOMPT::addDeviceEventList" );
                        break;
                     }
                  }

                  eventIdx++;
               }

               buffer.done += countCurrent;
            }
         }
         void createEventBuffer() {
             _devEventBuffers.push_back( NEW BufferInfo() );
         }

         void completeDeviceBuffer( int deviceId ) {
            BufferInfo &eventBuffer = *_devEventBuffers[ deviceId ];
            if (eventBuffer.current != eventBuffer.begin) {
               _completeBufferCallback( deviceId, ( ompt_target_buffer_t *) eventBuffer.buffer,
                     eventBuffer.size, eventBuffer.begin, eventBuffer.current );
               eventBuffer.current = eventBuffer.begin;
            }
         }
   };

extern "C" {

   /*
    * The following functions need to be redeclared declarations are inside an extern "C"
    * block. This makes these previous declarations non visible from here.
    */
   int ompt_nanos_target_get_device_info( int32_t device_id, const char **type,
         ompt_target_device_t **device, ompt_function_lookup_t *lookup,
         const char *documentation );
   int ompt_nanos_target_get_device_info( int32_t device_id, const char **type,
         ompt_target_device_t **device, ompt_function_lookup_t *lookup,
         const char *documentation )
   {
      /* FIXME Looks like documentation should be const char **documentation
       * as specification suggests that it is an output parameter but still provides
       * this type signature
       */
      InstrumentationOMPT *instr = ( InstrumentationOMPT *) sys.getInstrumentation();
      DeviceInstrumentation *devInstr = instr->getDeviceInstrumentation( device_id );
      *type = devInstr->getDeviceType();
      *device = (ompt_target_device_t*)devInstr;
      *lookup = ompt_nanos_target_lookup;

      //Create event buffer for this device
      instr->createEventBuffer();

      return 0;
   }

   int ompt_nanos_target_start_trace( ompt_target_device_t *device,
         ompt_target_buffer_request_callback_t request,
         ompt_target_buffer_complete_callback_t complete );
   int ompt_nanos_target_start_trace( ompt_target_device_t *device,
         ompt_target_buffer_request_callback_t request,
         ompt_target_buffer_complete_callback_t complete )
   {
      DeviceInstrumentation *devInstr = ( DeviceInstrumentation * )device;
      devInstr->startDeviceTrace();
      //we can assume that we are using ompt instrumentation as we've got so far
      InstrumentationOMPT *instr = ( InstrumentationOMPT *) sys.getInstrumentation();
      instr->setRequestBufferCallback( request );
      instr->setCompleteBufferCallback( complete );

      return 0; //Success
   }

   /*!
    * Advances cursor to point to the next position in the buffer
    * \return 0 on success. 1 if cursor goes out of bounds
    */
   int ompt_nanos_target_advance_buffer_cursor( ompt_target_buffer_t *buffer,
         ompt_target_buffer_cursor_t current,
         ompt_target_buffer_cursor_t *next );
   int ompt_nanos_target_advance_buffer_cursor( ompt_target_buffer_t *buffer,
         ompt_target_buffer_cursor_t current,
         ompt_target_buffer_cursor_t *next )
   {
      //Need to forward the call to the instrumentation class as we need buffer information
      //(length) in order to know if cursor goes out of bounds
      InstrumentationOMPT *instr = ( InstrumentationOMPT * ) sys.getInstrumentation();
      return instr->advanceBuffer(buffer, current, next);
   }

   int ompt_nanos_target_stop_trace ( ompt_target_device_t *device );
   int ompt_nanos_target_stop_trace ( ompt_target_device_t *device )
   {
      DeviceInstrumentation * devInst = ( DeviceInstrumentation * )device;
      InstrumentationOMPT *instr = ( InstrumentationOMPT * ) sys.getInstrumentation();
      //notify the device to stop instrumentation
      devInst->stopDeviceTrace();
      //Complete event buffer for this device
      instr->completeDeviceBuffer( devInst->getId() );
      return 0;
   }

   ompt_thread_id_t ompt_nanos_get_thread_id( void );
   ompt_thread_id_t ompt_nanos_get_thread_id( void )
   {
      //If instrumentation calls this before anything is initialized,
      //return 0 as the master is doing everything
      if ( nanos::myThread != NULL ) {
         return (ompt_thread_id_t) nanos::InstrumentationOMPT::getCurrentThreadId();
      } else {
         return 0;
      }
   }
}
   namespace ext
   {
      class InstrumentationOMPTPlugin : public Plugin
      {
         public:
            InstrumentationOMPTPlugin () : Plugin("Instrumentation OMPT compatible.",1) {}
            ~InstrumentationOMPTPlugin () {}
            void config( Config &cfg ) {}
            void init () { sys.setInstrumentation( NEW InstrumentationOMPT() ); }
      };
   } // namespace ext
} // namespace nanos

DECLARE_PLUGIN("instrumentation-ompt",nanos::ext::InstrumentationOMPTPlugin);
