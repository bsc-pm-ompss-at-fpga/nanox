/*************************************************************************************/
/*      Copyright 2015 Barcelona Supercomputing Center                               */
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

#ifndef _NANOS_TASK_REDUCTION_DECL_H
#define _NANOS_TASK_REDUCTION_DECL_H

//! \brief This class represent a Task Reduction.
//!
//! It contains all the information needed to handle a task reduction. It storages the
//! thread private copies and also keep all the information in order to compute final
//! reduction: reducers.
//

class TaskReduction {

   public:

      typedef void ( *initializer_t ) ( void *omp_priv,  void* omp_orig );
      typedef void ( *reducer_t ) ( void *obj1, void *obj2 );
      typedef std::vector<char> storage_t;

   private:

      // These two variables have the same value in almost all the cases. They
      // are only different when we are doing a Fortran Array Reduction
      void           *_original;         //!< Original variable address
      void           *_dependence;       //!< Related dependence

      unsigned        _depth;            //!< Reduction depth
      initializer_t   _initializer;      //!< Initialization function

      // These two variables have the same value in almost all the cases. They
      // are only different when we are doing a Fortran Array Reduction
      reducer_t       _reducer;          //!< Reducer operator
      reducer_t       _reducer_orig_var; //!< Reducer on orignal variable

      storage_t       _storage;          //!< Private copy vector
      size_t          _size_target;      //!< Size of array (size of element is scalar)
      size_t          _size_element;     //!< Size of element
      size_t          _num_elements;     //!< Number of elements (for a scalar reduction, this is 1)
      size_t          _num_threads;      //!< Number of threads (private copies)
      void           *_min;              //!< Pointer to first private copy
      void           *_max;              //!< Pointer to last private copy

      //! \brief TaskReduction copy constructor (disabled)
      TaskReduction( const TaskReduction &tr ) {}

   public:

      //! \brief TaskReduction constructor only used when we are performing a Reduction
      TaskReduction( void *orig, initializer_t init, reducer_t red,
    		  	  size_t size_target, size_t size_elem, size_t
				  threads, unsigned depth )
               	   : _original(orig), _dependence(orig), _depth(depth), _initializer(init),
					 _reducer(red), _reducer_orig_var(red), _storage(size_target*threads),
					 _size_target(size_target), _size_element(size_elem),_num_elements(size_target/size_elem),
					 _num_threads(threads), _min(NULL), _max(NULL)
      {
    	 _min          = & _storage[0];
         _max          = & _storage[_size_target*threads];

         //For each thread
         for ( size_t i=0; i<threads; i++) {
        	 //Initialize all elements (1 for scalars)
        	 for(size_t j=0; j<_num_elements; j++ ) {

        		 _initializer( &_storage[i*_size_target + j*_size_element], _original );
        	 }
         }
      }

      //! \brief TaskReduction constructor only used when we are performing a Fortran Array Reduction
      TaskReduction( void *orig, void *dep, initializer_t init, reducer_t red,
            reducer_t red_orig_var, size_t array_descriptor_size, size_t
            threads, unsigned depth )
               : _original(orig), _dependence(dep), _depth(depth),
                 _initializer(init), _reducer(red), _reducer_orig_var(red_orig_var),
                 _storage(array_descriptor_size*threads), _size_target(array_descriptor_size),
				 _size_element(0),_num_elements(0),
                 _num_threads(threads), _min(NULL), _max(NULL)
      {
         _min = & _storage[0];
         _max = & _storage[_size_target*threads];

         for ( size_t i=0; i<threads; i++) {
            // char ** addr = (char**) &_storage[i*_size];
            // *addr = &_storage[i*_size + array_descriptor];
            _initializer( &_storage[i*_size_target], _original );
         }
      }

      //! \brief Taskreduction destructor
     ~TaskReduction() {}

      //! \brief Is the provided address the original symbol or one of the private copies
      //! \return NULL if not matches, id's corresponding private copy if matches
      void * have ( const void *ptr, size_t id );

      //! \brief Finalizes reduction
      void * finalize ( void );

      //! \brief Get depth where task reduction were registered
      unsigned getDepth( void ) const;
};

#endif
