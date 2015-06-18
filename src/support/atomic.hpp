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

#ifndef _NANOS_ATOMIC
#define _NANOS_ATOMIC

#include "atomic_decl.hpp"
#include "basethread_decl.hpp"
#include "compatibility.hpp"
#include "nanos-int.h"
#include <algorithm> // for min/max
#include "instrumentationmodule_decl.hpp"

/* TODO: move to configure
#include <ext/atomicity.h>
#ifndef _GLIBCXX_ATOMIC_BUILTINS
#error "Atomic gcc builtins support is mandatory at this point"
#endif
*/


using namespace nanos;

template<typename T>
inline T Atomic<T>::fetchAndAdd ( const T& val )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_fetch_add(&_value, val, __ATOMIC_SEQ_CST);
#else
   return __sync_fetch_and_add( &_value,val );
#endif
}

template<typename T>
inline T Atomic<T>::addAndFetch ( const T& val )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_add_fetch(&_value, val, __ATOMIC_SEQ_CST);
#else
   return __sync_add_and_fetch( &_value,val );
#endif
}

template<typename T>
inline T Atomic<T>::fetchAndSub ( const T& val )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_fetch_sub( &_value, val, __ATOMIC_SEQ_CST);
#else
   return __sync_fetch_and_sub( &_value,val );
#endif
}

template<typename T>
inline T Atomic<T>::subAndFetch ( const T& val )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_sub_fetch( &_value, val, __ATOMIC_SEQ_CST);
#else
   return __sync_sub_and_fetch( &_value,val );
#endif
}

template<typename T>
inline T Atomic<T>::value() const
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_load_n(&_value, __ATOMIC_SEQ_CST);
#else
   return _value;
#endif
}

template<typename T>
inline T Atomic<T>::operator++ ()
{
   return addAndFetch();
}

template<typename T>
inline T Atomic<T>::operator-- ()
{
   return subAndFetch();
}

template<typename T>
inline T Atomic<T>::operator++ ( int val )
{
   return fetchAndAdd();
}

template<typename T>
inline T Atomic<T>::operator-- ( int val )
{
   return fetchAndSub();
}

template<typename T>
inline T Atomic<T>::operator+= ( const T val )
{
   return addAndFetch(val);
}

template<typename T>
inline T Atomic<T>::operator+= ( const Atomic<T> &val )
{
   return addAndFetch(val.value());
}

template<typename T>
inline T Atomic<T>::operator-= ( const T val )
{
   return subAndFetch(val);
}

template<typename T>
inline T Atomic<T>::operator-= ( const Atomic<T> &val )
{
   return subAndFetch(val.value());
}

template<typename T>
inline bool Atomic<T>::operator== ( const Atomic<T> &val )
{
   return value() == val.value();
}

template<typename T>
inline bool Atomic<T>::operator!= ( const Atomic<T> &val )
{
   return value() != val.value();
}

template<typename T>
inline bool Atomic<T>::operator< (const Atomic<T> &val )
{
   return value() < val.value();
}

template<typename T>
inline bool Atomic<T>::operator> ( const Atomic<T> &val ) const
{
   return value() > val.value();
}

template<typename T>
inline bool Atomic<T>::operator<= ( const Atomic<T> &val )
{
   return value() <= val.value();
}

template<typename T>
inline bool Atomic<T>::operator>= ( const Atomic<T> &val )
{
   return value() >= val.value();
}

template<typename T>
inline bool Atomic<T>::cswap ( const Atomic<T> &oldval, const Atomic<T> &newval )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   // FIXME: The atomics passed are const
   T* oldv = const_cast<T*>(&oldval._value);
   T* newv = const_cast<T*>(&newval._value);
   return __atomic_compare_exchange_n( &_value, oldv, newv,
         /* weak */ false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
#else
   return __sync_bool_compare_and_swap ( &_value, oldval.value(), newval.value() );
#endif
}

#ifdef HAVE_NEW_GCC_ATOMIC_OPS
template<typename T>
inline T& Atomic<T>::override()
{
   return _value;
}
#else
template<typename T>
inline volatile T& Atomic<T>::override()
{
   // Kludgy
   return _value;
}
#endif

