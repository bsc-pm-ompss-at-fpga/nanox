#ifndef _NANOS_RESILIENCE_H
#define _NANOS_RESILIENCE_H

#include "resilience_decl.hpp"
#include "system.hpp"

using namespace nanos;

inline ResilienceNode * ResiliencePersistence::getFreeResilienceNode( ResilienceNode * parent ) {
    int * ResilienceInfo = ( int * ) _resilienceResults;

    _resilienceTreeLock.acquire();
    // 0 is an invalid index. 
    if( ResilienceInfo[0] == 0 )
        fatal0( "Not enough space in tree file." );

    // Get first_free node and updatee first free to the next free node.
    int index_free = ResilienceInfo[0]; 
    int next = getResilienceNode( index_free )->_next;
    next = next ? next : index_free+1;
    ResilienceInfo[0] = ( next > (int) ( _RESILIENCE_TREE_MAX_FILE_SIZE/sizeof(ResilienceNode) ) ) ? 0 : next;

    // Sanity check
    ResilienceNode * res = getResilienceNode( index_free ); 
    if( res->_inUse ) {
        std::stringstream ss;
        ss << "Node " << index_free << " is already in use." << std::endl;
        fatal0( ss.str() );
    }

    // Set parent and mark node as used.
    if( parent != NULL )
        res->setInUse( parent->addDesc( index_free ) );
    else
        res->setInUse( true );

    // Get first_used and update first_used to the new first_used node.
    int index_used = ResilienceInfo[1]; 
    ResilienceInfo[1] = index_free;

    /**
      *  Pop from free_nodes list and push to used_nodes list. Three steps:
      *      1.  new_first_free->_prev = 0. (Now, this node is the head of free_nodes list, so it has not previous node.)
      **/
    ResilienceNode * next_free = getResilienceNode( next ); 
    if( next_free != NULL )
        next_free->_prev = 0;

    // 2. res->_next = old_first_used. (Now, this node is the head of used_nodes list, so it has not previous node.)
    if( index_free == index_used )
        fatal0( "getFreeRN: This is a cycle." );
    res->_next = index_used;

    // 3. old_first_used->_prev = res. (Now, this node is not the head and it substituted for res, so it has as previous node res.)
    ResilienceNode * next_used = getResilienceNode( index_used );
    if( next_used != NULL )
        next_used->_prev = index_free;

    _resilienceTreeLock.release();
    return res;
}

inline ResilienceNode * ResiliencePersistence::getResilienceNode( unsigned int offset ) { 
    if( offset < 1 || offset > _RESILIENCE_TREE_MAX_FILE_SIZE/sizeof(ResilienceNode) ) return NULL; 
    return _resilienceTree+offset-1;
}

inline void ResiliencePersistence::freeResilienceNode( unsigned int offset ) {
    int * ResilienceInfo = ( int * ) _resilienceResults;
    ResilienceNode * current = getResilienceNode( offset ); 
    ResilienceNode * next_used = getResilienceNode( current->_next ); 
    ResilienceNode * prev_used = getResilienceNode( current->_prev );
    int current_prev = current->_prev;
    int current_next = current->_next;
    if( current_next == current_prev ) 
        fatal( "freeRN: This is a cycle. (1)" );

    _resilienceTreeLock.acquire();

    // Clean Node
    memset( current, 0, sizeof(ResilienceNode) );

    // Check if the node to be freed is the head and update it to the next used if needed.
    if( ResilienceInfo[1] == (int) offset )
        ResilienceInfo[1] = current_next;

    // Update first_free node to the node beeing freed.
    int first_free = ResilienceInfo[0];
    if( first_free == (int) offset )
        fatal( "freeRN: This is a cycle. (2)" );
    ResilienceInfo[0] = offset;

    /**
      * Pop from used_nodes list and push to free_nodes list. Five steps required:
      *     1. current->_prev = 0. (Already done on cleaning.)
      *     2. current->_next = first_free.
      **/
    current->_next = first_free;

    // 3. next_used->_prev = current->_prev.
    if( next_used != NULL ) { 
        next_used->_prev = current_prev;
    }

    // 4. prev_used->_next = current->_next.
    if( prev_used != NULL ) {
        prev_used->_next = current_next;
    }

    // 5. first_free->_prev = current.
    ResilienceNode * first_free_node = getResilienceNode( first_free );
    if( first_free_node != NULL )
        first_free_node->_prev = offset;

    _resilienceTreeLock.release();
}

