#include "resilience_decl.hpp"
#include "resilience.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "copydata.hpp"

namespace nanos {

    void ResilienceNode::removeAllDescs() {
        if( _descSize == 0 ) 
            return;

        ResilienceNode * current = sys.getResilienceNode( _desc );
        int prev = _desc;
        while( current != NULL ) {
            int toDelete = prev;
            current->removeAllDescs();
            prev = current->_next;
            current = sys.getResilienceNode( current->_next );
            sys.freeResilienceNode( toDelete );
            _descSize--;
        }

        ensure( _descSize == 0, "There are still descs" );
        _desc = 0;
    }


    void ResilienceNode::loadResult( CopyData * copies, size_t numCopies, int task_id ) { 
        char * aux = ( char * ) sys.getResilienceResults( _result );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( copy_address, aux, copy_size );
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
        ensure( outputs_size > 0, "Store result of 0 bytes makes no sense." );
        //Get result from resilience results mmaped file.
        _resultsSize = outputs_size;
        _result = ( char * )sys.getResilienceResultsFreeSpace( _resultsSize ) - ( char * )sys.getResilienceResults( 0 );
        //std::cerr << "RN " << _id << " has " << _resultsSize << " bytes in results[" << _result << "]." << std::endl;

        //Copy the result
        char * aux = ( char * ) sys.getResilienceResults( _result );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( aux, copy_address, copy_size );
                aux += copy_size;
            }
        }

        //Mark it as computed
        _computed = true;

        //Remove all descendents. They are not needed anymore.
        removeAllDescs();
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
}
