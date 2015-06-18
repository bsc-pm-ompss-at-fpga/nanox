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

    /********** RESILIENCE PERSISTENCE **********/

    ResiliencePersistence::ResiliencePersistence( int rank, size_t resilienceTreeFileSize, size_t resilienceResultsFileSize ) :
      _resilienceTree( NULL )
      , _resilienceTreeFileDescriptor( -1 )
      , _resilienceTreeFilepath( NULL )
      , _resilienceResults( NULL )
      , _resilienceResultsFileDescriptor( -1 )
      , _resilienceResultsFilepath( NULL )
    {
        if( resilienceTreeFileSize == 0 ) { 
            message0( "Resilience tree filesize not defined. Using default filesize." );
            resilienceTreeFileSize = 1024*1024*sizeof(ResilienceNode);
        }
        if( resilienceResultsFileSize == 0 ) { 
            message0( "Resilience results filesize not defined. Using default filesize." );
            resilienceResultsFileSize = 1024*1024*sizeof(ResilienceNode);
        }
        // Make resilience tree filesize multiple of sizeof(ResilienceNode).
        if( resilienceTreeFileSize % sizeof(ResilienceNode) != 0 ) {
            _RESILIENCE_TREE_MAX_FILE_SIZE = resilienceTreeFileSize/sizeof(ResilienceNode);
            _RESILIENCE_TREE_MAX_FILE_SIZE *= sizeof(ResilienceNode);
            _RESILIENCE_TREE_MAX_FILE_SIZE += sizeof(ResilienceNode);
        }
        else {
            _RESILIENCE_TREE_MAX_FILE_SIZE = resilienceTreeFileSize;
        }

        _RESILIENCE_RESULTS_MAX_FILE_SIZE = resilienceResultsFileSize;

        /* Get path of executable file. With this path, the files for store the persistent resilience tree and the persistent resilience results are created 
         * with the same name of the executable, in $TMPDIR, terminating with ".tree" and ".results" respectively. If there is MPI, before the termination
         * is added the rank.
         * Example without MPI: ./example generates $TMPDIR/example.tree and $TMPDIR/example.results
         * Example with MPI (rank 0) : ./example generates $TMPDIR/example.0.tree and $TMPDIR/example.0.results
         */

        // Get path of executable file.
        bool restoreTree = true;
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
                restoreTree = false;
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
            _resilienceResults = ( ResilienceNode * ) mmap( 0, _RESILIENCE_RESULTS_MAX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _resilienceResultsFileDescriptor, 0 );
            if( _resilienceResults == MAP_FAILED ) {
                close( _resilienceResultsFileDescriptor );
                fatal0( "Error mmaping file" );
            }
            _freeResilienceResults.insert( std::make_pair( 0, _RESILIENCE_RESULTS_MAX_FILE_SIZE ) );
        }

        //Update _freeResilienceResults with the restored tree.

        size_t treeSize = _RESILIENCE_TREE_MAX_FILE_SIZE / sizeof( ResilienceNode );
        if( restoreTree ) {
            for( unsigned int i = 0; i < treeSize; i++ ) {
                ResilienceNode * rn = &_resilienceTree[i];
                if( rn->isInUse() ) {
                    _usedResilienceNodes.push_back( i );
                    if( rn->isComputed() ) {
                        //std::cerr << "Trying to restore " << rn->getResultSize() << " bytes." << std::endl;
                        restoreResilienceResultsSpace( rn->getResultIndex(), rn->getResultSize() );
                        if( rn->getNumDescendants() > 0 )
                            removeAllDescs( rn );
                    }
                    rn->restartLastDescRestored();
                }
                else {
                    _freeResilienceNodes.push( i );
                }
            }
        }
        else {
            for( unsigned int i = 0; i < treeSize; i++)
                _freeResilienceNodes.push( i );
        }

        if( sys.printResilienceInfo() )
            printResilienceInfo();
    }

    ResiliencePersistence::~ResiliencePersistence() {
        //Free mapped file for resilience tree.
        int res = munmap( _resilienceTree, _RESILIENCE_TREE_MAX_FILE_SIZE );
        if( res == -1 )
            fatal( "Error unmapping file." );
        close( _resilienceTreeFileDescriptor );

        //Free mapped file for resilience results.
        res = munmap( _resilienceResults, _RESILIENCE_RESULTS_MAX_FILE_SIZE );
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
            rn->_desc = current->_next;
            current = getResilienceNode( current->_next );
            freeResilienceNode( toDelete );
            rn->_descSize--;
        }

        if( rn->_descSize != 0 || rn->_desc != 0 )
            fatal0( "There are still descs" );
    }

    void ResiliencePersistence::printResilienceInfo() {
        // JUST FOR DEBUG PURPOSES
        size_t free_results = 0;
        for( std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.begin();
                it != _freeResilienceResults.end();
                it++ )
        {
            free_results += it->second;
            //std::cerr << "There are " << it->second << " bytes free starting at results[" << it->first << "]." << std::endl;
        }
        //std::cerr << "There are " << free_results << " bytes free in results." << std::endl;
        std::cerr << "There are " << _RESILIENCE_RESULTS_MAX_FILE_SIZE - free_results << " bytes used in results." << std::endl;
        std::cerr << "There are " << _usedResilienceNodes.size() << " resilience nodes in use." << std::endl;
    }

    /********** RESILIENCE PERSISTENCE **********/
    

    /********** RESILIENCE NODE **********/

    bool ResilienceNode::addNext( int next ) {
        if( _next == 0 ) {
            _next = next; 
            return true;
        }

        ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( _next );
        while( current->_next != 0 ) {
            current = sys.getResiliencePersistence()->getResilienceNode( current->_next );
        }
        current->_next = next; 
        return true;
    }


    void ResilienceNode::storeResult( CopyData * copies, size_t numCopies, int task_id ) {
        //Calculate outputs size
        size_t outputs_size = 0;
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                outputs_size += copies[i].getDimensions()->accessed_length;
            }
        }

        if( outputs_size <= 0 )
            return;
            //fatal( "Store result of 0 bytes makes no sense." );

        //Get result from resilience results mmaped file.
        _resultSize = outputs_size;
        //std::cerr << "Trying to reserve " << _resultSize << " bytes." << std::endl;
        _resultIndex = ( char * )sys.getResiliencePersistence()->getResilienceResultsFreeSpace( _resultSize ) 
            - ( char * )sys.getResiliencePersistence()->getResilienceResults( 0 );
        //Copy the result
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _resultIndex );
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

    void ResilienceNode::loadResult( CopyData * copies, size_t numCopies, int task_id ) { 
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _resultIndex );
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
            if( desc->_next == 0 ) {
                _lastDescRestored++;
                return NULL;
            }
            desc = sys.getResiliencePersistence()->getResilienceNode( desc->_next );
        }

        _lastDescRestored++;

        return desc;
    }

    bool ResilienceNode::addDesc( int desc ) { 
        if( desc == 0 )
            fatal0( "Trying to add null descendent." ); 

        if( _desc == 0 ) 
            _desc = desc; 
        else
            sys.getResiliencePersistence()->getResilienceNode( _desc )->addNext( desc );

        _descSize++;
        return true;
    }

    void ResilienceNode::removeAllDescs() {
        if( _descSize == 0 ) 
            return;

        ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( _desc );
        while( current != NULL ) {
            int toDelete = _desc;
            current->removeAllDescs();
            _desc = current->_next;
            current = sys.getResiliencePersistence()->getResilienceNode( current->_next );
            sys.getResiliencePersistence()->freeResilienceNode( toDelete );
            _descSize--;
        }

        if( _descSize != 0 || _desc != 0 )
            fatal0( "There are still descs" );
    }

    /********** RESILIENCE NODE **********/

}
