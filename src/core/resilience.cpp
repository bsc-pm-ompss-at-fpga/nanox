#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "resilience_decl.hpp"
#include "resilience.hpp"
#include "copydata.hpp"
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace nanos {

    /********** RESILIENCE PERSISTENCE ***********/

    ResiliencePersistence::ResiliencePersistence( int rank, size_t resilienceTreeFileSize, size_t resilienceResultsFileSize ) :
      _resilienceTree( NULL )
      , _resilienceTreeFileDescriptor( -1 )
      , _resilienceTreeFilepath( NULL )
      , _resilienceResults( NULL )
      , _resilienceResultsFileDescriptor( -1 )
      , _resilienceResultsFilepath( NULL )
    {
        if( resilienceTreeFileSize == 0 ) { 
            warning0( "Resilience tree filesize not defined. Using default filesize." );
            resilienceTreeFileSize = 1024*1024*sizeof(ResilienceNode);
        }
        if( resilienceResultsFileSize == 0 ) { 
            warning0( "Resilience results filesize not defined. Using default filesize." );
            resilienceResultsFileSize = 1024*1024*sizeof(ResilienceNode);
        }
        // Make resilience tree filesize multiple of sizeof(ResilienceNode).
        _RESILIENCE_TREE_MAX_FILE_SIZE = resilienceTreeFileSize % sizeof(ResilienceNode) ? 
            ( ( resilienceTreeFileSize/sizeof(ResilienceNode) ) + 1 ) * sizeof(ResilienceNode) : resilienceTreeFileSize;

        // Make resilience results filesize multiple of sizeof(ResultsNode).
        _RESILIENCE_RESULTS_MAX_FILE_SIZE = resilienceResultsFileSize % sizeof(ResultsNode) ?
            ( ( resilienceResultsFileSize/sizeof(ResultsNode) ) + 1 ) * sizeof(ResultsNode) : resilienceResultsFileSize;
        // Add size for _firstFreeResilienceNode(int), _firstUsedResilienceNode(int), _freeResilienceResults(unsigned long).
        _RESILIENCE_RESULTS_MAX_FILE_SIZE += 2*sizeof(int)+sizeof(unsigned long);

        /* Get path of executable file. With this path, the files for store the persistent resilience tree and the persistent resilience results are created 
         * with the same name of the executable, in $TMPDIR, terminating with ".tree" and ".results" respectively. If there is MPI, before the termination
         * is added the rank.
         * Example without MPI: ./example generates $TMPDIR/example.tree and $TMPDIR/example.results
         * Example with MPI (rank 0) : ./example generates $TMPDIR/example.0.tree and $TMPDIR/example.0.results
         */

        // Get path of executable file.
        _restoreTree = true;
        char link[32];
        sprintf( link, "/proc/%d/exe", getpid() );
        char path[256];
        int pos = readlink ( link, path, sizeof( path ) );
        if( pos <= 0 )
           fatal0( "Resilience: Error getting path of executable file" );

        // Get $TMPDIR
        char * tmpdir = getenv( "TMPDIR" );
        if( tmpdir == NULL )
           fatal0( "Resilience: Cannot find tmp directory." );

        // From the path of executable file, get just the name.
        std::string aux( path, pos ); 
        std::string name( tmpdir );
        //std::string name( "/tmp/" );
        name.append( aux.substr( aux.find_last_of( '/' ) ) );

        // Construct _resilienceTreeFilepath and _resilienceResultsFilepath
        _resilienceTreeFilepath = NEW char[name.length()+8+sizeof(".tree")];
        _resilienceResultsFilepath = NEW char[name.length()+8+sizeof(".results")];
        memset( _resilienceTreeFilepath, 0, name.length()+8+sizeof(".tree") );
        memset( _resilienceResultsFilepath, 0, name.length()+8+sizeof(".results") );
        strcpy( _resilienceTreeFilepath, name.c_str() );
        strcpy( _resilienceResultsFilepath, name.c_str() );

        // Append MPI_Rank if needed.
        if( rank != -1 ) {
            sprintf( _resilienceTreeFilepath, "%s.%d", _resilienceTreeFilepath, rank );
            sprintf( _resilienceResultsFilepath, "%s.%d", _resilienceResultsFilepath, rank );
        }

        // Append termination depending of the file content.
        strcat( _resilienceTreeFilepath, ".tree" );
        strcat( _resilienceResultsFilepath, ".results" );

        //Resilience tree mmapped file must be created before any WD because in WD constructor we will use the memory mapped.
        if( _resilienceTree == NULL ) {
            // Check if we have a file with the resilience tree from past executions.
            _resilienceTreeFileDescriptor = open( _resilienceTreeFilepath, O_RDWR );

            //If there isn't a file from past executions.
            if( _resilienceTreeFileDescriptor == -1 ) {
                _restoreTree = false;
                _resilienceTreeFileDescriptor = open( _resilienceTreeFilepath, O_RDWR | O_CREAT, (mode_t) 0600 );
                if( _resilienceTreeFileDescriptor == -1 )
                    fatal0( "Resilience: Error creating persistentTree." );

                //Stretch file.
                int res = lseek( _resilienceTreeFileDescriptor, _RESILIENCE_TREE_MAX_FILE_SIZE - 1, SEEK_SET );
                if( res == -1 ) {
                    close( _resilienceTreeFileDescriptor );
                    fatal0( "Resilience: Error calling lseek." );
                }

                /* Something needs to be written at the end of the file to force the file have actually the new size.
                 * Just writing an empty string at the current file position will do.
                 *
                 * Note:
                 *  - The current position in the file is at the end of the stretched file due to the call to lseek().
                 *  - An empty string is actually a single '\0' character, so a zero-byte will be written at the last byte of the file.
                 */
                res = write(_resilienceTreeFileDescriptor, "", 1);
                if (res != 1) {
                    close( _resilienceTreeFileDescriptor );
                    fatal0( "Error resizing file" );
                }
            }

            //Now the file is ready to be mmaped.
            _resilienceTree = ( ResilienceNode * ) mmap( 0, _RESILIENCE_TREE_MAX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _resilienceTreeFileDescriptor, 0 );
            if( _resilienceTree == MAP_FAILED ) {
                close( _resilienceTreeFileDescriptor );
                fatal0( "Error mmaping file" );
            }
        }

        //Resilience results mmapped file must be created before any WD because in WD constructor we will use the memory mapped.
        if( _resilienceResults == NULL ) {
            // Check if we have a file with the resilience tree from past executions.
            _resilienceResultsFileDescriptor = open( _resilienceResultsFilepath, O_RDWR );

            //If there isn't a file from past executions.
            if( _resilienceResultsFileDescriptor == -1 ) {
                _resilienceResultsFileDescriptor = open( _resilienceResultsFilepath, O_RDWR | O_CREAT, (mode_t) 0600 );
                if( _resilienceResultsFileDescriptor == -1 )
                    fatal0( "Resilience: Error creating persistentTree." );

                //Stretch file.
                int res = lseek( _resilienceResultsFileDescriptor, _RESILIENCE_RESULTS_MAX_FILE_SIZE - 1, SEEK_SET );
                if( res == -1 ) {
                    close( _resilienceResultsFileDescriptor );
                    fatal0( "Resilience: Error calling lseek." );
                }

                /* Something needs to be written at the end of the file to force the file have actually the new size.
                 * Just writing an empty string at the current file position will do.
                 *
                 * Note:
                 *  - The current position in the file is at the end of the stretched file due to the call to lseek().
                 *  - An empty string is actually a single '\0' character, so a zero-byte will be written at the last byte of the file.
                 */
                res = write(_resilienceResultsFileDescriptor, "", 1);
                if (res != 1) {
                    close( _resilienceResultsFileDescriptor );
                    fatal0( "Error resizing file" );
                }
            }

            //Now the file is ready to be mmaped.
            _resilienceResults = ( char * ) mmap( 0, _RESILIENCE_RESULTS_MAX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _resilienceResultsFileDescriptor, 0 );
            if( _resilienceResults == MAP_FAILED ) {
                close( _resilienceResultsFileDescriptor );
                fatal0( "Error mmaping file" );
            }
        }

        _checkpoint = NULL;

        //Update _freeResilienceResults with the restored tree.
        if( !_restoreTree ) {
            int * init = ( int * ) _resilienceResults;
            init[0] = 1; 
            init[1] = 0; 
            unsigned long * init2 = ( unsigned long * ) &init[2];
            init2[0] = 2*sizeof(int) + sizeof(unsigned long);
            ResultsNode * results = ( ResultsNode * ) getResilienceResults( init2[0] );
            results->free_info.next_free = 0;
            results->free_info.size = _RESILIENCE_RESULTS_MAX_FILE_SIZE - 2*sizeof(int) + sizeof(unsigned long);
        }
        //else {
        //    int * init = ( int * ) _resilienceResults; 
        //    int current = init[1];
        //    while( current != 0 ){
        //        ResilienceNode * rn = getResilienceNode( current );
        //        rn->restartLastDescRestored();
        //        current = rn->_next;
        //    }
        //}

        //if( sys.printResilienceInfo() )
        //    printResilienceInfo();
    }

    ResiliencePersistence::~ResiliencePersistence() {
        //std::cerr << "Destroying resilience persistence." << std::endl;
        while( sys._resilienceCriticalRegion.value() ) {}
        //Free mapped file for resilience tree.
        int res = munmap( _resilienceTree, _RESILIENCE_TREE_MAX_FILE_SIZE );
        if( res == -1 )
            fatal( "Error unmapping file." );
        close( _resilienceTreeFileDescriptor );

        //Free mapped file for resilience results.
        res = munmap( _resilienceResults, _RESILIENCE_RESULTS_MAX_FILE_SIZE + 2*sizeof(int) + sizeof(unsigned long) );
        if( res == -1 )
            fatal( "Error unmapping file." );
        close( _resilienceResultsFileDescriptor );

        //Remove resilience files.
        if( sys.removeResilienceFiles() ) {
            res = remove( _resilienceTreeFilepath );
            if( res != 0 )
                fatal( "Error removing file." );
            res = remove( _resilienceResultsFilepath );
            if( res != 0 )
                fatal( "Error removing file." );
        }
    }

    void ResiliencePersistence::removeAllDescs( ResilienceNode * rn ) {
        ResilienceNode * current = getResilienceNode( rn->_desc );
        while( current != NULL ) {
            int toDelete = rn->_desc;
            removeAllDescs( current );
            rn->_desc = current->_sibling;
            current = getResilienceNode( current->_sibling );
            freeResilienceResultsSpace(
                    getResilienceNode( toDelete )->getDataIndex(),
                    getResilienceNode( toDelete )->getDataSizeToFree() 
                    );
            freeResilienceNode( toDelete );
            rn->_descSize--;
        }

        rn->_descSize = 0;
        //if( rn->_descSize != 0 || rn->_desc != 0 )
        //    fatal0( "There are still descs" );
    }

    unsigned long ResiliencePersistence::defragmentateResultsSpace( size_t size ) {
        std::cerr << "Defragmentation." << std::endl;
        std::map<unsigned long, size_t> free_space;
        int * init = ( int * ) _resilienceResults;
        unsigned long * init2 = ( unsigned long * ) &init[2];
        unsigned long current = init2[0]; 

        // Insert all free ResultsNode in a map to get them ordered.
        while( current != 0 ) {
            ResultsNode * results = ( ResultsNode * ) getResilienceResults( current );
            if( results == NULL )
                break;
            free_space[current] = results->free_info.size;
            current = results->free_info.next_free; 
        }

        //printResilienceInfo();

        // Iterate through the map trying to join consecutive chunks.
        for( std::map<unsigned long, size_t>::iterator it = free_space.begin(); it != free_space.end(); it++ ) {
            std::map<unsigned long, size_t>::iterator consecutive = free_space.find( it->first + it->second );
            std::map<unsigned long, size_t>::iterator current_it = it;
            if( consecutive != free_space.end() ) {
                it->second += consecutive->second;
                free_space.erase( consecutive );
            }
            it++;
            ResultsNode * results = ( ResultsNode * ) getResilienceResults( current_it->first );
            results->free_info.next_free = it != free_space.end() ? it->first : 0;
            results->free_info.size = current_it->second;
            it = current_it;
        }

        init2[0] = free_space.begin()->first;

        //printResilienceInfo();
        // Call getResilienceResultsFreeSpace again to get space if possible after defragmentation.
        return getResilienceResultsFreeSpace( size, true );
    }

    void ResiliencePersistence::printResilienceInfo() {
        int * init = ( int * ) _resilienceResults;
        unsigned long * init2 = ( unsigned long * ) &init[2];
        size_t current = init[1]; 
        size_t used_space = 0;
        while( current != 0 ) {
            ResilienceNode * rn = getResilienceNode( current );
            used_space += rn->getDataSizeToFree(); 
            if( current == (size_t) rn->_next )
                fatal0( "Cycle in UsedResilienceNode linked list." );
            current = rn->_next;
        }

        current = init2[0]; 
        size_t free_space = 0;
        while( current != 0 ) {
            // /* DEBUG
            //std::cerr << "Current is " << current << ". Free space is " << free_space << "." << std::endl;
            // DEBUG */
            ResultsNode * results = ( ResultsNode * ) getResilienceResults( current );
            if( results == NULL )
                break;
            // /* DEBUG
            //std::cerr << current << "->free_info.size is " << results->free_info.size << "." << std::endl;
            // DEBUG */
            free_space += results->free_info.size;
            current = results->free_info.next_free; 
        }

        //used_space += 2*sizeof(int) + sizeof(unsigned long);
        std::cerr << "USED SPACE IS " << used_space << " THAT SHOULD BE EQUAL TO " 
            << _RESILIENCE_RESULTS_MAX_FILE_SIZE - free_space << std::endl;
        std::cerr << "FREE SPACE IS " << free_space << " THAT SHOULD BE EQUAL TO " 
            << _RESILIENCE_RESULTS_MAX_FILE_SIZE - used_space << std::endl;
    }

    /********** RESILIENCE PERSISTENCE **********/
    

    /********** RESILIENCE NODE **********/

    bool ResilienceNode::addSibling( int sibling ) {
        if( sys.getResiliencePersistence()->getResilienceNode( sibling ) == this )
            fatal0( "There is a cycle. (2)" );

        if( _sibling == 0 ) {
            _sibling = sibling; 
            return true;
        }

        ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( _sibling );
        while( current->_sibling != 0 ) {
            current = sys.getResiliencePersistence()->getResilienceNode( current->_sibling );
        }
        current->_sibling = sibling; 
        return true;
    }

    void ResilienceNode::storeInput( CopyData * copies, size_t numCopies, int task_id ) {
        //Calculate inputs size only if it is not already calculated.
        if( _dataSize == 0 ) {
            size_t inputs_size = 0;
            for( unsigned int i = 0; i < numCopies; i++ ) {
                if( copies[i].isInput() ) {
                    inputs_size += copies[i].getDimensions()->accessed_length;
                }
            }
            if( inputs_size <= 0 )
                return;
            _dataSize = inputs_size;
        }

        //Reserve _dataSize bytes in results.
        if( _dataIndex == 0 )
            _dataIndex = sys.getResiliencePersistence()->getResilienceResultsFreeSpace( _dataSize );

        //Copy the result
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _dataIndex );
        if( aux == NULL )
            fatal0( "Something went wrong." );
        
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isInput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( aux, copy_address, copy_size );
                aux += copy_size;
            }
        }

        ResilienceNode * old_checkpoint = sys.getResiliencePersistence()->getCheckpoint();
        unsigned long index_to_free = 0;
        size_t size_to_free = 0;
        if( old_checkpoint != NULL ) {
            index_to_free = old_checkpoint->getDataIndex();
            size_to_free = old_checkpoint->getDataSizeToFree();
        }

        //Mark it as computed
        _computed = true;

        //Update checkpoint pointer to the current checkpoint and remove old_checkpoint data.
        // TODO: FIXME: Maybe this update should be atomic.
        sys.getResiliencePersistence()->setCheckpoint( this );
        if( old_checkpoint != NULL ) old_checkpoint->_dataIndex = 0;

        //Remove old_checkpoint descendants and free results space.
        if( old_checkpoint != NULL ) {
            old_checkpoint->removeAllDescs();
            sys.getResiliencePersistence()->freeResilienceResultsSpace( index_to_free, size_to_free );
        }

        //std::cout << "Rank " << sys._rank /*<< ". RN " << this - sys.getResiliencePersistence()->getResilienceNode(1) */<< 
        //   " storeInput of " << _dataSize << " bytes starting at " << _dataIndex << " completed." << std::endl;
    }

    void ResilienceNode::loadInput( CopyData * copies, size_t numCopies, int task_id ) { 
        //std::cerr << "RN " << this - sys.getResiliencePersistence()->getResilienceNode(1) << 
        //   " loadInput of " << _dataSize << " bytes starting at " << _dataIndex << "." << std::endl;
        if( sys.getResiliencePersistence()->getCheckpoint() == NULL )
            sys.getResiliencePersistence()->setCheckpoint( this );
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _dataIndex );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isInput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( copy_address, aux, copy_size );
                //std::cerr << "WD " << task_id << " with RN " << this << "loading " << copy_size << " bytes in " << copy_address << "." << std::endl;
                aux += copy_size;
            }
        }
    }

    void ResilienceNode::storeOutput( CopyData * copies, size_t numCopies, int task_id ) {
        //std::cerr << "RN " << this - sys.getResiliencePersistence()->getResilienceNode(1) << 
        //   " storeOutput of started." << std::endl;
        //Calculate outputs size only if it is not already calculated.
        if( _dataSize == 0 ) {
            size_t outputs_size = 0;
            for( unsigned int i = 0; i < numCopies; i++ ) {
                if( copies[i].isOutput() ) {
                    outputs_size += copies[i].getDimensions()->accessed_length;
                }
            }
            if( outputs_size <= 0 )
                return;
            _dataSize = outputs_size;
        }

        //Reserve _dataSize bytes in results.
        if( _dataIndex == 0 )
            _dataIndex = sys.getResiliencePersistence()->getResilienceResultsFreeSpace( _dataSize );

        //Copy the result
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _dataIndex );
        if( aux == NULL )
            fatal0( "Something went wrong." );

        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( aux, copy_address, copy_size );
                aux += copy_size;
            }
        }

        //Mark it as computed
        //_computed = true;

        //std::cerr << "RN " << this - sys.getResiliencePersistence()->getResilienceNode(1) << 
        //   " storeOutput of " << _dataSize << " bytes starting at " << _dataIndex << " completed." << std::endl;
    }

    void ResilienceNode::loadOutput( CopyData * copies, size_t numCopies, int task_id ) { 
        //std::cerr << "RANK " << sys._rank << " RN " << this - sys.getResiliencePersistence()->getResilienceNode(1) << 
        //   " loadOutput of " << _dataSize << " bytes starting at " << _dataIndex << "." << std::endl;
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _dataIndex );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( copy_address, aux, copy_size );
                aux += copy_size;
            }
        }
    }

    ResilienceNode* ResilienceNode::getNextDescToRestore() {
        //TODO: FIXME: Maybe, this should throw FATAL ERROR.
        if( _desc == 0 || _lastDescRestored >= _descSize ) {
            _lastDescRestored++;
            return NULL;
        }

        ResilienceNode * desc = sys.getResiliencePersistence()->getResilienceNode( _desc );
        for( unsigned int i = 0; i < _lastDescRestored; i++) {
            //TODO: FIXME: Maybe, this should throw FATAL ERROR.
            if( desc->_sibling == 0 ) {
                _lastDescRestored++;
                return NULL;
            }
            desc = sys.getResiliencePersistence()->getResilienceNode( desc->_sibling );
        }

        _lastDescRestored++;

        return desc;
    }

    bool ResilienceNode::addDesc( int desc ) { 
        if( desc == 0 )
            fatal0( "Trying to add null descendent." ); 

        if( sys.getResiliencePersistence()->getResilienceNode( desc ) == this )
            fatal0( "There is a cycle. (1)" );

        if( _desc == 0 )
            _desc = desc; 
        else
            sys.getResiliencePersistence()->getResilienceNode( _desc )->addSibling( desc );

        _descSize++;
        return true;
    }

    void ResilienceNode::removeAllDescs() {
        if( _descSize == 0 ) 
            return;
        //if( !_computed )
        //    fatal( "Removing descs without having result." );

        ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( _desc );
        while( current != NULL ) {
            int toDelete = _desc;
            current->removeAllDescs();
            _desc = current->_sibling;
            current = sys.getResiliencePersistence()->getResilienceNode( current->_sibling );
            sys.getResiliencePersistence()->freeResilienceResultsSpace(
                    sys.getResiliencePersistence()->getResilienceNode( toDelete )->getDataIndex(),
                    sys.getResiliencePersistence()->getResilienceNode( toDelete )->getDataSizeToFree() 
                    );
            sys.getResiliencePersistence()->freeResilienceNode( toDelete );
            _descSize--;
        }

        _descSize = 0;
        //if( _descSize != 0 || _desc != 0 )
        //    fatal0( "There are still descs" );
    }

    /********** RESILIENCE NODE **********/

}
