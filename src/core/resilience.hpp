#ifndef _NANOS_RESILIENCE_H
#define _NANOS_RESILIENCE_H

#include "resilience_decl.hpp"
#include "system.hpp"

using namespace nanos;

inline ResilienceNode * ResiliencePersistence::getFreeResilienceNode( ResilienceNode * parent ) {
    _resilienceTreeLock.acquire();

    if( _freeResilienceNodes.size() == 0 ) {
        _resilienceTreeLock.release();
        fatal0( "Not enough space in file." );
    }

    unsigned int index = _freeResilienceNodes.front();
    ResilienceNode * res = &_resilienceTree[index];

    if( parent != NULL )
        /* Indexes start from 1 to be able to distinguish a initialized value (0) with a setted one (1..N). */
        res->setInUse( parent->addDesc( index + 1 ) );
    else
        res->setInUse( true );

    _freeResilienceNodes.pop();
    _usedResilienceNodes.push_back( index ); 

    _resilienceTreeLock.release();
    return res;
}

inline ResilienceNode * ResiliencePersistence::getResilienceNode( unsigned int offset ) { 
    if( offset < 1 || offset > _RESILIENCE_MAX_FILE_SIZE/sizeof(ResilienceNode) ) return NULL; 
    return _resilienceTree+offset-1;
}

inline void ResiliencePersistence::freeResilienceNode( unsigned int offset ) {
    _resilienceTreeLock.acquire();
    memset( getResilienceNode( offset ), 0, sizeof(ResilienceNode) );
    _usedResilienceNodes.remove( offset - 1 );
    _freeResilienceNodes.push( offset - 1 );
    _resilienceTreeLock.release();
}

inline void * ResiliencePersistence::getResilienceResultsFreeSpace( size_t size ) { 
    if( size > _RESILIENCE_MAX_FILE_SIZE ) 
        fatal( "Not enough space in file." ); 

    _resilienceResultsLock.acquire(); 

    if( _freeResilienceResults.size() == 0 ) {
        _resilienceResultsLock.release(); 
        fatal( "Not enough space in file." ); 
    }

    for( std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.begin();
            it != _freeResilienceResults.end();
            it++ ) 
    {
        if( size <= it->second ) {
            std::pair<unsigned int, size_t> p( it->first, it->second );
            _freeResilienceResults.erase( it );
            if( size < it->second )
                _freeResilienceResults.insert( std::make_pair( p.first + size, p.second - size ) );
            _resilienceResultsLock.release(); 
            return getResilienceResults( p.first );
        }
    }

    _resilienceResultsLock.release(); 
    fatal( "Cannot reserve such space in results." );
}

inline void * ResiliencePersistence::getResilienceResults( unsigned int offset ) { 
    if( offset >= _RESILIENCE_MAX_FILE_SIZE ) return NULL;
    return ( char * )_resilienceResults + offset;
}

inline void ResiliencePersistence::freeResilienceResultsSpace( unsigned int offset, size_t size ) {
    _resilienceResultsLock.acquire();

    if( _freeResilienceResults.find( offset ) != _freeResilienceResults.end() )
        fatal( "Already freed results space.");
    _freeResilienceResults.insert( std::make_pair( offset, size ) );
    // Join consecutive chunks.
    for( std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.begin();
            it != _freeResilienceResults.end();
            it++ )
    {
        std::map<unsigned int, size_t>::iterator next = _freeResilienceResults.find( it->first + it->second );
        if( next != _freeResilienceResults.end() ) {
            it->second += next->second;
            _freeResilienceResults.erase( next );
        }
    }

    _resilienceResultsLock.release();
}

inline void ResiliencePersistence::restoreResilienceResultsSpace( unsigned int offset, size_t size ) {
    std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.find( offset );
    if( it != _freeResilienceResults.end() ) {
        if( size > it->second ) {
            fatal0( "Cannot reserve such space in this position." );
        }
        else if( size == it->second ) {
            _freeResilienceResults.erase( it );
        }
        else {
            std::pair<unsigned int, size_t> p( it->first, it->second );
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
                std::pair<unsigned int, size_t> p( it->first, it->second );
                _freeResilienceResults.erase( it );
                _freeResilienceResults.insert( std::make_pair( p.first, offset - p.first ) );
                return;
            }
            else if( offset + size < it->first + it->second ) {
                std::pair<unsigned int, size_t> p( it->first, it->second );
                _freeResilienceResults.erase( it );
                _freeResilienceResults.insert( std::make_pair( p.first, offset - p.first ) );
                _freeResilienceResults.insert( std::make_pair( offset + size, p.second - size - ( offset - p.first ) ) );
                return;
            }
        }
        fatal0( "Cannot reserve such space in this position." );
    }
}

inline bool ResilienceNode::isInUse() const { return _inUse; }
inline void ResilienceNode::setInUse( bool flag ) { _inUse = flag; }
inline bool ResilienceNode::isComputed() const { return _computed; }
inline int ResilienceNode::getResultIndex() const { return _resultIndex; }
inline size_t ResilienceNode::getResultSize() const { return _resultSize; }
inline size_t ResilienceNode::getNumDescendants() { return _descSize; }
inline void ResilienceNode::restartLastDescRestored() { _lastDescRestored = 0; }

#endif /* _NANOS_RESILIENCE_H */
