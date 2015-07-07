#ifndef _NANOS_RESILIENCE_H
#define _NANOS_RESILIENCE_H

#include "resilience_decl.hpp"
#include "system.hpp"

using namespace nanos;

inline ResilienceNode * ResiliencePersistence::getFreeResilienceNode( ResilienceNode * parent ) {
    int * ResilienceInfo = ( int * ) _resilienceResults;

    _resilienceTreeLock.acquire();
    // 0 is an invalid index. 
    if( _firstFreeResilienceNode == 0 )
        fatal0( "Not enough space in tree file." );

    // Get free node and change head for the next free one.
    int index_free = _firstFreeResilienceNode;
    int next = getResilienceNode( index_free )->_next;
    _firstFreeResilienceNode = next ? next : index_free+1; 

    if( !next && index_free+1 > ( int ) ( _RESILIENCE_TREE_MAX_FILE_SIZE/sizeof(ResilienceNode) ) )
        fatal0( "There are not resilience nodes remaining." );

    ResilienceInfo[0] = _firstFreeResilienceNode;

    // Break association between the current first free and the old first free.
    ResilienceNode * res = getResilienceNode( index_free ); 
    ResilienceNode * next_free = getResilienceNode( next ); 
    if( next_free != NULL /*&& next_free->_prev == index_free*/ ) {
        next_free->_prev = 0;
        //std::cerr << "getFreeRN: Node " << res->_next << " has _prev 0." << std::endl;
    }

    // Sanity check
    if( res->_inUse ) {
        std::cerr << "Node " << index_free << " is already in use." << std::endl;
        fatal0( "Node already in use." );
    }

    // Set parent and mark node as used.
    if( parent != NULL )
        res->setInUse( parent->addDesc( index_free ) );
    else
        res->setInUse( true );

    // Change first used to the current one.
    int index_used = _firstUsedResilienceNode;
    _firstUsedResilienceNode = index_free;
    ResilienceInfo[1] = _firstUsedResilienceNode;

    // Link the current first used to the old one.
    res->_next = index_used;
    //std::cerr << "getFreeRN: Node " << index_free << " has _next " << index_used << std::endl;
    if( index_free == index_used )
        fatal0( "getFreeRN: This is a cycle." );
    ResilienceNode * next_used = getResilienceNode( index_used );
    if( next_used != NULL ) {
        next_used->_prev = index_free;
        //std::cerr << "Node " << index_used << " has _prev " << index_free << std::endl;
    }

    //std::cerr << "RN " << index_free << " given." << std::endl;
    _resilienceTreeLock.release();
    return res;


    //_resilienceTreeLock.acquire();

    //if( _freeResilienceNodes.size() == 0 ) {
    //    _resilienceTreeLock.release();
    //    //std::cerr << "Not enough space in tree file." << std::endl;
    //    fatal0( "Not enough space in tree file." );
    //}

    //unsigned int index = _freeResilienceNodes.front();
    //ResilienceNode * res = &_resilienceTree[index];

    //if( parent != NULL )
    //    /* Indexes start from 1 to be able to distinguish a initialized value (0) with a setted one (1..N). */
    //    res->setInUse( parent->addDesc( index + 1 ) );
    //else
    //    res->setInUse( true );

    //_freeResilienceNodes.pop();
    //_usedResilienceNodes.push_back( index ); 

    //_resilienceTreeLock.release();
    //return res;
}

inline ResilienceNode * ResiliencePersistence::getResilienceNode( unsigned int offset ) { 
    if( offset < 1 || offset > _RESILIENCE_TREE_MAX_FILE_SIZE/sizeof(ResilienceNode) ) return NULL; 
    return _resilienceTree+offset-1;
}

