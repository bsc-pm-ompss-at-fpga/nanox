#ifndef _NANOS_RESILIENCE_H
#define _NANOS_RESILIENCE_H

#include "resilience_decl.hpp"
#include "system.hpp"

using namespace nanos;

inline ResilienceNode * ResiliencePersistence::getFreeResilienceNode() {
   _resilienceTreeLock.acquire();
   if( _freeResilienceNodes.size() == 0 ) {
      _resilienceTreeLock.release();
      return NULL;
   }
   int index = _freeResilienceNodes.front();
   ResilienceNode * res = &_resilienceTree[index];
   res->setInUse( true );
   _usedResilienceNodes.push_back( index ); 
   _freeResilienceNodes.pop();
   _resilienceTreeLock.release();
   return res;
}

inline void ResiliencePersistence::freeResilienceNode( int index ) {
   _resilienceTreeLock.acquire();
   _usedResilienceNodes.remove( index - 1 );
   _freeResilienceNodes.push( index - 1 );
   memset( getResilienceNode( index ), 0, sizeof(ResilienceNode) );
   _resilienceTreeLock.release();
}

inline ResilienceNode * ResiliencePersistence::getResilienceNode( int offset ) { if( offset < 1 ) return NULL; return _resilienceTree+offset-1; }

inline void * ResiliencePersistence::getResilienceResultsFreeSpace( size_t size ) { 
   if( size > _RESILIENCE_MAX_FILE_SIZE ) 
       fatal( "Not enough space in file." ); 

   _resilienceResultsLock.acquire(); 
   if( _freeResilienceResults.size() == 0 ) {
       _resilienceResultsLock.release(); 
       fatal( "Not enough space in file." ); 
   }

   for( std::map<int, size_t>::iterator it = _freeResilienceResults.begin();
           it != _freeResilienceResults.end();
           it++ ) 
   {
       if( size <= it->second ) {
           std::pair<int, size_t> p( it->first, it->second );
           _freeResilienceResults.erase( it );
           if( size < it->second )
               _freeResilienceResults.insert( std::make_pair( p.first + size, p.second - size ) );
           _resilienceResultsLock.release(); 
           return getResilienceResults( p.first );
       }
   }
   _resilienceResultsLock.release(); 
   fatal( "Cannot reserve space in results." );
}


inline void * ResiliencePersistence::getResilienceResults( int offset ) { return ( char * )_resilienceResults + offset; }

inline void ResiliencePersistence::freeResilienceResultsSpace( int offset, size_t size ) {
    _resilienceResultsLock.acquire();
    if( _freeResilienceResults.find( offset ) != _freeResilienceResults.end() )
        fatal( "Already freed results space.");
    _freeResilienceResults.insert( std::make_pair( offset, size ) );
    // Join consecutive chunks.
    for( std::map<int, size_t>::iterator it = _freeResilienceResults.begin();
            it != _freeResilienceResults.end();
            it++ )
    {
        std::map<int, size_t>::iterator next = _freeResilienceResults.find( it->first + it->second );
        if( next != _freeResilienceResults.end() ) {
            it->second += next->second;
            _freeResilienceResults.erase( next );
        }
    }
    _resilienceResultsLock.release();
}

inline void ResiliencePersistence::restoreResilienceResultsSpace( int offset, size_t size ) {
    std::map<int, size_t>::iterator it = _freeResilienceResults.find( offset );
    if( it != _freeResilienceResults.end() ) {
        if( size > it->second ) {
            fatal0( "Cannot reserve such space in this position." );
        }
        else if( size == it->second ) {
            _freeResilienceResults.erase( it );
        }
        else {
            std::pair<int, size_t> p( it->first, it->second );
            _freeResilienceResults.erase( it );
            _freeResilienceResults.insert( std::make_pair( p.first + size, p.second - size ) );
        }
    }
    else {
        for( it = _freeResilienceResults.begin();
                it != _freeResilienceResults.end();
                it++ )
        {
            if( offset < it->first ) 
                break;

            if( offset + size == it->first + it->second ) {
                std::pair<int, size_t> p( it->first, it->second );
                _freeResilienceResults.erase( it );
                _freeResilienceResults.insert( std::make_pair( p.first, offset - p.first ) );
                return;
            }
            else if( offset + size < it->first + it->second ) {
                std::pair<int, size_t> p( it->first, it->second );
                _freeResilienceResults.erase( it );
                _freeResilienceResults.insert( std::make_pair( p.first, offset - p.first ) );
                _freeResilienceResults.insert( std::make_pair( offset + size, p.second - size - ( offset - p.first ) ) );
                return;
            }
        }
        fatal0( "Cannot reserve such space in this position." );
    }
}

inline void ResilienceNode::setParent( ResilienceNode * parent ) { 
    /*  Indexing starts from 1. So I get the difference between the argument passed and the 
     *  first element, +1, to be coherent with the indexing starting from 1. If I don't 
     *  make the increment, then parent will be 0 and sys.getResilienceNode(0) is NULL.
     */
    _parent = parent - sys.getResiliencePersistence()->getResilienceNode( 1 ) + 1; 
    sys.getResiliencePersistence()->getResilienceNode( _parent )->addDesc( this ); 
} 

inline ResilienceNode * ResilienceNode::getParent() { return sys.getResiliencePersistence()->getResilienceNode( _parent ); }
inline size_t ResilienceNode::getNumDescendants() { return _descSize; }
inline bool ResilienceNode::isComputed() const { return _computed; }
inline void ResilienceNode::restartLastDescVisited() { _lastDescVisited = 0; }
inline void ResilienceNode::restartLastDescRestored() { _lastDescRestored = 0; }
inline bool ResilienceNode::isInUse() const { return _inUse; }
inline void ResilienceNode::setInUse( bool flag ) { _inUse = flag; }
inline int ResilienceNode::getId() const { return _id; }
inline void ResilienceNode::setId( int id ) { _id = id; }
inline size_t ResilienceNode::getResultSize() const { return _resultSize; }
inline int ResilienceNode::getResultIndex() const { return _resultIndex; }

#endif /* _NANOS_RESILIENCE_H */
