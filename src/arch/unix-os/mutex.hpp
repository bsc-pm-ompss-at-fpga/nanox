/*************************************************************************************/
/*      Copyright 2009-20016 Barcelona Supercomputing Center                         */
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

#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "error.hpp"

#include <pthread.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

namespace nanos {

struct defer_lock_t {};
struct try_to_lock_t {};
struct adopt_lock_t {};

struct read_access_t {};
struct write_access_t {};

extern defer_lock_t   defer_lock;
extern try_to_lock_t  try_to_lock;
extern adopt_lock_t   adopt_lock;

extern read_access_t  read_access;
extern write_access_t write_access;

class Mutex {
	private:
		pthread_mutex_t _handle;

		// Non copyable
		Mutex( const Mutex & );

		// Non assignable
		Mutex& operator=( const Mutex & );

	public:
#ifdef HAVE_CXX11
		// This constructor is only compatible
		// with C++11 or later.
		Mutex() :
			_handle(PTHREAD_MUTEX_INITIALIZER)
		{
		}
#else
		Mutex()
		{
			// Alternative for PTHREAD_MUTEX_INITIALIZER
			pthread_mutex_init( &_handle, NULL );
		}
#endif

		~Mutex()
		{
			pthread_mutex_destroy(&_handle);
		}

		void lock()
		{
			int error = pthread_mutex_lock(&_handle);
			ensure( error == 0, "Failed to unlock mutex" );
		}

		void unlock()
		{
			int error = pthread_mutex_unlock(&_handle);
			ensure( error == 0, "Failed to unlock mutex" );
		}

		bool try_lock() throw()
		{
			return !pthread_mutex_trylock(&_handle);
		}

		pthread_mutex_t* native_handle()
		{
			return &_handle;
		}
};

class ReadWriteLock {
	private:
		pthread_rwlock_t _handle;

#ifdef HAVE_CXX11
	public:
		ReadWriteLock() :
			_handle(PTHREAD_RWLOCK_INITIALIZER)
		{
		}

		// Non copyable
		ReadWriteLock( const ReadWriteLock & ) = delete;

		// Non assignable
		ReadWriteLock& operator=( const ReadWriteLock & ) = delete;
#else
		// Non copyable
		ReadWriteLock( const ReadWriteLock & );

		// Non assignable
		ReadWriteLock& operator=( const ReadWriteLock & );

	public:
		ReadWriteLock()
		{
			// Alternative for PTHREAD_READWRITE_LOCK_INITIALIZER
			pthread_rwlock_init( &_handle, NULL );
		}
#endif

		~ReadWriteLock()
		{
			pthread_rwlock_destroy(&_handle);
		}

		void lock( read_access_t type )
		{
			int error = pthread_rwlock_rdlock(&_handle);
			ensure( error == 0, "Failed to acquire rwlock for read" );
		}

      // If no access type is specificed, write access
      // is used (mutual exclusion).
		void lock( write_access_t type = write_access)
		{
			int error = pthread_rwlock_wrlock(&_handle);
			ensure( error == 0, "Failed to acquire rwlock for write" );
		}

		bool try_lock( read_access_t type ) throw()
		{
			return !pthread_rwlock_tryrdlock(&_handle);
		}

      // If no access type is specificed, write access
      // is used (mutual exclusion).
		bool try_lock( write_access_t type = write_access ) throw()
		{
			return !pthread_rwlock_trywrlock(&_handle);
		}

		void unlock()
		{
			int error = pthread_rwlock_unlock(&_handle);
			ensure( error == 0, "Failed to unlock rwlock" );
		}

		pthread_rwlock_t* native_handle()
		{
			return &_handle;
		}
};

template < class MutexType >
class LockGuard {
	private:
		MutexType& _mutex;

		LockGuard( const LockGuard & ); // Non copyable

		LockGuard& operator=( const LockGuard & ); // Non assignable

	public:
		LockGuard( MutexType &m ) :
			_mutex(m)
		{
			_mutex.lock();
		}

		template < class... Args >
		LockGuard( MutexType &m, Args&&... args ) :
			_mutex(m)
		{
			_mutex.lock( std::forward<Args>(args)... );
		}

		LockGuard( MutexType &m, adopt_lock_t t ) :
			_mutex(m)
		{
		}

		~LockGuard()
		{
			_mutex.unlock();
		}
};

template < class MutexType >
class UniqueLock {
	private:
		MutexType& _mutex;
		bool       _isOwner;

		UniqueLock( const UniqueLock & ); // Non copyable

		UniqueLock& operator=( const UniqueLock & ); // Non assignable
	public:
		UniqueLock( MutexType &m ) :
			_mutex(m), _isOwner(false)
		{
			_mutex.lock();
			_isOwner = true;
		}

		template < class... Args >
		UniqueLock( MutexType &m, Args&&... args ) :
			_mutex(m), _isOwner(false)
		{
			_mutex.lock( std::forward<Args>(args)... );
			_isOwner = true;
		}

		UniqueLock( MutexType &m, defer_lock_t t ) :
			_mutex(m), _isOwner(false)
		{
		}

		UniqueLock( MutexType &m, try_to_lock_t t ) :
			_mutex(m), _isOwner(false)
		{
			_isOwner = _mutex.try_lock();
		}

		template < class... Args >
		UniqueLock( MutexType &m, try_to_lock_t t, Args&&... args ) :
			_mutex(m), _isOwner(false)
		{
			_isOwner = _mutex.try_lock( std::forward<Args>(args)... );
		}

		UniqueLock( MutexType &m, adopt_lock_t t ) :
			_mutex(m), _isOwner(true)
		{
		}

		~UniqueLock()
		{
			if( owns_lock() )
				unlock();
		}

		template < class... Args >
		void lock( Args&&... args)
		{
			_mutex.lock( std::forward<Args>(args)... );
			_isOwner = true;
		}

		template < class... Args >
		bool try_lock( Args&&... args )
		{
			_isOwner = _mutex.try_lock( std::forward<Args>(args)... );
			return _isOwner;
		}

		void unlock()
		{
			_isOwner = false;
			_mutex.unlock();
		}

		bool owns_lock() const throw()
		{
			return _isOwner;
		}

		MutexType* mutex() const throw()
		{
			return &_mutex;
		}
};

} // namespace nanos

#pragma GCC diagnostic pop

#endif // MUTEX_HPP