template<typename T>
inline Atomic<T> & Atomic<T>::operator= ( const T val )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   __atomic_store_n(&_value, val, __ATOMIC_SEQ_CST);
#else
   _value = val;
#endif
   return *this;
}

template<typename T>
inline Atomic<T> & Atomic<T>::operator= ( const Atomic<T> &val )
{
   return operator=( val._value );
}

inline Lock::state_t Lock::operator* () const
{
   return this->getState();
}

inline Lock::state_t Lock::getState () const
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_load_n(&state_, __ATOMIC_SEQ_CST);
#else
   return state_;
#endif
}

inline void Lock::operator++ ( int )
{
   acquire();
}

inline void Lock::operator-- ( int )
{
   release();
}

inline void Lock::acquire ( void )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   acquire_noinst();
#else
   if ( (state_ == NANOS_LOCK_FREE) &&  !__sync_lock_test_and_set( &state_,NANOS_LOCK_BUSY ) ) return;

   // Disabling lock instrumentation; do not remove follow code which can be reenabled for testing purposes
   // NANOS_INSTRUMENT( InstrumentState inst(NANOS_ACQUIRING_LOCK) )
spin:
   while ( state_ == NANOS_LOCK_BUSY ) {}

   if ( __sync_lock_test_and_set( &state_,NANOS_LOCK_BUSY ) ) goto spin;

   // NANOS_INSTRUMENT( inst.close() )
#endif
}

inline void Lock::acquire_noinst ( void )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   while (__atomic_exchange_n( &state_, NANOS_LOCK_BUSY, __ATOMIC_SEQ_CST) == NANOS_LOCK_BUSY ) { }
#else
spin:
   while ( state_ == NANOS_LOCK_BUSY ) {}
   if ( __sync_lock_test_and_set( &state_,NANOS_LOCK_BUSY ) ) goto spin;
#endif
}

inline bool Lock::tryAcquire ( void )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   if (__atomic_load_n(&state_, __ATOMIC_SEQ_CST) == NANOS_LOCK_FREE)
   {
      if (__atomic_exchange_n(&state_, NANOS_LOCK_BUSY, __ATOMIC_SEQ_CST) == NANOS_LOCK_BUSY)
         return false;
      else // will return NANOS_LOCK_FREE
         return true;
   }
   else
   {
      return false;
   }
#else
   if ( state_ == NANOS_LOCK_FREE ) {
      if ( __sync_lock_test_and_set( &state_,NANOS_LOCK_BUSY ) ) return false;
      else return true;
   } else return false;
#endif
}

inline void Lock::release ( void )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   __atomic_store_n(&state_, 0, __ATOMIC_SEQ_CST);
#else
   __sync_lock_release( &state_ );
#endif
}

inline void nanos::memoryFence ()
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   __atomic_thread_fence(__ATOMIC_SEQ_CST);
#else
#ifndef __MIC__
    __sync_synchronize();
#else
    __asm__ __volatile__("" ::: "memory");
#endif
#endif
}

template<typename T>
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
inline bool nanos::compareAndSwap( T *ptr, T oldval, T  newval )
#else
inline bool nanos::compareAndSwap( volatile T *ptr, T oldval, T  newval )
#endif
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   return __atomic_compare_exchange_n(ptr, &oldval, newval,
         /* weak */ false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
#else
    return __sync_bool_compare_and_swap ( ptr, oldval, newval );
#endif
}

inline LockBlock::LockBlock ( Lock & lock ) : _lock(lock)
{
   acquire();
}

inline LockBlock::~LockBlock ( )
{
   release();
}

inline void LockBlock::acquire()
{
   _lock++;
}

inline void LockBlock::release()
{
   _lock--;
}

inline LockBlock_noinst::LockBlock_noinst ( Lock & lock ) : _lock(lock)
{
   acquire();
}

inline LockBlock_noinst::~LockBlock_noinst ( )
{
   release();
}

inline void LockBlock_noinst::acquire()
{
   _lock.acquire_noinst();
}

inline void LockBlock_noinst::release()
{
   _lock.release();
}

inline SyncLockBlock::SyncLockBlock ( Lock & lock ) : LockBlock(lock)
{
   memoryFence();
}