inline unsigned long ResiliencePersistence::getResilienceResultsFreeSpace( size_t size, bool avoidDefragmentation ) { 
    int * ResilienceInfo = ( int * ) _resilienceResults;
    unsigned long * ResilienceInfo2 = ( unsigned long * ) &ResilienceInfo[2];
    if( size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) 
        fatal( "Not enough space in results file." );

    if( !avoidDefragmentation )
        _resilienceResultsLock.acquire();

    if( ResilienceInfo2[0] == 0 )
        fatal( "No space remaining in results file." );
    if( ResilienceInfo2[0] >= _RESILIENCE_RESULTS_MAX_FILE_SIZE )
        fatal( "No space remaining in results file (1)." );

    ResultsNode * results = ( ResultsNode * ) getResilienceResults( ResilienceInfo2[0] );
    FreeInfo current_info = results->free_info;

    // Forcing the size to be multiple of sizeof(ResultsNode).
    if( size % sizeof(ResultsNode) )
        size = ( ( size/sizeof(ResultsNode) ) + 1 ) * sizeof(ResultsNode);

    if( current_info.size == size ) {
        // /* DEBUG 
        //std::cerr << "Chunk of the exactly requested size." << std::endl;
        //FreeInfo debug_info = results->free_info;
        // DEBUG */
        // Chunk of the exactly requested size: just point the head of free to the next one.
        unsigned long res = ResilienceInfo2[0];
        if( results->free_info.next_free == 0 ) {
            results->free_info.next_free = 
                ( ResilienceInfo2[0] + size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : ResilienceInfo2[0] + size;
        }
        // /* DEBUG 
        //if( _freeResilienceResults == results->free_info.next_free ) {
        //    std::cerr << "1. _freeResilienceResults updated from " << _freeResilienceResults << " to " << results->free_info.next_free << "." << std::endl;
        //    std::cerr << "debug_info: {" << debug_info.next_free << ", " << debug_info.size << "}." << std::endl;
        //}
        // DEBUG */
        ResilienceInfo2[0] = results->free_info.next_free;

        // Prepare the next node.
        results = ( ResultsNode * ) getResilienceResults( ResilienceInfo2[0] );
        if( results != NULL ) {
            if( results->free_info.next_free == 0 && results->free_info.size == 0 ) {
                results->free_info.size = _RESILIENCE_RESULTS_MAX_FILE_SIZE - ResilienceInfo2[0];
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

        if( !avoidDefragmentation )
            _resilienceResultsLock.release();
        if( (size_t) res+size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) { 
            std::cerr << "Giving index " << res+size << " that is bigger than " << _RESILIENCE_RESULTS_MAX_FILE_SIZE
                << "." << std::endl;
            fatal( "Chunk bigger than requested size: Giving more space than existent." );
        }
        return res; 
    }
    else if( current_info.size > size ) {
        /*  Chunk bigger than requested size: 
         ** 1- Point the head of free to the end of the requested chunk, in other words, current head + size requested, if there is enough space.
         ** 2- Resize the current chunk, substracting the size requested. 
         */
        //std::cerr << "Chunk bigger than requested size." << std::endl;
        unsigned long res = ResilienceInfo2[0];

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
        ResilienceInfo2[0] = ( res + size >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) ? 0 : res + size; 

        /* Prepare next results node. */
        results = ( ResultsNode * ) getResilienceResults( ResilienceInfo2[0] );
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

        if( !avoidDefragmentation )
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
        unsigned long current = ResilienceInfo2[0];
        unsigned long next = current_info.next_free;
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
            fatal( "Cannot reserve such space in results. (1)" );

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
            next_info = next_results->free_info;
        }

        // No chunk found for the requested size. Defragmentate memory.
        if( next_info.next_free == 0 && next_info.size < size ) { 
            if( avoidDefragmentation ) 
                fatal( "Cannot reserve such space in results. (2)" );
            next = defragmentateResultsSpace( size );
        }
        else {

            //if( next_info.size < size + sizeof(ResultsNode) ) {
            if( next_info.size == size ) {
                ResultsNode * current_results = ( ResultsNode * ) getResilienceResults( current );
                if( current_results != NULL ) {
                    current_results->free_info.next_free = next_info.next_free; 
                    // /* DEBUG
                    //std::cerr << "6. Results[" << current << "] = {" << current_results->free_info.next_free << ", " << current_results->free_info.size << "}." << std::endl;
                    // DEBUG */
                }
                else 
                    fatal( "This should not happen. Current cannot be null." );

                // /* DEBUG
                //std::cerr << "Smaller than requested->if. " << size << " bytes requested and " << next_info.size << " bytes given." << std::endl;
                //std::cerr << "Exactly equal than requested." << std::endl;
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
        }

        if( !avoidDefragmentation )
            _resilienceResultsLock.release();
        if( (size_t) next + size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) { 
            std::cerr << "Given space starts at " << next << " and ends at " << next+size << " that is bigger than " 
                << _RESILIENCE_RESULTS_MAX_FILE_SIZE << "." << std::endl;
            fatal( "Chunk smaller than requested size: Giving more space than existent." );
        }
        return next; 
    }
}

inline void * ResiliencePersistence::getResilienceResults( unsigned long offset ) { 
    if( offset < 2*sizeof(int)+sizeof(unsigned long) || offset >= _RESILIENCE_RESULTS_MAX_FILE_SIZE ) return NULL;
    return ( char * )_resilienceResults + offset;
}

inline void ResiliencePersistence::freeResilienceResultsSpace( unsigned long offset, size_t size ) {
    if( size == 0 || offset < 2*sizeof(int) + sizeof(unsigned long) ) return;
    if( offset + size > _RESILIENCE_RESULTS_MAX_FILE_SIZE ) return;
    if( size < sizeof(ResultsNode) ) 
        fatal( "All chunks are forced to be, at least, of sizeof(ResultsNode)." );

    int * ResilienceInfo = ( int * ) _resilienceResults;
    unsigned long * ResilienceInfo2 = ( unsigned long * ) &ResilienceInfo[2];
    _resilienceResultsLock.acquire();

    // /* DEBUG
    //std::cerr << "Freeing chunk of size " << size << " starting at " << offset << " and ending at " << offset+size << std::endl;
    // DEBUG */

    memset( getResilienceResults( offset ), 0, size ); 
    ResultsNode * results = ( ResultsNode * ) getResilienceResults( offset );
    results->free_info.next_free = ResilienceInfo2[0];
    results->free_info.size = size;
    // /* DEBUG
    //std::cerr << "9. Results[" << offset << "] = {" << results->free_info.next_free << ", " << results->free_info.size << "}." << std::endl;
    //if( _freeResilienceResults == offset )
    //    std::cerr << "4. _freeResilienceResults updated from " << _freeResilienceResults << " to " << offset << "." << std::endl;
    // DEBUG */
    ResilienceInfo2[0] = offset;
    
    // /* DEBUG
    //std::cerr << "Chunk freed. Next_free is " << free.next_free << " with size " << free.size << "." << std::endl;
    // DEBUG */

    _resilienceResultsLock.release();
}

inline void ResiliencePersistence::setCheckpoint( ResilienceNode * checkpoint ) { _checkpoint = checkpoint; }
inline ResilienceNode * ResiliencePersistence::getCheckpoint() { return _checkpoint; }

inline bool ResilienceNode::isInUse() const { return _inUse; }
inline void ResilienceNode::setInUse( bool flag ) { _inUse = flag; }
inline bool * ResilienceNode::getComputed() { return &_computed; }
inline bool ResilienceNode::isComputed() const { return _computed; }
inline unsigned long ResilienceNode::getDataIndex() const { return _dataIndex; }
inline size_t ResilienceNode::getDataSize() const { return _dataSize; }
inline size_t ResilienceNode::getDataSizeToFree() const 
{ return _dataSize % sizeof(ResultsNode) ? ( ( _dataSize/sizeof(ResultsNode) ) + 1 ) * sizeof(ResultsNode) : _dataSize; }
inline unsigned int ResilienceNode::getNumDescendants() { return _descSize; }
inline void ResilienceNode::restartLastDescRestored() { _lastDescRestored = 0; }

#endif /* _NANOS_RESILIENCE_H */
