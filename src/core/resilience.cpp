#include "resilience_decl.hpp"
#include "resilience.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "copydata.hpp"

namespace nanos {

    ResilienceNode::~ResilienceNode() {
        //if( _desc != 0 )
            //removeAllDescsNode();

        //I have to break the association with _parent. In other words, remove this node from the parent _desc list.
        if ( _parent != -1 ) {
            sys.getResilienceNode( _parent )->removeDesc(this);
        }
    }

    void ResilienceNode::removeAllDescs() {
        ResilienceNode * current = sys.getResilienceNode( _desc );
        while( current != NULL ){
            ResilienceNode * toDelete = current;
            current = sys.getResilienceNode( current->_next );
            delete toDelete;
        }
        _descSize = 0;
    }


    void ResilienceNode::loadResult( CopyData * copies, size_t numCopies, int task_id ) { 
        //std::cerr << "Loading result from RN " << this << std::endl;
        char * aux = ( char * ) sys.getResilienceResults( _result );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( copy_address, aux, copy_size );
                //std::stringstream ss;
                //ss << std::dec << "Task " << task_id << " with RN " << _id << " has loaded " << *(int *)aux << " in x[" << copies[i].getOffset()/copy_size 
                //   << "] from " << sys.getResilienceResults( _result ) << " (results[" << _result << "])" << std::endl;
                //message(ss.str());
                aux += copy_size;
            }
        }
        sys.getResilienceNode( _parent )->_lastDescVisited++;
    }

    void ResilienceNode::storeResult( CopyData * copies, size_t numCopies, int task_id ) {
        //Calculate outputs size
        size_t outputs_size = 0;
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                outputs_size += copies[i].getDimensions()->accessed_length;
            }
        }
        //Get result from resilience results mmaped file.
        _resultsSize = outputs_size;
        _result = ( char * )sys.getResilienceResultsFreeSpace( _resultsSize ) - ( char * )sys.getResilienceResults( 0 );

        //Copy the result
        char * aux = ( char * ) sys.getResilienceResults( _result );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( aux, copy_address, copy_size );
                //std::stringstream ss;
                //ss << std::dec << "Task " << task_id << " with RN " << _id << " has stored " << *(int *)copy_address << " in x[" << copies[i].getOffset()/copy_size
                //   << "] to " << sys.getResilienceResults( _result ) << "(results[" << _result << "])" << std::endl;
                //message(ss.str());
                aux += copy_size;
            }
        }

        //Mark it as computed
        _computed = true;

        //Remove all descendents. They are not needed anymore.
        //removeAllDescs();
    }

    void ResilienceNode::addDesc( ResilienceNode * rn ) { 
        if( rn == NULL )
            return;

        if( _desc == 0 ) 
            _desc = rn - sys.getResilienceNode( 1 ) + 1;
        else
            sys.getResilienceNode( _desc )->addNext( rn );

        _descSize++;
    }

    void ResilienceNode::removeDesc( ResilienceNode * rn ) { 
        if( _desc == 0 || rn == NULL )
            return;

        if( _desc == rn - sys.getResilienceNode( 1 ) + 1 ) 
            _desc = sys.getResilienceNode( _desc )->_next;
        else 
            sys.getResilienceNode( _desc )->removeNext( rn );

        _descSize--;
        memset( rn, 0, sizeof(ResilienceNode) );
    }

    void ResilienceNode::addNext( ResilienceNode * rn ) {
        if( _next == 0 )
            _next = rn - sys.getResilienceNode( 1 ) + 1;
        else {
            ResilienceNode * current = sys.getResilienceNode( _next );
            while( current->_next != 0 ) {
                current = sys.getResilienceNode( current->_next );
            }
            current->_next = rn - sys.getResilienceNode( 1 ) + 1;
        }
    }

    void ResilienceNode::removeNext( ResilienceNode * rn ) {
        if( _next == 0 )
            return;

        if( _next == rn - sys.getResilienceNode( 1 ) + 1 )
            _next = sys.getResilienceNode( _next )->_next;
        else {
            ResilienceNode * current = sys.getResilienceNode( _next );
            while( current->_next != 0 && current->_next != rn - sys.getResilienceNode( 1 ) + 1 ) {
                current = sys.getResilienceNode( current->_next );
            }
            if( current->_next == rn - sys.getResilienceNode( 1 ) + 1 )
                current->_next = sys.getResilienceNode( current->_next )->_next;
        }
    }

    ResilienceNode* ResilienceNode::getNextDesc( bool inc ) {
        //TODO: FIXME: Maybe, this should throw FATAL ERROR.
        if( _desc == 0 || _lastDescVisited >= _descSize ) {
            _lastDescVisited++;
            return NULL;
        }

        ResilienceNode * desc = sys.getResilienceNode( _desc );
        for( unsigned int i = 0; i < _lastDescVisited; i++) {
            //TODO: FIXME: Maybe, this should throw FATAL ERROR.
            if( desc->_next == 0 ) {
                _lastDescVisited++;
                return NULL;
            }
            desc = sys.getResilienceNode( desc->_next );
        }

        if( desc != NULL && !desc->isComputed() )
            _lastDescVisited++;

        return desc;
    }

    ResilienceNode* ResilienceNode::getNextDescToRestore( bool inc ) {
        //TODO: FIXME: Maybe, this should throw FATAL ERROR.
        if( _desc == 0 || _lastDescRestored >= _descSize ) {
            _lastDescRestored++;
            return NULL;
        }

        ResilienceNode * desc = sys.getResilienceNode( _desc );
        for( unsigned int i = 0; i < _lastDescRestored; i++) {
            //TODO: FIXME: Maybe, this should throw FATAL ERROR.
            if( desc->_next == 0 ) {
                _lastDescRestored++;
                return NULL;
            }
            desc = sys.getResilienceNode( desc->_next );
        }

        if( desc != NULL )
            _lastDescRestored++;

        return desc;
    }

    //void ResilienceNode::restartAllLastDescVisited() { 
    //    _lastDescVisited = 0; 
    //    if( _parent != 0 ) 
    //        sys.getResilienceNode( _parent )->restartAllLastDescVisited();
    //}

    //void ResilienceNode::restartAllLastDescRestored() { 
    //    _lastDescRestored = 0; 
    //    if( _parent != 0 ) 
    //        sys.getResilienceNode( _parent )->restartAllLastDescRestored();
    //}

}
