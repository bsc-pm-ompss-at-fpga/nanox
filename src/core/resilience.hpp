#ifndef _NANOS_RESILIENCE_H
#define _NANOS_RESILIENCE_H

#include "resilience_decl.hpp"
#include "system.hpp"

using namespace nanos;

inline void ResilienceNode::setParent( ResilienceNode * parent ) { 
    /*  Indexing starts from 1. So I get the difference between the argument passed and the 
     *  first element, +1, to be coherent with the indexing starting from 1. If I don't 
     *  make the increment, then parent will be 0.
     */
    _parent = parent - sys.getResilienceNode( 1 ) + 1; 
    sys.getResilienceNode( _parent )->addDescNode( this ); 
} 
inline ResilienceNode * ResilienceNode::getParent() { return sys.getResilienceNode( _parent ); }
inline bool ResilienceNode::isComputed() const { return _computed; }
inline void ResilienceNode::incLastDescVisited() { _lastDescVisited++; } 
inline void ResilienceNode::restartLastDescVisited() { _lastDescVisited = 0; }
inline bool ResilienceNode::isInUse() const { return _inUse; }
inline void ResilienceNode::setInUse( bool flag ) { _inUse = flag; }

#endif /* _NANOS_RESILIENCE_H */