inline SyncLockBlock::~SyncLockBlock ( )
{
   memoryFence();
}

inline RecursiveLock::state_t RecursiveLock::operator* () const
{
   return state_;
}

inline RecursiveLock::state_t RecursiveLock::getState () const
{
   return state_;
}

inline void RecursiveLock::operator++ ( int )
{
   acquire( );
}

inline void RecursiveLock::operator-- ( int )
{
   release( );
}

inline void RecursiveLock::acquire ( )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   if ( __atomic_load_n(&_holderThread, __ATOMIC_SEQ_CST) == getMyThreadSafe() )
   {
      __atomic_add_fetch(&_recursionCount, 1, __ATOMIC_SEQ_CST);
      return;
   }

   while (__atomic_exchange_n( &state_, NANOS_LOCK_BUSY, __ATOMIC_SEQ_CST) == NANOS_LOCK_BUSY ) { }

   __atomic_store_n(&_holderThread, getMyThreadSafe(), __ATOMIC_SEQ_CST);
   __atomic_add_fetch(&_recursionCount, 1, __ATOMIC_SEQ_CST);

#else
   if ( _holderThread == getMyThreadSafe() )
   {
      _recursionCount++;
      return;
   }
   
spin:
   while ( state_ == NANOS_LOCK_BUSY ) {}

   if ( __sync_lock_test_and_set( &state_,NANOS_LOCK_BUSY ) ) goto spin;

   _holderThread = getMyThreadSafe();
   _recursionCount++;
#endif
}

inline bool RecursiveLock::tryAcquire ( )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   if ( __atomic_load_n(&_holderThread, __ATOMIC_SEQ_CST) == getMyThreadSafe() )
   {
      __atomic_add_fetch(&_recursionCount, 1, __ATOMIC_SEQ_CST);
      return true;
   }

   if ( __atomic_load_n(&state_, __ATOMIC_SEQ_CST) == NANOS_LOCK_FREE )
   {
      if ( __atomic_exchange_n( &state_, NANOS_LOCK_BUSY, __ATOMIC_SEQ_CST ) == NANOS_LOCK_BUSY )
      {
         return false;
      }
      else
      {
         __atomic_store_n(&_holderThread, getMyThreadSafe(), __ATOMIC_SEQ_CST);
         __atomic_add_fetch(&_recursionCount, 1, __ATOMIC_SEQ_CST);
         return true;
      }
   }
   else
   {
      return false;
   }
#else
   if ( _holderThread == getMyThreadSafe() ) {
      _recursionCount++;
      return true;
   }
   
   if ( state_ == NANOS_LOCK_FREE ) {
      if ( __sync_lock_test_and_set( &state_,NANOS_LOCK_BUSY ) ) return false;
      else
      {
         _holderThread = getMyThreadSafe();
         _recursionCount++;
         return true;
      }
   } else return false;
#endif
}

inline void RecursiveLock::release ( )
{
#ifdef HAVE_NEW_GCC_ATOMIC_OPS
   if ( __atomic_sub_fetch(&_recursionCount, 1, __ATOMIC_SEQ_CST) == 0 )
   {
      _holderThread = 0UL;
      __atomic_store_n(&state_, NANOS_LOCK_FREE, __ATOMIC_SEQ_CST);
   }
#else
   _recursionCount--;
   if ( _recursionCount == 0UL )
   {
      _holderThread = 0UL;
      __sync_lock_release( &state_ );
   }
#endif
}

inline RecursiveLockBlock::RecursiveLockBlock ( RecursiveLock & lock ) : _lock(lock)
{
   acquire();
}

inline RecursiveLockBlock::~RecursiveLockBlock ( )
{
   release();
}

inline void RecursiveLockBlock::acquire()
{
   _lock++;
}

inline void RecursiveLockBlock::release()
{
   _lock--;
}

inline SyncRecursiveLockBlock::SyncRecursiveLockBlock ( RecursiveLock & lock ) : RecursiveLockBlock(lock)
{
   memoryFence();
}

inline SyncRecursiveLockBlock::~SyncRecursiveLockBlock ( )
{
   memoryFence();
}

#endif
