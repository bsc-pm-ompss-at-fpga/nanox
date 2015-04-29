#include "resilience_decl.hpp"
#include "resilience.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "copydata.hpp"

namespace nanos {

    ResilienceNode::ResilienceNode( ResilienceNode * parent, CopyData * copies, size_t numCopies ) 
        : _parent( 0 ), _desc( 0 ), _next( 0 ), _computed( false ), _lastDescVisited( 0 )
    {
        if( parent != NULL ) {
            /*  Indexing starts from 1. So I get the difference between the argument passed and the 
             *  first element, +1, to be coherent with the indexing starting from 1. If I don't 
             *  make the increment, then parent will be 0.
             */
            _parent = parent - sys.getResilienceNode( 1 ) + 1; 
            sys.getResilienceNode( _parent )->addDescNode( this );
            std::cerr << "My parent is " << _parent << std::endl;
        }
    }

    ResilienceNode::~ResilienceNode() {
        //if( _desc != 0 )
            //removeAllDescsNode();

        //I have to break the association with _parent. In other words, remove this node from the parent _desc list.
        if ( _parent != -1 ) {
            sys.getResilienceNode( _parent )->removeDescNode(this);
        }
    }

    void ResilienceNode::removeAllDescsNode() {
        ResilienceNode * current = sys.getResilienceNode( _desc );
        while( current != NULL ){
            ResilienceNode * toDelete = current;
            current = sys.getResilienceNode( current->_next );
            delete toDelete;
        }
        _descSize = 0;
    }


    void ResilienceNode::loadResult( CopyData * copies, size_t numCopies ) { 
        //Increment wd.id generator to maintain coherence with RN.
        sys.getWorkDescriptorId();
        //std::cerr << "Loading result from ResilienceNode " << this << std::endl;
        char * aux = ( char * ) sys.getResilienceResults( _result );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                //Works for arrays.
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                //std::cerr << "Load result =  " << *(int *)aux << std::endl;
                memcpy( copy_address, aux, copy_size );
                //Only works for basic types.
                //memcpy( copies[i].getBaseAddress(), aux, copy_size );
                aux += copy_size;
            }
        }
        sys.getResilienceNode( _parent )->incLastDescVisited();
    }

    void ResilienceNode::storeResult( CopyData * copies, size_t numCopies ) {
        //std::cerr << "Storing result in RN " << this << std::endl;
        //Calculate outputs size
        size_t outputs_size = 0;
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                outputs_size += copies[i].getDimensions()->accessed_length;
            }
        }
        //Get result from resilience results mmaped file.
        _result = ( char * )sys.getResilienceResultsFreeSpace( outputs_size ) - ( char * )sys.getResilienceResults( 0 );

        //Copy the result
        char * aux = ( char * ) sys.getResilienceResults( _result );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                //Works for arrays.
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                //std::cerr << "Store result =  " << *(int *)copy_address << std::endl;
                memcpy( aux, copy_address, copy_size );
                //Only works for basic types.
                //memcpy( aux, copies[i].getBaseAddress(), copy_size );
                aux += copy_size;
            }
        }

        //Mark it as computed
        _computed = true;

        //Remove all descendents. They are not needed anymore.
        //removeAllDescsNode();
    }

    void ResilienceNode::addDescNode( ResilienceNode * rn ) { 
        if( rn == NULL )
            return;

        if( _desc == 0 ) 
            _desc = rn - sys.getResilienceNode( 1 ) + 1;
        else
            sys.getResilienceNode( _desc )->addNext( rn );
        _descSize++;
    }

    void ResilienceNode::removeDescNode( ResilienceNode * rn ) { 
        if( _desc == 0 || rn == NULL )
            return;

        if( _desc == rn - sys.getResilienceNode( 1 ) ) 
            _desc = sys.getResilienceNode( _desc )->_next;
        else 
            sys.getResilienceNode( _desc )->removeNext( rn );
        _descSize--;
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

        if( _next == rn - sys.getResilienceNode( 1 ) )
            _next = sys.getResilienceNode( _next )->_next;
        else {
            ResilienceNode * current = sys.getResilienceNode( _next );
            while( current->_next != 0 && current->_next != rn - sys.getResilienceNode( 1 ) ) {
                current = sys.getResilienceNode( current->_next );
            }
            if( current->_next == rn - sys.getResilienceNode( 1 ) )
                current->_next = sys.getResilienceNode( current->_next )->_next;
        }
    }

    ResilienceNode* ResilienceNode::getCurrentDescNode() {
        if( _desc == 0 || _lastDescVisited >= _descSize ) {
            //std::cerr << "No descendent" << std::endl;
            incLastDescVisited();
            return NULL;
        }
        ResilienceNode * currentDesc = sys.getResilienceNode( _desc );
        for( unsigned int i = 0; i < _lastDescVisited; i++)
            currentDesc = sys.getResilienceNode( currentDesc->_next );
        if( currentDesc != NULL && !currentDesc->isComputed() ) {
            incLastDescVisited();
        }
        return currentDesc;
    }

    void ResilienceNode::restartAllLastDescVisited() { 
        _lastDescVisited = 0; 
        if( _parent != 0 ) 
            sys.getResilienceNode( _parent )->restartAllLastDescVisited();
    }

}