inline void ResiliencePersistence::freeResilienceNode( unsigned int offset ) {
    _resilienceTreeLock.acquire();
    //std::cerr << "Freeing node " << offset << std::endl;
    int * ResilienceInfo = ( int * ) _resilienceResults;
    ResilienceNode * current = getResilienceNode( offset ); 
    ResilienceNode * next_used = getResilienceNode( current->_next ); 
    ResilienceNode * prev_used = getResilienceNode( current->_prev );
    int current_prev = current->_prev;
    int current_next = current->_next;

    // Clean Node
    memset( current, 0, sizeof(ResilienceNode) );

    // Remove [offset-1] from used.
    if( next_used != NULL ) { 
        next_used->_prev = current_prev;
        //std::cerr << "freeRN: Node " << current_next << " has _prev " << current_prev << std::endl;
        if( current_next == current_prev ) 
            fatal( "freeRN1: This is a cycle." );
    }
    if( prev_used != NULL ) {
        prev_used->_next = current_next;
        //std::cerr << "freeRN: Node " << current_prev << " has _next " << current_next << std::endl;
        if( current_next == current_prev ) 
            fatal( "freeRN2: This is a cycle." );
    }

    int first_used = _firstUsedResilienceNode;
    if( first_used == (int) offset ) {
        _firstUsedResilienceNode = current_next;
    }
    ResilienceInfo[1] = _firstUsedResilienceNode;
    //std::cerr << "_firstUsedResilienceNode = " << _firstUsedResilienceNode.value() << std::endl;

    // Push [offset-1] into free.
    int first_free = _firstFreeResilienceNode;
    _firstFreeResilienceNode = offset;
    ResilienceInfo[0] = _firstFreeResilienceNode;
    current->_next = first_free;
    current->_prev = 0;
    //std::cerr << "freeRN: Node " << offset << " has _next " << first_free << " and _prev 0." << std::endl;

    ResilienceNode * first_free_node = getResilienceNode( first_free );
    if( first_free_node != NULL ) { 
        if( first_free_node->_prev != 0 )
            warning( "This should not happen. This was head, so this should not has _prev." );
        first_free_node->_prev = offset;
        //std::cerr << "Node " << first_free << " has _prev " << offset << std::endl;
        if( first_free == (int) offset )
            fatal( "freeRN3: This is a cycle." );
    }

    //std::cerr << "Node " << offset << " freed." << std::endl;
    //std::cerr << "Node freed." << std::endl;
    _resilienceTreeLock.release();


    //_resilienceTreeLock.acquire();
    //memset( getResilienceNode( offset ), 0, sizeof(ResilienceNode) );
    //_usedResilienceNodes.remove( offset - 1 );
    //_freeResilienceNodes.push( offset - 1 );
    //_resilienceTreeLock.release();
}

