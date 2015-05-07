#ifndef _NANOS_RESILIENCE_H
#define _NANOS_RESILIENCE_H

#include "resilience_decl.hpp"
#include "system.hpp"

using namespace nanos;

inline void ResilienceNode::setParent( ResilienceNode * parent ) { 
    /*  Indexing starts from 1. So I get the difference between the argument passed and the 
     *  first element, +1, to be coherent with the indexing starting from 1. If I don't 
     *  make the increment, then parent will be 0 and sys.getResilienceNode(0) is NULL.
     */
    _parent = parent - sys.getResilienceNode( 1 ) + 1; 
    sys.getResilienceNode( _parent )->addDesc( this ); 
} 
inline ResilienceNode * ResilienceNode::getParent() { return sys.getResilienceNode( _parent ); }
inline size_t ResilienceNode::getNumDescendants() { return _descSize; }
inline bool ResilienceNode::isComputed() const { return _computed; }
inline void ResilienceNode::restartLastDescVisited() { _lastDescVisited = 0; }
inline void ResilienceNode::restartLastDescRestored() { _lastDescRestored = 0; }
inline bool ResilienceNode::isInUse() const { return _inUse; }
inline void ResilienceNode::setInUse( bool flag ) { _inUse = flag; }
inline int ResilienceNode::getId() const { return _id; }
inline void ResilienceNode::setId( int id ) { _id = id; }
inline size_t ResilienceNode::getResultsSize() const { return _resultsSize; }
inline int ResilienceNode::getResultIndex() const { return _result; }

#endif /* _NANOS_RESILIENCE_H */