inline unsigned long ResiliencePersistence::getResilienceResultsFreeSpace( size_t size, unsigned char &extraSize ) { 
    int * ResilienceInfo = ( int * ) _resilienceResults;
    if( size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) 
        fatal( "Not enough space in results file." );

    _resilienceResultsLock.acquire();

    //std::cerr << "Trying to reserve " << size << " bytes in results. Current _freeResilienceResults is "
    //    << _freeResilienceResults << "." << std::endl;

    if( _freeResilienceResults == 0 )
        fatal( "No space remaining in results file." );
    if( (size_t) _freeResilienceResults >= _RESILIENCE_RESULTS_MAX_FILE_SIZE )
        fatal( "No space remaining in results file (1)." );

    ResultsNode * results = ( ResultsNode * ) getResilienceResults( _freeResilienceResults );
    FreeInfo current_info = results->free_info;

    // Forcing the minimum size to be, at least, sizeof(ResultsNode). Otherwise, it couldn't be freed afterwards.
    extraSize = 0;

    if( size < sizeof(ResultsNode) ) {
        extraSize = sizeof(ResultsNode) - size;
        size = sizeof(ResultsNode);
    }

    if( current_info.size == size ) {
        // /* DEBUG 
        //std::cerr << "Chunk of the exactly requested size." << std::endl;
        // DEBUG */
        // Chunk of the exactly requested size: just point the head of free to the next one.
        unsigned long res = _freeResilienceResults;
        FreeInfo debug_info = results->free_info;
        if( results->free_info.next_free == 0 ) {
            results->free_info.next_free = 
                ( _freeResilienceResults + size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : _freeResilienceResults + size;
        }
        // /* DEBUG 
        //if( _freeResilienceResults == results->free_info.next_free ) {
        //    std::cerr << "1. _freeResilienceResults updated from " << _freeResilienceResults << " to " << results->free_info.next_free << "." << std::endl;
        //    std::cerr << "debug_info: {" << debug_info.next_free << ", " << debug_info.size << "}." << std::endl;
        //}
        // DEBUG */
        _freeResilienceResults = results->free_info.next_free; 

        // Prepare the next node.
        results = ( ResultsNode * ) getResilienceResults( _freeResilienceResults );
        if( results != NULL ) {
            if( results->free_info.next_free == 0 && results->free_info.size == 0 ) {
                results->free_info.size = _RESILIENCE_RESULTS_MAX_FILE_SIZE - _freeResilienceResults;
                // /* DEBUG 
                //std::cerr << "1. Results[" << _freeResilienceResults << "] = {" << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl;
                // DEBUG */
            }
        }

        // /*DEBUG
        //FreeInfo aux;
        //if( results != NULL )
        //    aux = results->free_info;
        //else {
        //    aux.next_free = 0;
        //    aux.size = 0;
        //}
        //int new_head = 
        //        ( _freeResilienceResults + size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : _freeResilienceResults + size;

        //std::cerr << "Chunk given starts at " << res << " and ends at " << res + size << ". " << std::endl;
        //    << "New _freeResilienceResults is " << _freeResilienceResults << " that should be equal to " << new_head << "."
        //    << "Results[" << _freeResilienceResults << "]->free_info: {" << aux.next_free
        //    << ", " << aux.size << "} that should be equal to {"
        //    << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl; 
        // DEBUG*/

        unsigned long * ResilienceInfo2 = ( unsigned long * ) &ResilienceInfo[2];
        ResilienceInfo2[0] = _freeResilienceResults;
        _resilienceResultsLock.release();
        if( (size_t) res+size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) { 
            std::cerr << "Giving index " << res+size << " that is bigger than " << _RESILIENCE_RESULTS_MAX_FILE_SIZE
                << "." << std::endl;
            fatal( "Chunk bigger than requested size: Giving more space than existent." );
        }
        extraSize += current_info.size - size;
        return res; 
    }
    else if( current_info.size > size ) {
        /*  Chunk bigger than requested size: 
         ** 1- Point the head of free to the end of the requested chunk, in other words, current head + size requested, if there is enough space.
         ** 2- Resize the current chunk, substracting the size requested. 
         */
        //std::cerr << "Chunk bigger than requested size." << std::endl;
        unsigned long res = _freeResilienceResults;

        /* If there is enough space to put a ResultsNode after the requested size, new head is current head + size requested. */
        if( size + sizeof(ResultsNode) <= current_info.size ) {
            /* Set next_free if it is no already setted. */
            // /*DEBUG
            //FreeInfo prev = results->free_info;
            // DEBUG*/
            if( results->free_info.next_free == 0 || results->free_info.next_free < res + current_info.size ) {
                results->free_info.size = current_info.size; 
            }
            FreeInfo aux = results->free_info;
            // /*DEBUG
            //if( aux.size > _RESILIENCE_RESULTS_MAX_FILE_SIZE || aux.next_free > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) {
            //    std::cerr << "prev: {" << prev.next_free << ", " << prev.size << "}." << std::endl;
            //    std::cerr << "aux: {" << aux.next_free << ", " << aux.size << "}." << std::endl;
            //    std::cerr << "Why?" << std::endl;
            //}
            // DEBUG*/
            
            /* Update head. */
            // /*DEBUG
            //if( _freeResilienceResults == res+size )
            //    std::cerr << "2. _freeResilienceResults updated from " << _freeResilienceResults << " to " << res+size << "." << std::endl;
            // DEBUG*/
            _freeResilienceResults = 
                ( res + size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : res + size; 

            /* Prepare next results node. */
            results = ( ResultsNode * ) getResilienceResults( _freeResilienceResults );
            if( results!= NULL ) {
                // /*DEBUG
                //if( aux.next_free == _freeResilienceResults ) 
                //    std::cerr << "Cycle." << std::endl;
                // DEBUG*/
                results->free_info.next_free = aux.next_free;
                results->free_info.size = current_info.size - size;
                // /*DEBUG
                //std::cerr << "2. Results[" << _freeResilienceResults << "] = {" << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl;
                // DEBUG*/
            }
            // /*DEBUG
            //std::cerr << "Bigger than requested->if. " << size << " bytes requested and " << size << " bytes given." << std::endl;
            // DEBUG*/

            // /* DEBUG
            //FreeInfo aux_info;
            //if( results != NULL )
            //    aux_info = results->free_info;
            //else {
            //    aux_info.next_free = 0;
            //    aux_info.size = 0;
            //}
            //int new_head = res+size; 

            //std::cerr << "Chunk given starts at " << res << " and ends at " << res + size << ". " << std::endl;
            //    << "New _freeResilienceResults is " << _freeResilienceResults << " that should be equal to " << new_head << "."
            //    << "Results[" << _freeResilienceResults << "]->free_info: {" << aux_info.next_free
            //    << ", " << aux_info.size << "} that should be equal to {"
            //    << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl; 
            // DEBUG */

            extraSize += 0;
        }
        /*  If there is not enough space to put a ResultsNode after the requested size (example: There are 24 bytes avalaible. The requested size is 16 bytes,
         *  so there only 8 remaining. A ResultsNode needs 16 bytes, so if a ResultsNode is inserted here, it will overwrite already allocated space. )
         *  We jump to current head + chunk size and put the ResultsNode in the next free node.
         */
        else { 
            /* Set next_free if it is no already setted. */
            if( results->free_info.next_free == 0 ) {
                results->free_info.next_free = 
                    ( _freeResilienceResults + current_info.size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 
                      0 : _freeResilienceResults + current_info.size;
            }

            /* Update head. */
            // /* DEBUG
            //if( _freeResilienceResults == results->free_info.next_free )
            //    std::cerr << "3. _freeResilienceResults updated from " << _freeResilienceResults << " to " << results->free_info.next_free << "." << std::endl;
            // DEBUG */
            _freeResilienceResults = results->free_info.next_free;

            /* Prepare next results node. */
            results = ( ResultsNode * ) getResilienceResults( _freeResilienceResults );
            if( results != NULL ) {
                if( results->free_info.next_free == 0 ) {
                    results->free_info.size = _RESILIENCE_RESULTS_MAX_FILE_SIZE - _freeResilienceResults;
                    // /* DEBUG
                    //std::cerr << "3. Results[" << _freeResilienceResults << "] = {" << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl;
                    // DEBUG */
                }
            }
            // /* DEBUG
            //std::cerr << "Bigger than requested->else. " << size << " bytes requested and " << current_info.size << " bytes given." << std::endl;
            // DEBUG */

            // /* DEBUG
            //FreeInfo aux_info;
            //if( results != NULL )
            //    aux_info = results->free_info;
            //else {
            //    aux_info.next_free = 0;
            //    aux_info.size = 0;
            //}
            //int new_head =  
            //        ( _freeResilienceResults + current_info.size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : _freeResilienceResults + current_info.size;

            //std::cerr << "Chunk given starts at " << res << " and ends at " << res + size << ". " << std::endl;
            //    << "New _freeResilienceResults is " << _freeResilienceResults << " that should be equal to " << new_head << "."
            //    << "Results[" << _freeResilienceResults << "]->free_info: {" << aux_info.next_free
            //    << ", " << aux_info.size << "} that should be equal to {"
            //    << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl; 
            // DEBUG */

            extraSize += current_info.size - size;
        }


        unsigned long * ResilienceInfo2 = ( unsigned long * ) &ResilienceInfo[2];
        ResilienceInfo2[0] = _freeResilienceResults;
        _resilienceResultsLock.release();
        if( (size_t) res+size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) { 
            std::cerr << "Giving index " << res+size << " that is bigger than " << _RESILIENCE_RESULTS_MAX_FILE_SIZE
                << "." << std::endl;
            std::cerr << "_freeResilienceResults is " << res << " and has space for " << current_info.size << std::endl;
            fatal( "Chunk bigger than requested size: Giving more space than existent." );
        }
        return res; 
    }
    else {
        /*  Chunk smaller than requested size:
         ** 1- Iterate through the list until find one chunk with enough size.
         ** 2- Remove the used chunk of the list and link the previous with the next. Example: 0->2->4 to 0->4
         */
        //std::cerr << "Chunk smaller than requested size." << std::endl;
        int current = _freeResilienceResults;
        int next = current_info.next_free;
        ResultsNode * next_results = ( ResultsNode * ) getResilienceResults( next );
        FreeInfo next_info;
        next_info.next_free = 0;
        next_info.size = 0;

        /* Set next_free if it is no already setted. */
        if( next_results != NULL ) {
            if( next_results->free_info.next_free == 0 && next_results->free_info.size == 0 ) {
                next_results->free_info.next_free = 
                    ( next + sizeof(ResultsNode) >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : next + sizeof(ResultsNode);
                next_results->free_info.size = next_results->free_info.next_free - next;
                // /* DEBUG
                //std::cerr << "4. Results[" << next << "] = {" << next_results->free_info.next_free << ", " << next_results->free_info.size << "}." << std::endl;
                // DEBUG */
            }
            next_info = next_results->free_info; 
        }

        if( next_info.size < size && next_info.next_free == 0 )
            fatal( "Cannot reserve such space in results" );

        while( next_info.size < size && next_info.next_free != 0 ) {
            current = next;
            next = next_info.next_free;
            next_results = ( ResultsNode * ) getResilienceResults( next );
            if( next_results == NULL ) {
                std::stringstream ss;
                ss << "This should not happen. Next: " << next << std::endl;
                fatal( ss.str() );
            }
            if( next_results->free_info.next_free == 0  && next_results->free_info.size == 0 ) {
                next_results->free_info.next_free = 
                    ( next + sizeof(ResultsNode) >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : next + sizeof(ResultsNode);
                next_results->free_info.size = next_results->free_info.next_free - next; 
                // /* DEBUG
                //std::cerr << "5. Results[" << next << "] = {" << next_results->free_info.next_free << ", " << next_results->free_info.size << "}." << std::endl;
                // DEBUG */
            }
                // TODO: FIXME: DEFRAGMENTATE MEMORY
            next_info = next_results->free_info;
        }

        if( next_info.size < size + sizeof(ResultsNode) ) {
            ResultsNode * current_results = ( ResultsNode * ) getResilienceResults( current );
            if( current_results != NULL ) {
                current_results->free_info.next_free = next_info.next_free; 
                // /* DEBUG
                //std::cerr << "6. Results[" << current << "] = {" << current_results->free_info.next_free << ", " << current_results->free_info.size << "}." << std::endl;
                // DEBUG */
            }
            else 
                fatal( "This should not happen. Current cannot be null." );

            extraSize += next_info.size - size;
            // /* DEBUG
            //std::cerr << "Smaller than requested->if. " << size << " bytes requested and " << next_info.size << " bytes given." << std::endl;
            //FreeInfo aux_info;
            //if( current_results != NULL )
            //    aux_info = current_results->free_info;
            //else {
            //    aux_info.next_free = 0;
            //    aux_info.size = 0;
            //}

            //std::cerr << "Chunk given starts at " << next << " and ends at " << next + size << ". " << std::endl;
            //    << "Results[" << current  << "]->free_info: {" << aux_info.next_free
            //    << ", " << aux_info.size << "} that should be equal to {"
            //    << next_info.next_free << ", " << next_info.size << "}." << std::endl; 
            // DEBUG */

        }
        else {
            ResultsNode * current_results = ( ResultsNode * ) getResilienceResults( current );
            // /* DEBUG
            //FreeInfo info = current_results->free_info;
            // DEBUG */
            ResultsNode * new_results;
            if( current_results != NULL ) {
                current_results->free_info.next_free = next + size; 
                new_results = ( ResultsNode * ) getResilienceResults( next + size );
                new_results->free_info.next_free = next_info.next_free;
                new_results->free_info.size = next_info.size - size;
                // /* DEBUG
                //std::cerr << "7. Results[" << current << "] = {" << current_results->free_info.next_free << ", " << current_results->free_info.size << "}." << std::endl;
                //std::cerr << "8. Results[" << next+size << "] = {" << new_results->free_info.next_free << ", " << new_results->free_info.size << "}." << std::endl;
                //if( new_results->free_info.next_free > _RESILIENCE_RESULTS_MAX_FILE_SIZE || new_results->free_info.size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) {
                //    std::cerr << "info: {" << info.next_free << ", " << info.size << "}." << std::endl;
                //    std::cerr << "new_results->free_info: {" << new_results->free_info.next_free << ", " << new_results->free_info.size << "}." << std::endl;
                //    std::cerr << "Why? (2)" << std::endl;
                //}
                // DEBUG */
            }
            else 
                fatal( "This should not happen. Current cannot be null." );

            extraSize += 0;
            // /* DEBUG
            //std::cerr << "Smaller than requested->else. " << size << " bytes requested and " << size << " bytes given." << std::endl;
            //FreeInfo aux_info;
            //if( current_results != NULL )
            //    aux_info = current_results->free_info;
            //else {
            //    aux_info.next_free = 0;
            //    aux_info.size = 0;
            //}
            //FreeInfo aux_info2;
            //if( new_results != NULL )
            //    aux_info2 = new_results->free_info;
            //else {
            //    aux_info2.next_free = 0;
            //    aux_info2.size = 0;
            //}

            //std::cerr << "Chunk given starts at " << next << " and ends at " << next + size << ". " << std::endl;
            //    << "Results[" << current  << "]->free_info: {" << aux_info.next_free
            //    << ", " << aux_info.size << "} that should be equal to {"
            //    << next+size << ", " << aux_info.size << "} and Results[" << next + size << "]->free_info: {"
            //    << aux_info2.next_free << ", " << aux_info2.size << "} that should be equal to {" 
            //    << next_info.next_free << ", " << next_info.size - size << "}." << std::endl; 
            // DEBUG */

        }

        _resilienceResultsLock.release();
        if( (size_t) next + size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) { 
            std::cerr << "Given space starts at " << next << " and ends at " << next+size << " that is bigger than " 
                << _RESILIENCE_RESULTS_MAX_FILE_SIZE << "." << std::endl;
            fatal( "Chunk smaller than requested size: Giving more space than existent." );
        }
        return next; 
    }


    //if( size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) { 
    //    //std::cerr << "Not enough space in results file" << std::endl;
    //    fatal( "Not enough space in results file." ); 
    //}

    //_resilienceResultsLock.acquire(); 

    //if( _freeResilienceResults.size() == 0 ) {
    //    _resilienceResultsLock.release(); 
    //    //std::cerr << "Not enough space in results file (1)" << std::endl;
    //    //std::cerr << "Trying to reserve " << size << " bytes." << std::endl;
    //    fatal( "Not enough space in results file." ); 
    //}

    //for( std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.begin();
    //        it != _freeResilienceResults.end();
    //        it++ ) 
    //{
    //    if( size < it->second ) {
    //        std::pair<unsigned int, size_t> p( it->first, it->second );
    //        _freeResilienceResults.erase( it );
    //        _freeResilienceResults.insert( std::make_pair( p.first + size, p.second - size ) );
    //        //message( "There are " << p.second-size << " bytes free starting at position " << p.first+size );
    //        ////std::cerr << size << " bytes reserved. From " << p.first << " to " << p.first+size << std::endl;
    //        _resilienceResultsLock.release(); 
    //        return getResilienceResults( p.first );
    //    }
    //    else if( size == it->second ) {
    //        std::pair<unsigned int, size_t> p( it->first, it->second );
    //        _freeResilienceResults.erase( it );
    //        //message( "There is no more free space in results file." );
    //        ////std::cerr << size << " bytes reserved. From " << p.first << " to " << p.first+size << std::endl;
    //        _resilienceResultsLock.release(); 
    //        return getResilienceResults( p.first );
    //    }
    //}

    //_resilienceResultsLock.release(); 
    ////std::cerr << "Trying to reserve " << size << " bytes. But there are only " << _freeResilienceResults.begin()->second << " bytes free." << std::endl;
    //fatal( "Cannot reserve such space in results." );
}

inline void * ResiliencePersistence::getResilienceResults( unsigned long offset ) { 
    if( offset < 2*sizeof(int)+sizeof(unsigned long) || offset >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) return NULL;
    return ( char * )_resilienceResults + offset;
}

inline void ResiliencePersistence::freeResilienceResultsSpace( unsigned long offset, size_t size ) {
    if( size == 0 ) return;
    if( size < sizeof(ResultsNode) ) 
        fatal( "All chunks are forced to be, at least, of sizeof(ResultsNode)." );

    int * ResilienceInfo = ( int * ) _resilienceResults;
    _resilienceResultsLock.acquire();

    // /* DEBUG
    //std::cerr << "Freeing chunk of size " << size << " starting at " << offset << " and ending at " << offset+size << std::endl;
    // DEBUG */

    memset( getResilienceResults( offset ), 0, size ); 
    ResultsNode * results = ( ResultsNode * ) getResilienceResults( offset );
    results->free_info.next_free = _freeResilienceResults;
    results->free_info.size = size;
    // /* DEBUG
    //std::cerr << "9. Results[" << offset << "] = {" << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl;
    //if( _freeResilienceResults == offset )
    //    std::cerr << "4. _freeResilienceResults updated from " << _freeResilienceResults << " to " << offset << "." << std::endl;
    // DEBUG */
    _freeResilienceResults = offset;
    unsigned long * ResilienceInfo2 = ( unsigned long * ) &ResilienceInfo[2];
    ResilienceInfo2[0] = _freeResilienceResults;
    
    // /* DEBUG
    //std::cerr << "Chunk freed. Next_free is " << free.next_free << " with size " << free.size << "." << std::endl;
    // DEBUG */

    _resilienceResultsLock.release();


    //_resilienceResultsLock.acquire();

    //// TODO: FIXME: This check is not enough to ensure this space is not already freed. I should iterate over the map to ensure that.
    //if( _freeResilienceResults.find( offset ) != _freeResilienceResults.end() )
    //    fatal( "Already freed results space.");
    //_freeResilienceResults.insert( std::make_pair( offset, size ) );
    //// Join consecutive chunks.
    //for( std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.begin();
    //        it != _freeResilienceResults.end();
    //        it++ )
    //{
    //    std::map<unsigned int, size_t>::iterator next = _freeResilienceResults.find( it->first + it->second );
    //    if( next != _freeResilienceResults.end() ) {
    //        it->second += next->second;
    //        _freeResilienceResults.erase( next );
    //    }
    //}

    //////std::cerr << size << " bytes freed. From " << offset << " to " << offset+size << std::endl;
    //_resilienceResultsLock.release();
}

//inline void ResiliencePersistence::restoreResilienceResultsSpace( unsigned int offset, size_t size ) {
//    std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.find( offset );
//    if( it != _freeResilienceResults.end() ) {
//        if( size > it->second ) {
//            //std::cerr << "Cannot reserve such space in this position. Requesting " << size << " bytes, but there are only "
//                << it->second << " bytes avalaible." << std::endl;
//            fatal0( "Cannot reserve such space in this position." );
//        }
//        else if( size == it->second ) {
//            _freeResilienceResults.erase( it );
//            ////std::cerr << size << " bytes restored. From " << offset << " to " << offset+size << std::endl;
//        }
//        else {
//            std::pair<unsigned int, size_t> p( it->first, it->second );
//            _freeResilienceResults.erase( it );
//            _freeResilienceResults.insert( std::make_pair( p.first + size, p.second - size ) );
//            ////std::cerr << size << " bytes restored. From " << offset << " to " << offset+size << std::endl;
//        }
//    }
//    else {
//        for( it = _freeResilienceResults.begin();
//                it != _freeResilienceResults.end();
//                it++ )
//        {
//            if( offset < it->first ) 
//                break;
//
//            if( offset + size == it->first + it->second ) {
//                std::pair<unsigned int, size_t> p( it->first, it->second );
//                _freeResilienceResults.erase( it );
//                _freeResilienceResults.insert( std::make_pair( p.first, offset - p.first ) );
//                ////std::cerr << size << " bytes restored. From " << offset << " to " << offset+size << std::endl;
//                return;
//            }
//            else if( offset + size < it->first + it->second ) {
//                std::pair<unsigned int, size_t> p( it->first, it->second );
//                _freeResilienceResults.erase( it );
//                _freeResilienceResults.insert( std::make_pair( p.first, offset - p.first ) );
//                _freeResilienceResults.insert( std::make_pair( offset + size, p.second - size - ( offset - p.first ) ) );
//                ////std::cerr << size << " bytes restored. From " << offset << " to " << offset+size << std::endl;
//                return;
//            }
//        }
//        //std::cerr << "Cannot reserve such space in this position. Requesting " << size << " bytes in " << offset << std::endl;
//        for( it = _freeResilienceResults.begin();
//                it != _freeResilienceResults.end();
//                it++ )
//        {
//            //std::cerr << "There are " << it->second << " bytes free from " << it->first << " to " << it->first+it->second << std::endl;
//        }
//        fatal0( "Cannot reserve such space in this position." );
//    }
//}

inline bool ResilienceNode::isInUse() const { return _inUse; }
inline void ResilienceNode::setInUse( bool flag ) { _inUse = flag; }
inline bool ResilienceNode::isComputed() const { return _computed; }
inline unsigned long ResilienceNode::getResultIndex() const { return _resultIndex; }
inline size_t ResilienceNode::getResultSize() const { return _resultSize; }
inline size_t ResilienceNode::getResultSizeToFree() const { return _resultSize + _extraSize; }
inline unsigned int ResilienceNode::getNumDescendants() { return _descSize; }
inline void ResilienceNode::restartLastDescRestored() { _lastDescRestored = 0; }

#endif /* _NANOS_RESILIENCE_H */
